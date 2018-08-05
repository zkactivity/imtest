#include "connection.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>
using namespace std;
connection::connection(int sockfd) {
    fd = sockfd;
    rbuf = new char[CONNECTION_MAX_BUFSIZE]();
    wbuf = new char[CONNECTION_MAX_BUFSIZE]();
    events = 0;
}

connection::~connection() {
    if(fd > 0) {
        close(fd);
        fd = CONNECTION_INVALID_FD;
    }
    delete [] rbuf;
    rbuf = nullptr;
    delete [] wbuf;
    wbuf = nullptr;
}


ssize_t connection::read(size_t n) {
    n = (n > (CONNECTION_MAX_BUFSIZE - 1)) ? (CONNECTION_MAX_BUFSIZE - 1) : n;
    ssize_t recv_len = recv(fd, rbuf, n, 0);
    wbuf[CONNECTION_MAX_BUFSIZE - 1] = '\0';
    if(recv_len <= 0) {
        perror("connection::read");
    }
    return recv_len;
}

ssize_t connection::write(size_t n) {
    n = (n > (CONNECTION_MAX_BUFSIZE - 1)) ? (CONNECTION_MAX_BUFSIZE - 1) : n;
    ssize_t send_len = send(fd, wbuf, n, 0);
    if(send_len <= 0) {
        perror("connection::write");
    }
    return send_len;
}

size_t connection::copy_to_buf(const char *buf, size_t n) {
    n = (n > (CONNECTION_MAX_BUFSIZE - 1)) ? (CONNECTION_MAX_BUFSIZE - 1) : n;
    memcpy(wbuf, buf, n);
    wbuf[CONNECTION_MAX_BUFSIZE - 1] = '\0';
    return n;
}

bool connection::set_nonblocking() {
    if(fd > 0) {
        int opts = fcntl(fd, F_GETFL);
        if(opts < 0) {
            perror("fcntl F_GETFL");
            return false;
        }
        opts = opts | O_NONBLOCK;
        if(fcntl(fd, F_SETFL, opts) < 0) {
            perror("fcntl F_SETFL");
            return false;
        }
    }
    return true;
}

void connection::dump_read_buf_info() {
    cout << "the read buffer info <<<" << endl;
    cout << rbuf << endl;
    cout << ">>>" << endl;
}

void connection::dump_write_buf_info() {
    cout << "the read buffer info <<<" << endl;
    cout << wbuf << endl;
    cout << ">>>" << endl;
}

void connection::add_event(uint32_t ev) {
    epev.events = epev.events | ev;
}

void connection::remove_event(uint32_t ev) {
    epev.events = epev.events & (~ev);
}

void connection::set_client_addr(sockaddr_in & addr) {
    memcpy(&client_addr, &addr, sizeof(client_addr));
}

uint32_t connection::get_events() const {
    return epev.events;
}

int connection::get_fd() const {
    return fd;
}

void connection::set_epoll_event(const epoll_event & ev) {
    memcpy(&epev, &ev, sizeof(epev));
}