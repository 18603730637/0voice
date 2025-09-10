#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#define BUFFER_LEN 1024
#define MAX_CLIENT 2000

struct conn_item{
    int fd;
    char buf[BUFFER_LEN];
    int idx;
};

struct conn_item connlist[MAX_CLIENT];

int main()
{
    int epfd = epoll_create1(0);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct epoll_event event, events[1024];
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2048);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
        perror("bind");
        return -1;
    }

    listen(sockfd, 10);

    event.events = EPOLLIN;
    event.data.fd = sockfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);

    while (1) {
        int nready = epoll_wait(epfd, events, 1024, -1);
        for (int i=0; i<nready; i++) {
            int connfd = events[i].data.fd;
            if (events[i].data.fd == sockfd) {
                if (events[i].events == EPOLLIN) {
                    int clientfd = accept(sockfd, NULL, NULL);
                    event.data.fd = clientfd;
                    event.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &event);
                    connlist[clientfd].fd = clientfd;
                    memset(connlist[clientfd].buf, 0, BUFFER_LEN);
                    connlist[clientfd].idx = 0;
                }
            } else 
            {
                if (events[i].events == EPOLLIN) {
                    char* buf = connlist[connfd].buf;
                    int idx = connlist[connfd].idx;
                    int count = recv(connfd, buf + idx, BUFFER_LEN - idx, 0); 
                    if (count == 0) {
                        close(events[i].data.fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        continue;
                    }
                    connlist[connfd].idx += count;
                    printf("%s\n", buf);
                }
            }
        }
    }
    close(sockfd);
    close(epfd);
}