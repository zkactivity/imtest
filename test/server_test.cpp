//g++ -g libevent_server.cpp -o libevent_server -levent -lpthread
//说明:服务器监听在本地19870端口, 等待udp client连接,有惊群现象: 当有数据到来时, 每个线程都被唤醒, 但是只有一个线程可以读到数据
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <event.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

int init_count = 0;
pthread_mutex_t init_lock;
pthread_cond_t init_cond;

typedef struct {
    pthread_t thread_id; /* unique ID of this thread */
    struct event_base *base; /* libevent handle this thread uses */
    struct event notify_event; /* listen event for notify pipe */
} mythread;

void *worker_libevent(void *arg)
{
    mythread *p = (mythread *)arg;
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
    event_base_loop(p->base, 0);
}

int create_worker(void*(*func)(void *), void *arg)
{
    mythread *p = (mythread *)arg;
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, func, arg);
    p->thread_id = tid;
    pthread_attr_destroy(&attr);
    return 0;
}

void process(int fd, short which, void *arg)
{
    mythread *p = (mythread *)arg;
    printf("I am in the thread: [%lu]\n", p->thread_id);

    char buffer[100];
    memset(buffer, 0, 100);

    int ilen = read(fd, buffer, 100);
    printf("read num is: %d\n", ilen);
    printf("the buffer: %s\n", buffer);
}

//设置libevent事件回调
int setup_thread(mythread *p, int fd)
{
    p->base = event_init();
    event_set(&p->notify_event, fd, EV_READ|EV_PERSIST, process, p);
    event_base_set(p->base, &p->notify_event);
    event_add(&p->notify_event, 0);
    return 0;
}

int main()
{
    struct sockaddr_in sin;
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    //在所有IP:19870处监听
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(9877);

    bind(fd, (struct sockaddr*)&sin, sizeof(sin));
    int threadnum = 10; //创建10个线程
    int i;

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
    mythread *g_thread;
    g_thread = (mythread *)malloc(sizeof(mythread)*10);
    for(i=0; i<threadnum; i++)
    { //10个线程都监听同一个socket描述符, 检查是否产生惊群现象?
        setup_thread(&g_thread[i], fd);
    }

    for(i=0; i<threadnum; i++)
    {
        create_worker(worker_libevent, &g_thread[i]);
    }

    //master线程等待worker线程池初始化完全
    pthread_mutex_lock(&init_lock);
    while(init_count < threadnum)
    {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);


    printf("IN THE MAIN LOOP\n");

    while(1)
    {
        sleep(1);
    }

    //没有回收线程的代码
    free(g_thread);
    return 0;
}