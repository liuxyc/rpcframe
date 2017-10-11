/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
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

extern RPC_LOG_LEV g_log_level;

extern void RPC_LOG_FUNC(RPC_LOG_LEV level, const char *file_name, int lineno, const char* func_name, const char *format, ... );

#define RPC_LOG(level, format, ...) \
RPC_LOG_FUNC(level, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__); \

};

