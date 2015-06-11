/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_COMMON_UTIL_H
#define RPCFRAME_COMMON_UTIL_H

#include <string>
namespace rpcframe {

bool getHostIp(std::string &str_ip);
bool getHostIpByName(std::string &str_ip, const char *hname);


};


#endif
