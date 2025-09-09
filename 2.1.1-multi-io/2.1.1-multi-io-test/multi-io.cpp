#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/poll.h>

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    struct pollfd fds[1024] = {0};
    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLLIN;
    int maxfd = sockfd;

    while (1) {

        poll(fds, maxfd+1, -1);
        if (fds[sockfd].revents == POLLIN) {

            struct sockaddr_in clientAddr;
            socklen_t len = sizeof(clientAddr);
            int clientfd = accept(sockfd, (struct sockaddr*)&clientAddr, &len);
            if (clientfd != -1) {
                fds[clientfd].fd = clientfd;
                fds[clientfd].events = POLLIN;
                maxfd = clientfd;
            }
        }

        for (int i=sockfd; i < maxfd+1; i++) {
            if (fds[i].revents == POLLIN) {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0) {
                    close(i);
                    fds[i].fd = 0;
                    continue;
                }
                send(i, buffer, count, 0);
                printf("Recv from client:%s\n", buffer);
            }
        }
    }
}