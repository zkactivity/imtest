#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <poll.h>
#include <sys/epoll.h>
#include <signal.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>  
#include <openssl/err.h>

#include <string>
#include <iostream>
using namespace std;

#define log(...) do{printf(__VA_ARGS__);fflush(stdout);}while(0)
#define check0(x, ...) if(x) do { log( __VA_ARGS__); exit(1); } while(0)
#define check1(x, ...) if(!x) do { log( __VA_ARGS__); exit(1); } while(0)

BIO * g_errBio;
SSL_CTX * g_sslCtx;

int g_epollfd, g_listenfd;

class Channel {
public:
	int m_fd;
	SSL *m_pSsl;
	bool m_bTcpConnected;
	bool m_bSslConnected;
	int m_events;

	Channel(int fd, int events):
		m_fd(fd),
		m_events(events),
		m_pSsl(nullptr),
		m_bTcpConnected(false),
		m_bSslConnected(false)
	{ }

 	void Update() {
 		struct epoll_event epev;
		memset(&epev, 0, sizeof(epev));
		epev.events = m_events;
		epev.data.ptr = this;
		log("modifying fd %d events read %d write %d\n", m_fd, epev.events & EPOLLIN, epev.events & EPOLLOUT);
		int r = epoll_ctl(g_epollfd, EPOLL_CTL_MOD, m_fd, &epev);
        check0(r, "epoll_ctl mod failed %d %s", errno, strerror(errno));
	}

	~Channel() {
		if(m_pSsl != nullptr) {
			SSL_shutdown(m_pSsl);
			SSL_free(m_pSsl);
			m_pSsl = nullptr;
			m_bSslConnected = false;
		}
        log("deleting fd %d\n", m_fd);
		close(m_fd);
		m_fd = -1;
		m_bTcpConnected = false;
	}
		
};

int setNonBlock(int fd, bool value) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags < 0) {
		return errno;
	}
	if(value) {
		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	} else {
		return fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
	}
}

void addEpollFd(int epollfd, Channel * ch) {
	struct epoll_event epev;
	memset(&epev, 0, sizeof(epev));
	epev.events = ch->m_events;
	epev.data.ptr = ch;
    log("adding fd %d events %d\n", ch->m_fd, epev.events);
	int r = epoll_ctl(epollfd, EPOLL_CTL_ADD, ch->m_fd, &epev);
    check0(r, "epoll_ctl add failed %d %s", errno, strerror(errno));
}

int createServer(short port, int listens) {
	int fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	setNonBlock(fd, true);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	int r = ::bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    check0(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));

	r = listen(fd, listens);
    check0(r, "listen failed %d %s", errno, strerror(errno));
    log("fd %d listening at %d\n", fd, port);
    return fd;
}

void handleAccept(int epollfd, int listenfd) {
	struct sockaddr_in rd_addr;
	socklen_t rd_size = sizeof(rd_addr);
	int client_fd;

	while((client_fd = accept4(listenfd, (struct sockaddr *)&rd_addr, &rd_size, SOCK_CLOEXEC)) >= 0) {

		sockaddr_in peer, local;
		socklen_t alen = sizeof(peer);

		int r = getpeername(client_fd, (sockaddr *)&peer, &alen);
		if(r < 0) {
            log("get peer name failed %d %s\n", errno, strerror(errno));
            continue;
		}

		r = getsockname(client_fd, (sockaddr *)&local, &alen);
		if(r < 0) {
            log("getsockname failed %d %s\n", errno, strerror(errno));
            continue;
		}

		setNonBlock(client_fd, true);
		Channel * ch = new Channel(client_fd, EPOLLIN | EPOLLOUT);
		addEpollFd(epollfd, ch);
	}
}

void handleHandshake(Channel * ch, SSL_CTX * ctx, BIO * errBio) {
	if(!(ch->m_bTcpConnected)) {
		struct pollfd pfd;
		pfd.fd = ch->m_fd;
		pfd.events = POLLOUT | POLLERR;
		int r = poll(&pfd, 1, 0);  //目的是为了检测套接字是否可写，若可写，则表示已正确连接
		if((r == 1) && (pfd.revents == POLLOUT)) {
            log("tcp connected fd %d\n", ch->m_fd);
			ch->m_bTcpConnected = true;
			ch->m_events = EPOLLIN|EPOLLOUT|EPOLLERR;
			ch->Update();
		} else {
            log("poll fd %d return %d revents %d\n", ch->m_fd, r, pfd.revents);
			delete ch;
			return;
		}
	}

	if(ch->m_pSsl == nullptr) {
		ch->m_pSsl = SSL_new(ctx);
        check0(ch->m_pSsl == nullptr, "SSL_new failed");

		int r = SSL_set_fd(ch->m_pSsl, ch->m_fd);
        check0(!r, "SSL_set_fd failed");

        log("SSL_set_accept_state for fd %d\n", ch->m_fd);
		SSL_set_accept_state(ch->m_pSsl);
	}

	int r = SSL_do_handshake(ch->m_pSsl);
	if(r == 1) {
		ch->m_bSslConnected = true;
        log("ssl connected fd %d\n", ch->m_fd);
		return;
	}

	int err = SSL_get_error(ch->m_pSsl, r);
	int oldev = ch->m_events;
	if(err == SSL_ERROR_WANT_WRITE) {
		ch->m_events |= EPOLLOUT;
		ch->m_events &= ~EPOLLIN;
        log("return want write set events %d\n", ch->m_events);
		if(oldev == ch->m_events) return;
		ch->Update();
	} else if(err == SSL_ERROR_WANT_READ) {
		ch->m_events |= EPOLLIN;
		ch->m_events &= (~EPOLLOUT);
        log("return want read set events %d\n", ch->m_events);
		if(oldev == ch->m_events) return;
		ch->Update();
	} else {
		log("SSL_do_handshake return %d error %d errno %d msg %s\n", r, err, errno, strerror(errno));
		ERR_print_errors(errBio);
		delete ch;
	}
}

void handleDataRead(Channel * ch) {
	char buf[4096];
	int rd = SSL_read(ch->m_pSsl, buf, sizeof buf - 1);
	int ssle = SSL_get_error(ch->m_pSsl, rd);
	if(rd > 0) {
		cout << "Get data from client: <<<\033[1;31m" << endl << buf << "\033[0m>>>" << endl;
        const char* cont = "HTTP/1.1 200 OK\r\nConnection: Close\r\nContent-Length: 18\r\n\r\n<p>Server Msg!</p>";
        int len1 = strlen(cont);
        int wd = SSL_write(ch->m_pSsl, cont, len1);
        log("SSL_write %d bytes\n", wd);
        delete ch;
	}

	if((rd < 0) && (ssle != SSL_ERROR_WANT_READ)) {
        log("SSL_read return %d error %d errno %d msg %s", rd, ssle, errno, strerror(errno));
        delete ch;
        return;
	}

	if(rd == 0) {
        if (ssle == SSL_ERROR_ZERO_RETURN) {
            log("SSL has been shutdown.\n");
        }
        else {
            log("Connection has been aborted.\n");
        }
        delete ch;
	}
}

void handleRead(Channel * ch, int erpollfd, int listenfd) {
	if(ch->m_fd == listenfd) {
		handleAccept(erpollfd, listenfd);
		return;
	}
	if(ch->m_bSslConnected) {
		handleDataRead(ch);
		return;
	}
	handleHandshake(ch, g_sslCtx, g_errBio);
}

void handleWrite(Channel * ch, SSL_CTX * ctx, BIO * errBio) {
	if(!(ch->m_bSslConnected)) {
		handleHandshake(ch, ctx, errBio);
		return;
	}

    log("handle write fd %d\n", ch->m_fd);
	ch->m_events &= (~EPOLLOUT);
	ch->Update();
}

void initSSL() {
	SSL_load_error_strings();

	int r = SSL_library_init();
    check0(!r, "SSL_library_init failed");

	g_sslCtx = SSL_CTX_new(SSLv23_method());
    check0(g_sslCtx == nullptr, "SSL_CTX_new failed");

	g_errBio = BIO_new_fd(STDERR_FILENO, BIO_NOCLOSE);

	string cert = "/opt/ca/certs/stf.cert.pem";
	string key = "/opt/ca/private/stf.key.pem";
	r = SSL_CTX_use_certificate_file(g_sslCtx, cert.c_str(), SSL_FILETYPE_PEM);
    check0(r <= 0, "SSL_CTX_use_certificate_file %s failed", cert.c_str());

	r = SSL_CTX_use_PrivateKey_file(g_sslCtx, key.c_str(), SSL_FILETYPE_PEM);
    check0(r<=0, "SSL_CTX_use_PrivateKey_file %s failed", key.c_str());

	r = SSL_CTX_check_private_key(g_sslCtx);
    check0(!r, "SSL_CTX_check_private_key failed");
    log("SSL inited\n");
}

int g_stop = 0;

void loop_once(int epollfd, int waitms) {
	const int kMaxEvents = 20;
	struct epoll_event activeEvs[kMaxEvents];
	int n = epoll_wait(epollfd, activeEvs, kMaxEvents, waitms);
	for(int i = 0; i < n; i++) {
		Channel * ch = (Channel *)(activeEvs[i].data.ptr);
		int events = activeEvs[i].events;
		if(events & (EPOLLIN|EPOLLERR)) {
            log("fd %d handle read\n", ch->m_fd);
			handleRead(ch, g_epollfd, g_listenfd);
		} else if(events & EPOLLOUT) {
            log("fd %d handle write\n", ch->m_fd);
			handleWrite(ch, g_sslCtx, g_errBio);
		} else {
            log("unknown event %d\n", events);
		}
	}
}

void handleInterrupt(int sig) {
	g_stop = true;
}

int main(int argc, char **argv) {
	signal(SIGINT, handleInterrupt);
	initSSL();

	g_epollfd = epoll_create1(EPOLL_CLOEXEC);

	g_listenfd = createServer(443, 20);

	Channel * li = new Channel(g_listenfd, EPOLLIN);

	addEpollFd(g_epollfd, li);

	while(!g_stop) {
		loop_once(g_epollfd, 5000);
	}

	delete li;
	
	::close(g_epollfd);
	BIO_free(g_errBio);
	SSL_CTX_free(g_sslCtx);

	ERR_free_strings();
    log("program exited\n");
	
	return 0;
}