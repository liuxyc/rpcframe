#include "util.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    struct hostent *hent;
    hent = gethostbyname(hname);
    if(hent == nullptr) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname error %s", strerror(errno));
        return false;
    }

    //RPC_LOG(RPC_LOG_LEV::DEBUG, "hostname: %s/naddress list: ", hent->h_name);
    //get first hostname ip
    char *c_ip = inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]));
    if(c_ip != nullptr) {
        str_ip.assign(c_ip);
        return true;
    }
    return false;
}

std::vector<std::string> log_level_map = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

void RPC_LOG(RPC_LOG_LEV level, const char *format, ... ){
  char logbuf[4096];
  va_list arglist;
  va_start( arglist, format );
  vsprintf(logbuf, format, arglist );
  va_end( arglist );
  std::stringstream log_ss;
  log_ss << "[";
  log_ss << log_level_map[(int)level];
  log_ss << " ";
  log_ss << std::time(nullptr);
  log_ss << " ";
  log_ss << std::hex << std::this_thread::get_id();
  log_ss << "] ";
  log_ss << logbuf;
  printf("%s\n", log_ss.str().c_str());
}

};

