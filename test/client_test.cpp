//g++ -g client_test.cpp -o cpp_cli
//

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;


int main()
{
    struct sockaddr_in in;
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct in_addr s;
    bzero(&in, sizeof(in));
    in.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", (void *)&s);
    in.sin_addr.s_addr = s.s_addr;
    in.sin_port = htons(9877);

    string str = "I am Michael";
    sendto(fd, str.c_str(), str.size(), 0, (struct sockaddr *)&in, sizeof(struct sockaddr_in));

    return 0;
}