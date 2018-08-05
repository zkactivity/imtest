
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
#include <map>
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
const char listen_addr[] = "0.0.0.0";
const int SERVER_PORT = 9877;
const int INFTIM = 1000;

static int listenfd;
static int epfd;

static map<int, connection *> clients;
int main(int argc, char **argv) {
	int i, maxi, connfd, sockfd, nfds, portnumber;
	ssize_t n;
	char line[MAXLINE];
	socklen_t clilen;
	string szTemp("");

	//����epoll_event�ṹ��ı���, ev����ע���¼�,�������ڻش�Ҫ������¼�
	struct epoll_event ev, events[20];

	//����һ��epoll���, size���������ں����������Ŀһ���ж��
	epfd = epoll_create(256); 	//�������ڴ���accept��epollר���ļ�������

	struct sockaddr_in clientaddr;
	struct sockaddr_in servaddr;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//��socket��Ϊ������
	setNonblocking(listenfd);

	//����Ҫ������¼���ص��ļ�������
	ev.data.fd = listenfd;

	//����Ҫ������¼�����
	ev.events = EPOLLIN|EPOLLET;

	//ע��epoll�¼�
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(listen_addr);
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
		//�ȴ�epoll�¼��ķ���
		//������Ҫ������¼���Ŀnfds, ������0��ʾ��ʱ
		nfds = epoll_wait(epfd, events, 20, 500);

		//�������������¼�
		for(i = 0; i < nfds; i++) {
			//�����⵽һ��socket�û����ӵ��˰󶨵Ķ˿ڣ������µ�����
			if(events[i].data.fd == listenfd) {
				connfd = accept(listenfd, (sockaddr *)(&clientaddr), &clilen);
				if(connfd < 0) {
					perror("accept error");
					exit(1);
				}
                connection * client_conn = new connection(connfd);
                client_conn->set_client_addr(clientaddr);
                client_conn->set_nonblocking();
				//setNonblocking(connfd);
				char *str = inet_ntoa(clientaddr.sin_addr);
				cout << "accept a connection from " << str << endl;

				//�������ڶ��������ļ�������
				client_conn->epev.data.fd = client_conn->get_fd();
				//ע������¼�
                client_conn->add_event(EPOLLIN);
                client_conn->add_event(EPOLLET);

				//ע��ev
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_conn->get_fd(), &(client_conn->epev));  /*����¼�*/
                clients[client_conn->get_fd()] = client_conn;
			} else if(events[i].events&EPOLLIN) {
				cout << "EPOLLIN " << endl;
				if((sockfd = events[i].data.fd) < 0) {
					continue;
				}
				clients[events[i].data.fd]->set_epoll_event(events[i]);
				
				memset(line, 0, sizeof(line));
				if((n = clients[events[i].data.fd]->read()) < 0) {
					//Connection Reset
					if(errno == ECONNRESET) {
						//close(sockfd);
						clients.erase(clients.find(events[i].data.fd));
						events[i].data.fd = -1;
					} else {
						cout << "read line error" << endl;
					}
				} else if (n == 0)  { //��ȡ����Ϊ��
					//close(sockfd);
					clients.erase(clients.find(events[i].data.fd));
					events[i].data.fd = -1;
				}
				clients[events[i].data.fd]->dump_read_buf_info();
				//szTemp = "";
				//szTemp += line;
				//szTemp = szTemp.substr(0, szTemp.find('\r'));  //remove the enter key
				//memset(line, 0, sizeof(line));
				//cout << "Read in: " << szTemp << endl;

				//��������д�������ļ�������
				//ev.data.fd = sockfd;
				//����д�¼�
				//ev.events = EPOLLOUT|EPOLLET;
				clients[events[i].data.fd]->remove_event(EPOLLIN);
				clients[events[i].data.fd]->add_event(EPOLLET);
				clients[events[i].data.fd]->add_event(EPOLLOUT);
				//�޸�sockfd�ϵ�Ҫ������¼�ΪEPOLLOUT
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &(clients[events[i].data.fd]->epev));
			} else if(events[i].events & EPOLLOUT) {
				sockfd = events[i].data.fd;
				szTemp = "Server message!\n";
				clients[events[i].data.fd]->copy_to_buf(szTemp.c_str(), szTemp.length());
				clients[events[i].data.fd]->write(szTemp.length() + 1);
				//send(sockfd, szTemp.c_str(), szTemp.size(), 0);

				//���ö�������������
				ev.data.fd = sockfd;
				//���ö������¼�
				//ev.events = EPOLLIN|EPOLLET;
				clients[events[i].data.fd]->remove_event(EPOLLOUT);
				clients[events[i].data.fd]->add_event(EPOLLET);
				clients[events[i].data.fd]->add_event(EPOLLIN);
				//���ö��¼�
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &(clients[events[i].data.fd]->epev));
			}
		}
	}
	close(epfd);
	return 0;
}

