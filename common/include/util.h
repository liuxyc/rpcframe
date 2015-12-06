/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_COMMON_UTIL_H
#define RPCFRAME_COMMON_UTIL_H

#include <string>
#include <vector>
#include <stdarg.h>

namespace rpcframe {

bool getHostIp(std::string &str_ip);
bool getHostIpByName(std::string &str_ip, const char *hname);

enum class RPC_LOG_LEV 
{
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
};

extern std::vector<std::string> log_level_map;


extern void RPC_LOG(RPC_LOG_LEV level, const char *format, ... );

};


#endif
