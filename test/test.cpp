extern "C" {
#include <arpa/inet.h>
#include <signal.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <string.h>

#include <errno.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
}

#include <string>
#include <vector>
#include <iostream>

#define INET_ADDRSTRLEN  16

void accept_conn_cb(struct evconnlistener * listener,
    evutil_socket_t fd,
    struct sockaddr * address,
    int socklen,
    void * arg) {

    char addr[INET_ADDRSTRLEN] = {0};

    sockaddr_in * sin = reinterpret_cast<sockaddr_in *>(address);
    inet_ntop(AF_INET, &(sin->sin_addr), addr, INET_ADDRSTRLEN);

    std::cout << "Accept TCP connection from: " << addr << std::endl;
}

void accept_error_cb(struct evconnlistener * listener, void * arg) {

    event_base * base = evconnlistener_get_base(listener);

    //处理错误
    int err = EVUTIL_SOCKET_ERROR();
    std::cerr << "Got an error on the listener: " \
        << evutil_socket_error_to_string(err) \
        << std::endl;

    event_base_loopexit(base, NULL);
}

int main(int argc, char ** argv) {
    const short port = 9877;
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    event_base * base = event_base_new();
    evconnlistener * listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
        reinterpret_cast<sockaddr * >(&sin), sizeof(sin)
    );

    if(listener == nullptr) {
        std::cerr << "Couldn't create listener" << std::endl;
        return 1; //exit with error
    }

    evconnlistener_set_error_cb(listener, accept_error_cb);
    event_base_dispatch(base);

    return 0; //exit successfully
}