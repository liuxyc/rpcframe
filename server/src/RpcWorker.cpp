/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcWorker.h"

#include <sstream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <arpa/inet.h>

#include "RpcRespBroker.h"
#include "RpcServerConn.h"
#include "RpcServerImpl.h"
#include "RpcServerConnWorker.h"
#include "RpcMethod.h"
#include "IServiceImpl.h"
#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{


RpcWorker::RpcWorker(ReqQueue *workqueue, RpcServerImpl *server)
: m_work_q(workqueue)
, m_server(server)
, m_stop(false)
{
    m_thread = new std::thread(&RpcWorker::run, this);
}

RpcWorker::~RpcWorker() {
  delete m_thread;
  for(auto &p:m_srvmap){
    if(p.second.owner) {
      delete p.second.pSrv;
    }
  }
}

void RpcWorker::stop() {
    m_stop = true;
    m_thread->join();
}

void RpcWorker::addService(const std::string &name, IService *service, bool owner)
{
  m_srvmap[name] = ServiceBlock(service, owner);
}

void RpcWorker::run() {
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
    prctl(PR_SET_NAME, "RpcSWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        std::shared_ptr<request_pkg> pkg = nullptr;
        if (m_work_q->pop(pkg, 100)) {
            auto during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - pkg->gen_time);
            if (during.count() < 0) {
              during = during.zero();
            }
            m_server->calcReqQTime(during.count());
            uint32_t proto_len = ntohl(*((uint32_t *)pkg->data));

            //must get request id from here
            RpcInnerReq req;
            if (!req.ParseFromArray(pkg->data + sizeof(proto_len), proto_len)) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "parse internal pkg fail %s", req.DebugString().c_str());
                continue;
            }
            RawData rd;
            if(req.has_data()) {
              rd.data = (char *)req.data().data();
              rd.data_len = req.data().size();
            }
            else {
              rd.data = pkg->data + sizeof(proto_len);
              rd.data_len = pkg->data_len - sizeof(proto_len) - proto_len;
            }
            RpcServerConnWorker *connworker = pkg->conn_worker;
            if(connworker == nullptr) {
              RPC_LOG(RPC_LOG_LEV::ERROR, "connworker is null, %s %s", req.method_name().c_str(), req.request_id().c_str());
              continue;
            }
            RPC_LOG(RPC_LOG_LEV::DEBUG, "req %s:%s stay: %d ms", req.request_id().c_str(), req.method_name().c_str(), during.count());

            IService *p_service = m_srvmap[req.service_name()].pSrv;
            IRpcRespBrokerPtr rpcbroker = std::make_shared<RpcRespBroker>(connworker, 
                pkg->connection_id,
                req.request_id(),
                (req.type() == RpcInnerReq::TWO_WAY), nullptr);
            if (p_service != nullptr) {

                std::chrono::system_clock::time_point begin_call_timepoint = std::chrono::system_clock::now();
                RpcMethodStatusPtr method_status = nullptr;
                RpcStatus ret = p_service->m_impl->runMethod(req.method_name(), rd, rpcbroker, method_status);
                during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - begin_call_timepoint);
                if(method_status) {
                  if (during.count() < 0) {
                    during = during.zero();
                  }
                  m_server->calcCallTime(during.count());
                  method_status->calcCallTime(during.count());
                  RPC_LOG(RPC_LOG_LEV::DEBUG, "%s call take: %llu ms", req.method_name().c_str(), during.count());
                  auto timeout_during = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - pkg->gen_time);
                  if (req.timeout() > 0 && timeout_during.count() > req.timeout() + 3) {
                    if(method_status) {
                      ++(method_status->timeout_call_nums);
                    }
                    RPC_LOG(RPC_LOG_LEV::WARNING, "req %s:%s should already timeout on client %d->%d, will not response", req.request_id().c_str(), req.method_name().c_str(), timeout_during.count(), req.timeout() + 3);
                    continue;
                  }
                }
                switch (ret) {
                    case RpcStatus::RPC_SERVER_OK:
                      rpcbroker->setReturnVal(ret);
                        //TODO:if broker already call response, continue loop here
                        if(rpcbroker->isResponed()) {
                          continue;
                        }
                        break;
                    case RpcStatus::RPC_METHOD_NOTFOUND:
                        rpcbroker->setReturnVal(ret);
                        RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow method request #%s#", req.method_name().c_str());
                        break;
                    case RpcStatus::RPC_SERVER_FAIL:
                        rpcbroker->setReturnVal(ret);
                        RPC_LOG(RPC_LOG_LEV::WARNING, "method call fail #%s#", req.method_name().c_str());
                        break;
                    case RpcStatus::RPC_SERVER_NONE:
                        continue;
                        break;
                    default:
                        break;
                }
            }
            else {
              rpcbroker->setReturnVal(RpcStatus::RPC_SRV_NOTFOUND);
              RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow service request #%s#", req.service_name().c_str());
            }
            if (req.type() == RpcInnerReq::TWO_WAY) {
                //put response to connection queue, max worker throughput
                connworker->pushResp(pkg->connection_id, *(dynamic_cast<RpcRespBroker *>(rpcbroker.get())));
            }
        } 
        else {
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "thread: %lu, no data", std::this_thread::get_id());
        }
    }
}

};
