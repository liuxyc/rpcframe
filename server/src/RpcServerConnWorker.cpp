/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerConnWorker.h"

#include <sys/epoll.h>  
#include <sys/eventfd.h>
#include <netinet/tcp.h>  
#include <fcntl.h>  
#include <arpa/inet.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

#include <chrono>
#include <sstream>

#include "RpcServerImpl.h"
#include "RpcServerConn.h"
#include "util.h"
#include "rpc.pb.h"
#include "RpcServerConfig.h"
#include "RpcRespBroker.h"

#define RPC_MAX_SOCKFD_COUNT 65535 

namespace rpcframe
{

RpcServerConnWorker::RpcServerConnWorker(RpcServerImpl *server, const char *name, ReqQueue *req_q)
: m_server(server)
, m_seqid(0)
, m_req_q(req_q)
, m_epoll_fd(-1)
, m_listen_socket(-1)
, m_resp_ev_fd(-1)
, m_stop(false)
, m_name(name)
{
}

RpcServerConnWorker::~RpcServerConnWorker() {
  stop();
}

void RpcServerConnWorker::onDataOut(const int fd) {
  //RPC_LOG(RPC_LOG_LEV::DEBUG, "onDataOut %d", fd);
  auto conn_iter = m_conn_map.find(fd);
  if (conn_iter != m_conn_map.end()) {
    RpcServerConn *conn = conn_iter->second;
    PkgIOStatus sent_ret = conn->sendResponse();
    if (sent_ret == PkgIOStatus::FAIL) {
      removeConnection(fd);
    }
    else if (sent_ret == PkgIOStatus::PARTIAL){
      RPC_LOG(RPC_LOG_LEV::DEBUG, "OUT sent partial to %d", fd);
    }
    else if (sent_ret == PkgIOStatus::NODATA){
      struct epoll_event event_mod;  
      memset(&event_mod, 0, sizeof(event_mod));
      event_mod.data.fd = fd;
      event_mod.events = EPOLLIN | EPOLLET;
      epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
    }
    else {
      RPC_LOG(RPC_LOG_LEV::DEBUG, "OUT sent to %d", fd);
      //send full resp, remove EPOLLOUT flag
      if(conn->m_response_q.size() == 0) {
        struct epoll_event event_mod;  
        memset(&event_mod, 0, sizeof(event_mod));
        event_mod.data.fd = fd;
        event_mod.events = EPOLLIN | EPOLLET;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
      }
    }
  }
  else {
    RPC_LOG(RPC_LOG_LEV::DEBUG, "not found connection of %d", fd);
  }
}

bool RpcServerConnWorker::onDataOutEvent() {
  RPC_LOG(RPC_LOG_LEV::DEBUG, "onDataOutEvent");
  eventfd_t resp_cnt = -1;
  if (eventfd_read(m_resp_ev_fd, &resp_cnt) == -1) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "read resp event fail");
    return false;
  }

  std::string connid;
  std::unordered_map<std::string, RpcServerConn *>::iterator conn_iter;
  if (m_resp_conn_q.pop(connid, 0)) {
    ReadLockGuard rg(m_conn_rwlock);
    conn_iter = m_conn_set.find(connid);
    if (conn_iter != m_conn_set.end()) {
      RpcServerConn *conn = conn_iter->second;
      if(!conn->isSending()) {
        struct epoll_event ev;  
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = conn->getFd();
        epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev);  
      }
    }
    else {
      RPC_LOG(RPC_LOG_LEV::DEBUG, " not found connection for %s", connid.c_str());
    }
  }
  else {
    RPC_LOG(RPC_LOG_LEV::DEBUG, "can not peek connection for %s", connid.c_str());
  }

  return true;
}

void RpcServerConnWorker::onAccept() {
  //listen socket event
  sockaddr_in remote_addr;  
  int len = sizeof(remote_addr);  
  int new_client_socket = accept(m_listen_socket, 
      (sockaddr *)&remote_addr, 
      (socklen_t*)&len);  
  if (new_client_socket < 0) {
    if(errno != EAGAIN) {  
      RPC_LOG(RPC_LOG_LEV::ERROR, "accept fail %s, new_client_socket: %d", 
          strerror(errno), new_client_socket);  
    }
  }  
  else {
    if(m_server->GetConnCount() >= m_server->getConfig()->m_max_conn_num) {
      RPC_LOG(RPC_LOG_LEV::ERROR, "conn number reach limit %d, close %d", (uint32_t)m_server->getConfig()->m_max_conn_num, 
          new_client_socket);  
      ::close(new_client_socket);
      m_server->IncRejectedConn();
    }
    setSocketKeepAlive(new_client_socket);
    fcntl(new_client_socket, F_SETFL, fcntl(new_client_socket, F_GETFL) | O_NONBLOCK);
    struct epoll_event ev;  
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = new_client_socket;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, new_client_socket, &ev);  
    m_seqid++;
    std::string id(m_name);
    id += "_";
    id += std::to_string(m_seqid);
    addConnection(new_client_socket, new RpcServerConn(new_client_socket, id.c_str(), m_server));
    RPC_LOG(RPC_LOG_LEV::INFO, "new_client_socket: %d", new_client_socket);  
  }
}

void RpcServerConnWorker::onDataIn(const int fd) {
  //data come in
  RPC_LOG(RPC_LOG_LEV::DEBUG, "data coming in");
  RpcServerConn *conn = getConnection(fd);
  if (conn == nullptr) {
    RPC_LOG(RPC_LOG_LEV::INFO, "rpc server socket already disconnected: %d", fd);  
    return;
  }
  while(1) {
      pkg_ret_t pkgret = conn->getRequest();
      if( pkgret.first < 0 )  
      {  
          RPC_LOG(RPC_LOG_LEV::INFO, "rpc server socket disconnected: %d", fd);  
          removeConnection(fd);
          return;
      }  
      if(pkgret.second != nullptr) {
          pkgret.second->gen_time = std::chrono::system_clock::now();
          pkgret.second->conn_worker = this;
          //got a full request, put to worker queue
          if ( !m_req_q->push(pkgret.second)) {
              //queue fail, drop pkg
              RPC_LOG(RPC_LOG_LEV::WARNING, "server queue fail, drop pkg");
              m_server->IncReqInQFail();
          }
      }
      else {
          if(pkgret.first == 0) {
              break;
          }
      }  
  }
}

bool RpcServerConnWorker::start(int listen_fd) 
{
  sigset_t set;
  int s;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  s = pthread_sigmask(SIG_BLOCK, &set, NULL);
  std::stringstream ss;
  ss << std::this_thread::get_id();
  if (s != 0) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "thread %s block SIGINT fail", ss.str().c_str());
  }
  prctl(PR_SET_NAME, "RpcSConnWorker", 0, 0, 0); 
  m_listen_socket = listen_fd;
  m_epoll_fd = epoll_create(RPC_MAX_SOCKFD_COUNT);  
  if( m_epoll_fd == -1) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "epoll_create fail %s", strerror(errno));
    return false;
  }
  //set nonblock
  int opts = O_NONBLOCK;  
  if(fcntl(m_epoll_fd, F_SETFL, opts) < 0)  
  {  
    RPC_LOG(RPC_LOG_LEV::ERROR, "set epool fd nonblock fail");  
    return false;
  }  
  //listen socket epoll event
  struct epoll_event ev;  
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;  
  ev.data.fd = m_listen_socket;
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_socket, &ev);  

  //listen resp_ev_fd, this event fd used for response data avaliable notification
  m_resp_ev_fd = eventfd(0, EFD_SEMAPHORE);  
  if (m_resp_ev_fd == -1) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "create event fd fail");  
    return false;
  }

  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;  
  ev.data.fd = m_resp_ev_fd;
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_resp_ev_fd, &ev);  

  struct epoll_event events[RPC_MAX_SOCKFD_COUNT];  
  while(!m_stop) {
    int nfds = epoll_wait(m_epoll_fd, events, RPC_MAX_SOCKFD_COUNT, 2000);  
    if( nfds == -1) {
      if( m_stop ) {
        break;
      }
      RPC_LOG(RPC_LOG_LEV::ERROR, "epoll_wait");
    }
    for(int i = 0; i < nfds; i++)  
    {  
      int client_socket = events[i].data.fd;  
      if(client_socket == m_listen_socket) {
        onAccept();
      }
      else {
        //event fd, we have data to send
        if(events[i].events & EPOLLIN)
        {  
          if(client_socket == m_resp_ev_fd) {
            onDataOutEvent();
            continue;
          }
          else {
            onDataIn(client_socket);
          }
        }  

        if(events[i].events & EPOLLOUT)
        {  
          onDataOut(client_socket);
        }

        if(events[i].events & EPOLLERR) {
          RPC_LOG(RPC_LOG_LEV::ERROR, "EPOLL ERROR");
        }
        if(events[i].events & EPOLLHUP) {
          RPC_LOG(RPC_LOG_LEV::ERROR, "EPOLL HUP");
        }
      }
    }  
  }
  return true;
}

void RpcServerConnWorker::setSocketKeepAlive(int fd)
{
  int keepAlive = 1;   
  int keepIdle = 60;   
  int keepInterval = 5;   
  int keepCount = 3;   
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));  
  setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));  
  setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));  
  setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));  
}

void RpcServerConnWorker::removeConnection(int fd)
{
  WriteLockGuard wg(m_conn_rwlock);
  epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
  auto conn_iter = m_conn_map.find(fd);
  if (conn_iter != m_conn_map.end()) {
    m_conn_set.erase(conn_iter->second->m_seqid);
    delete conn_iter->second;
    m_conn_map.erase(conn_iter);
    m_server->DecConnCount();
  }
}

void RpcServerConnWorker::addConnection(int fd, RpcServerConn *conn)
{
  WriteLockGuard wg(m_conn_rwlock);
  m_conn_set.emplace(conn->m_seqid, conn);
  m_conn_map.emplace(fd, conn);
  m_server->IncConnCount();
}

RpcServerConn *RpcServerConnWorker::getConnection(int fd)
{
  auto conn_iter = m_conn_map.find(fd);
  if( conn_iter != m_conn_map.end()) {
    return conn_iter->second;
  }
  return nullptr;
}

void RpcServerConnWorker::pushResp(std::string conn_id, RpcRespBroker &br)
{
  //user may have no data to response
  if(!br.isAlloced()) {
    br.allocRespBuf(1);
  }
  RespPkgPtr resp_pkg = br.getRespPkg();
  if(resp_pkg.get() == nullptr) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "RespPkgPtr is null");
    assert(resp_pkg.get() != nullptr);
    return;
  }
  else {
    ReadLockGuard rg(m_conn_rwlock);
    auto conn_iter = m_conn_set.find(conn_id);
    if (conn_iter != m_conn_set.end()) {
      RpcServerConn *conn = conn_iter->second;
      resp_pkg->gen_time = std::chrono::system_clock::now();
      if (!conn->m_response_q.push(resp_pkg)) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "server resp queue fail, drop resp pkg");
        m_server->IncRespInQFail();
        return;
      }
      m_resp_conn_q.push(conn_id);

      eventfd_t resp_cnt = 1;
      if( eventfd_write(m_resp_ev_fd, resp_cnt) == -1) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "write resp event fd fail");
      }
    }
    else {
      RPC_LOG(RPC_LOG_LEV::WARNING, "connection %s gone, drop resp", conn_id.c_str());
    }
  }
  return;
}

void RpcServerConnWorker::stop() {
  if (m_stop) {
    return;
  }
  m_stop = true;
  RPC_LOG(RPC_LOG_LEV::INFO, "stoped");
}

void RpcServerConnWorker::setWorkQ(ReqQueue *q)
{
  m_req_q = q;
}

void RpcServerConnWorker::dumpConnIDs(std::vector<std::string> &ids)
{
  ReadLockGuard rg(m_conn_rwlock);
  for(auto &connid: m_conn_set) {
    ids.emplace_back(connid.first);
  }
}

};
