#include <arpa/inet.h>
#include <signal.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>

#include <string>
#include <vector>
#include <iostream>

std::vector<std::string> hostname_to_ip(const std::string & hostname) {
    std::vector<std::string> ip_ans;
    hostent * hname_info = gethostbyname(hostname.c_str());
    if(hname_info != NULL) {
        for(int i = 0; hname_info->h_addr_list[i] != NULL; i++) {
            char buf[64] = {0};
            std::cout << "the ip address " << i << " : " << inet_ntop(hname_info->h_addrtype, hname_info->h_addr_list[i], buf, sizeof(buf) - 1) << std::endl;
        }
    }
    return ip_ans;
}

int main(int argc, char *argv[]) {
    hostname_to_ip("www.baidu.com");
    return 0;
}
