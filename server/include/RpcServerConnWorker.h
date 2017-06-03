/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#pragma once
#include <unordered_map>
#include <vector>
#include <atomic>
#include <thread>

#include "Queue.h"
#include "SpinLock.h"
#include "RWLock.h"
#include "RpcDefs.h"
#include "RpcPackage.h"

namespace rpcframe
{

class RpcServerConn;
class RpcServerImpl;
class IService;
class RpcStatusService;
class RpcRespBroker;
class RpcServerConfig;


class EpollStruct 
{
public:
    EpollStruct(int pfd, void *pptr)
    : fd(pfd)
    , ptr(pptr)
    {};
    int fd;
    void *ptr;
};

class RpcServerConnWorker
{
  friend RpcStatusService;
public:
  RpcServerConnWorker(RpcServerImpl *server, const char *name, ReqQueue *req_q);
  RpcServerConnWorker &operator=(const RpcServerConfig &cfg) = delete;
  ~RpcServerConnWorker();


  bool start(int listen_fd);
  void stop();

  void setSocketKeepAlive(int fd);
  void removeConnection(const EpollStruct *eps);
  void addConnection(int fd, RpcServerConn *conn);
  void pushResp(std::string seqid, RpcRespBroker &rb);
  const RpcServerConfig *getConfig();

  void onDataOut(EpollStruct *eps);
  bool onDataOutEvent(EpollStruct *eps);
  void onAccept();
  void onDataIn(const EpollStruct *eps);
  void setWorkQ(ReqQueue *q);
  void dumpConnIDs(std::vector<std::string> &ids);


private:
  RpcServerImpl *m_server;
  std::unordered_map<std::string, RpcServerConn *> m_conn_set;
  uint32_t m_seqid;
  ReqQueue *m_req_q;
  Queue<std::string> m_resp_conn_q;
  RWLock m_conn_rwlock;
  int m_epoll_fd;
  int m_listen_socket;
  int m_resp_ev_fd;
  std::atomic<bool> m_stop;
  std::string m_name;
};

};
