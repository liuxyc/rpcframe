#include "util.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <ctime>
#include <thread>
#include <sstream>

namespace rpcframe {

bool getHostIp(std::string &str_ip) {
    char hname[256] = {0};

    if( -1 == gethostname(hname, sizeof(hname))) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostname error %s", strerror(errno));
        return false;
    }

    return  getHostIpByName(str_ip, hname);
}

bool getHostIpByName(std::string &str_ip, const char *hname) {
  int len = 1024;
  char *buf = (char *)malloc(len);
  if(buf == nullptr) {
    return false;
  }
  int rc, err;
  struct hostent hbuf;
  struct hostent *result;

  while ((rc = gethostbyname_r(hname, &hbuf, buf, 1024, &result, &err)) == ERANGE) {
    /* expand buf */
    len *= 2;
    char *tmp = (char *)realloc(buf, len);
    if (nullptr == tmp) {
      free(buf);
      return false;
    }else{
      buf = tmp;
    }
  }
  if (0 != rc || nullptr == result) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname error %s", strerror(errno));
    free(buf);
    return false;
  }

  //RPC_LOG(RPC_LOG_LEV::DEBUG, "hostname: %s/naddress list: ", result->h_name);
  //get first hostname ip
  char *c_ip = inet_ntoa(*(struct in_addr*)(result->h_addr_list[0]));
  if(c_ip != nullptr) {
    str_ip.assign(c_ip);
    free(buf);
    return true;
  }
  free(buf);
  return false;
}

std::vector<std::string> log_level_map = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

void RPC_LOG_FUNC(RPC_LOG_LEV level, const char* func_name, const char *format, ... ){
  char logbuf[4096] = {0};
  va_list arglist;
  va_start( arglist, format );
  int writenum = vsnprintf(logbuf, 4096, format, arglist);
  va_end( arglist );
  struct timeval tm;
  gettimeofday(&tm, nullptr);
  std::stringstream log_ss;
  log_ss << "[";
  log_ss << log_level_map[(int)level] << " ";
  log_ss << tm.tv_sec << "." << tm.tv_usec * 1000 << " ";
  log_ss << std::hex << std::this_thread::get_id() << "][";
  log_ss << func_name << "] ";
  log_ss << logbuf;
  if(writenum > 4096) {
    log_ss << " LOG HAS BEEN TRUNCATED..." << std::dec << writenum;
  }
  printf("%s\n", log_ss.str().c_str());
}

};

