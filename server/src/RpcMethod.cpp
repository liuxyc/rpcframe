#include "RpcMethod.h"
#include "SpinLock.h"
#include "mutex"

namespace rpcframe
{

RpcMethodStatus::RpcMethodStatus()
  : enabled(true)
  , total_call_nums(0)
  , timeout_call_nums(0)
  , avg_call_time(0)
  , longest_call_time(0)
  , call_from_http_num(0)
  , m_stat_lock(new SpinLock())
{
}

RpcMethodStatus::~RpcMethodStatus()
{
  delete m_stat_lock;
}

void RpcMethodStatus::calcCallTime(uint64_t call_time)
{
  std::lock_guard<SpinLock> mlock(*m_stat_lock);
  if (avg_call_time == 0) {
    avg_call_time = call_time;
  }
  else {
    avg_call_time = ((avg_call_time * total_call_nums) + call_time) / (total_call_nums + 1);
  }
  ++total_call_nums;
  if(call_time > longest_call_time) {
    longest_call_time = call_time;
  }
}

RpcMethod::RpcMethod(const RPC_FUNC_T &func) 
  : m_func(func)
  , m_status(new RpcMethodStatus()) 
{
}

RpcMethod::~RpcMethod() {
  delete m_status;
}

RpcMethod::RpcMethod(RpcMethod &&m) {
  m_func = m.m_func;
  m_status = m.m_status;
  m.m_status = nullptr;
}

};
