/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
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
#include "ThreadPool.h"
#include "Epoll.h"

namespace rpcframe
{

class RpcServerConn;
class RpcServerImpl;
class IService;
class RpcStatusService;
class RpcRespBroker;
class RpcServerConfig;


enum class ConnType
{
    RPC_CONN = 0,
    HTTP_CONN = 1,
};

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
  RpcServerConnWorker(RpcServerImpl *server, const char *name, ConnType ctype);
  RpcServerConnWorker &operator=(const RpcServerConfig &cfg) = delete;
  ~RpcServerConnWorker();


  bool start(int listen_fd);
  void stop();

  void setSocketKeepAlive(int fd);
  void removeConnection(const EpollStruct *eps);
  void addConnection(int fd, RpcServerConn *conn);
  void pushResp(std::string seqid, RpcRespBroker &rb);
  const RpcServerConfig *getConfig();

  bool onDataOut(EpollStruct *eps);
  bool onDataOutEvent(EpollStruct *eps);
  void onAccept();
  bool onDataIn(const EpollStruct *eps);
  //void setWorkQ(ReqQueue *q);
  void dumpConnIDs(std::vector<std::string> &ids);


private:
  RpcServerImpl *m_server;
  std::unordered_map<std::string, RpcServerConn *> m_conn_set;
  uint32_t m_seqid;
  //ReqQueue *m_req_q;
  Queue<std::string> m_resp_conn_q;
  RWLock m_conn_rwlock;
  Epoll m_epoll;
  int m_listen_socket;
  int m_resp_ev_fd;
  std::atomic<bool> m_stop;
  std::string m_name;
  ConnType m_conn_type;
};

};
