#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h>
#include <sys/epoll.h>

#include <string>

class connection;

enum Conn_Staus_e {
    CONN_OK = 0,
    CONN_ERROR,
    CONN_AGIN,
    CONN_BUSY,
    CONN_DONE,
    CONN_DECLINED,
    CONN_ABORT
};
#define CONNECTION_MAX_BUFSIZE      (1024*1024)
#define CONNECTION_INVALID_FD       (-1)
class connection
{
public:
    connection(int sockfd = -1);
    virtual ~connection();
    
    virtual ssize_t read(size_t n = CONNECTION_MAX_BUFSIZE - 1);
    virtual ssize_t write(size_t n = CONNECTION_MAX_BUFSIZE - 1);
    virtual size_t copy_to_buf(const char *buf, size_t n);
    
    bool set_nonblocking();
    
    void dump_read_buf_info();
    void dump_write_buf_info();
    
    virtual void add_event(uint32_t ev);
    virtual void remove_event(uint32_t ev);
    virtual void set_client_addr(sockaddr_in & addr);
    uint32_t get_events() const;
    int get_fd() const;
    void set_epoll_event(const epoll_event & ev);
    
    epoll_event     epev;
private:
    int             fd;
    sockaddr        client_addr;
    char            *rbuf;
    char            *wbuf;
    uint32_t        events;
};

#endif // CONNECTION_H