#ifndef _IM_EVENT_H_
#define _IM_EVENT_H_

extern "C" {
#include <sys/epoll.h>
}
#include <string>

enum im_event_status{
IM_EVENT_OK  		= 0,
IM_EVENT_ERROR,
IM_EVENT_AGAIN,
IM_EVENT_BUSY,
IM_EVENT_DONE,
IM_EVENT_DECLINED,
IM_EVENT_ABORT,
};

typedef ssize_t (*im_recv_pt)(im_connection_t *c, char * buf, size_t size);
typedef ssize_t (*im_send_pt)(im_connection_t *c, char *buf, size_t size);
typedef void (*im_event_handler_pt)(im_event_t *ev);
typedef void (*im_connection_handler_pt)(im_connection_t *c)

struct im_listening_t {
	int 		 	fd;
	sockaddr 	 	*skaddr;
	socklen_t	 	socklen;
	size_t 		 	addr_text_max_len;
	std::string  	addr_text;

	int 		 	type;

	int 	 		backlog;
	int 			rcvbuf;
	int 			sndbuf;

	/* have keep alive tunable */
	int 			keepidle;
	int 			keepintvl;
	int 			keepcnt;

	/* handler of accepted connection */
	im_connection_handler_pt 		handler;

	void 			*server;
	int 			post_accept_timeout;
};

struct im_event_t {
	void 		*data;
	bool 		write;
	bool 		accept;
	bool 		instance;
	bool 		active;
	bool 		disabled;
	bool 		ready;
	bool 		oneshot;
	bool 		complete;
};

struct im_connection_t {
	void 			*data;
	int 			fd;

	im_recv_pt 		recv;
	im_send_pt 		send;

	im_listening_t 	*listening;
	off_t 			sent;

	void 			*buf;

	int 			type;

	sockaddr 		*skaddr;
	socklen_t 		socklen;
	std::string 	addr_text;

	std::string 	proxy_protocol_addr;
	short 			proxy_protocol_port;

	im_ssl_connection 		*ssl_conn;
};



#define MAX_EPOLL_EVENTS  	1024
#endif
