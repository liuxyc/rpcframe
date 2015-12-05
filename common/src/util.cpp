#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"

namespace rpcframe {

bool getHostIp(std::string &str_ip) {
    char hname[256] = {0};

    if( -1 == gethostname(hname, sizeof(hname))) {
        printf("gethostname error %s\n", strerror(errno));
        return false;
    }

    return  getHostIpByName(str_ip, hname);
}

bool getHostIpByName(std::string &str_ip, const char *hname) {
    struct hostent *hent;
    hent = gethostbyname(hname);
    if(hent == nullptr) {
        printf("gethostbyname error %s\n", strerror(errno));
        return false;
    }

    //printf("hostname: %s/naddress list: ", hent->h_name);
    //get first hostname ip
    char *c_ip = inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]));
    if(c_ip != nullptr) {
        str_ip.assign(c_ip);
        return true;
    }
    return false;
}

};

