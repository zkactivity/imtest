
extern "C" {
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
}
#include "connection.hpp"
#include <iostream>
using namespace std;

char uri_root[512];

struct table_entry {
	string extension;
	string content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ "", "" },
};

static const string guess_content_type(string path) {
	int pos = path.find_last_of('.');
	if(pos != path.npos) {
		string extension = path.substr(pos + 1, path.length() - pos);
		for(table_entry * tent = &content_type_table[0]; tent->extension.length() > 0; tent++) {
			if(strcasecmp(tent->extension.c_str(), extension.c_str()) == 0) {
				return tent->content_type;
			}
		}
	}
    return "application/misc";
}

void setNonblocking(int sockfd) {
	int opts = fcntl(sockfd, F_GETFL);
	if(opts < 0) {
		perror("fcntl(sockfd, F_GETFL)");
		exit(1);
	}

	opts = opts | O_NONBLOCK;
	if(fcntl(sockfd, F_SETFL, opts) < 0) {
		perror("fcntl(sockfd, F_SETFL, opts)");
		exit(1);
	}
}

const int MAXLINE = 100;
const int OPENMAX = 100;
const int LISTENQ = 20;
const int SERVER_PORT = 9877;
const int INFTIM = 1000;

int main(int argc, char **argv) {
	int i, maxi, listenfd, connfd, sockfd, epfd, nfds, portnumber;
	ssize_t n;
	char line[MAXLINE];
	socklen_t clilen;
	string szTemp("");

	//声明epoll_event结构体的变量, ev用于注册事件,数组用于回传要处理的事件
	struct epoll_event ev, events[20];

	//创建一个epoll句柄, size用来高速内核这个监听数目一共有多大
	epfd = epoll_create(256); 	//生成用于处理accept的epoll专用文件描述符

	struct sockaddr_in clientaddr;
	struct sockaddr_in servaddr;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//把socket设为非阻塞
	setNonblocking(listenfd);

	//设置要处理的事件相关的文件描述符
	ev.data.fd = listenfd;

	//设置要处理的事件类型
	ev.events = EPOLLIN|EPOLLET;

	//注册epoll事件
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	const char *listenaddr = "127.0.0.1";
	servaddr.sin_addr.s_addr = inet_addr(listenaddr);
	servaddr.sin_port = htons(SERVER_PORT);

	if(bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind error");
		exit(1);
	}

	if(listen(listenfd, LISTENQ) < 0) {
		perror("listen error");
		exit(1);
	}

	maxi = 0;

	while(true) {
		//等待epoll事件的发生
		//返回需要处理的事件数目nfds, 若返回0表示超时
		nfds = epoll_wait(epfd, events, 20, 500);

		//处理发生的所有事件
		for(i = 0; i < nfds; i++) {
			//如果监测到一个socket用户连接到了绑定的端口，简历新的连接
			if(events[i].data.fd == listenfd) {
				connfd = accept(listenfd, (sockaddr *)&clientaddr, &clilen);
				if(connfd < 0) {
					perror("accept error");
					exit(1);
				}
				setNonblocking(connfd);
				char *str = inet_ntoa(clientaddr.sin_addr);
				cout << "accept a connection from " << str << endl;

				//设置用于读操作的文件描述符
				ev.data.fd = connfd;
				//注册监听事件
				ev.events = EPOLLIN|EPOLLET;

				//注册ev
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);  /*添加事件*/
			} else if(events[i].events&EPOLLIN) {
				cout << "EPOLLIN " << endl;
				if((sockfd = events[i].data.fd) < 0) {
					continue;
				}
				memset(line, 0, sizeof(line));
				if((n = recv(sockfd, line, sizeof(line) - 1, 0)) < 0) {
					//Connection Reset
					if(errno == ECONNRESET) {
						close(sockfd);
						events[i].data.fd = -1;
					} else {
						cout << "read line error" << endl;
					}
				} else if (n == 0)  { //读取数据为空
					close(sockfd);
					events[i].data.fd = -1;
				}

				szTemp = "";
				szTemp += line;
				szTemp = szTemp.substr(0, szTemp.find('\r'));  //remove the enter key
				memset(line, 0, sizeof(line));
				cout << "Read in: " << szTemp << endl;

				//设置用于写操作的文件描述符
				ev.data.fd = sockfd;
				//设置写事件
				ev.events = EPOLLOUT|EPOLLET;
				//修改sockfd上的要处理的事件为EPOLLOUT
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
			} else if(events[i].events & EPOLLOUT) {
				sockfd = events[i].data.fd;
				szTemp = "Server message";
				send(sockfd, szTemp.c_str(), szTemp.size(), 0);

				//设置读操作的描述符
				ev.data.fd = sockfd;
				//设置读操作事件
				ev.events = EPOLLIN|EPOLLET;
				//设置读事件
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
			}
		}
	}
	close(epfd);
	return 0;
}

