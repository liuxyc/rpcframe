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
    char hname[256];
    struct hostent *hent;

    if( -1 == gethostname(hname, sizeof(hname))) {
        printf("gethostname error %s\n", strerror(errno));
        return false;
    }

    hent = gethostbyname(hname);
    if(hent == NULL) {
        printf("gethostbyname error %s\n", strerror(errno));
        return false;
    }

    //printf("hostname: %s/naddress list: ", hent->h_name);
    //get first hostname ip
    char *c_ip = inet_ntoa(*(struct in_addr*)(hent->h_addr_list[0]));
    if(c_ip != NULL) {
        str_ip.assign(c_ip);
        return true;
    }
    return false;

}

};

