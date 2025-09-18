#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define BUFFER_LEN 1024
#define MAX_CLIENT 4096

int epfd = 0;

typedef void (*CALLBACK) (int);

struct conn_item{
    int fd;
    char rbuffer[BUFFER_LEN];
    int rbuflen;
    char wbuffer[BUFFER_LEN];
    int wbuflen;
    union Reactor
    {
        CALLBACK accept_cb;
        CALLBACK recv_cb;
    }recv_t;
    CALLBACK send_cb;
};

struct conn_item connlist[MAX_CLIENT];
typedef struct conn_item connection_t;


void accept_cb(int listenfd);
void recv_cb(int connfd);
void send_cb(int connfd);

int http_response(connection_t* conn) {
    // int len = sprintf(conn->wbuffer, 
    //     "HTTP/1.1 200 OK\r\n"
    //     "Accept-Ranges: bytes\r\n"
    //     "Content-Length:82\r\n"
    //     "Content-Type:text/html\r\n"
    //     "Date:Sat,06 Aug 2022 13:16:46 GMT\r\n\r\n"
    //     "<html><head><title>0voice.king</title></head><body><h1>King</h1></body></html>\r\n\r\n");
    // return len;

    int filefd = open("test.html", O_RDONLY);
    struct stat file_stat;
    fstat(filefd, &file_stat);
    off_t length = file_stat.st_size;

    conn->wbuflen = sprintf(conn->wbuffer, 
        "HTTP/1.1 200 OK\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length:%ld\r\n"
        "Content-Type:text/html\r\n"
        "Date:Sat,06 Aug 2022 13:16:46 GMT\r\n\r\n", length);
    int count = read(filefd, conn->wbuffer + conn->wbuflen, length);
    conn->wbuflen += count;
}

void set_event(int fd, int event, int flag) {
    if(flag) {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = event;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    } else 
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = event;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}

void accept_cb(int listenfd) {

    int connfd = accept(listenfd, NULL, NULL);
    connlist[connfd].fd = connfd;
    memset(connlist[connfd].rbuffer, 0, sizeof(BUFFER_LEN));
    connlist[connfd].rbuflen = 0;
    memset(connlist[connfd].wbuffer, 0, BUFFER_LEN);
    connlist[connfd].wbuflen = 0;
    connlist[connfd].recv_t.recv_cb = recv_cb;
    connlist[connfd].send_cb = send_cb;

    set_event(connfd, EPOLLIN, 1);
}

void recv_cb(int connfd) {
    char* buf = connlist[connfd].rbuffer;
    int idx = connlist[connfd].rbuflen;
    int count = recv(connfd, buf + idx, BUFFER_LEN - idx, 0);
    if (count == 0) {
        close (connfd);

    }
    connlist[connfd].rbuflen += count;
    // printf("recv <-- %s\n", buf);
    memcpy(connlist[connfd].wbuffer, buf, connlist[connfd].rbuflen);
    connlist[connfd].wbuflen = connlist[connfd].rbuflen;
    connlist[connfd].rbuflen -= connlist[connfd].rbuflen;

    // int len = http_response(&connlist[connfd]);
    // connlist[connfd].wbuflen = len;
    set_event(connfd, EPOLLOUT, 0);
}

void send_cb(int connfd) {
    send(connfd, connlist[connfd].wbuffer, connlist[connfd].wbuflen, 0);
    // printf ("send --> %s\n", connlist[connfd].wbuffer);

    set_event(connfd, EPOLLIN, 0);
}

int Sock_Init() {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct epoll_event event, events[BUFFER_LEN];
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2048);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
        perror("bind");
        return -1;
    }

    listen(sockfd, MAX_CLIENT);

    connlist[sockfd].recv_t.accept_cb = accept_cb;
    set_event(sockfd, EPOLLIN, 1);
}


int main()
{
    epfd = epoll_create1(0);
    struct epoll_event events[MAX_CLIENT];
    Sock_Init();

    while (1) {
        int nready = epoll_wait(epfd, events, MAX_CLIENT, -1);
        for (int i=0; i<nready; i++) {
            int fd = events[i].data.fd;
            if (events[i].events == EPOLLIN) {
                connlist[fd].recv_t.recv_cb(fd);
            } else if (events[i].events == EPOLLOUT) {
                connlist[fd].send_cb(fd);
            }
        }
    }
}