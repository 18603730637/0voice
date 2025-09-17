#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define SERVER_IP "192.168.243.131"  // 服务器IP（请替换为实际地址）
#define SERVER_PORT 2048             // 服务器端口（请替换为实际端口）
#define MAX_CONNECTIONS 1000         // 目标最大连接数（可按需调整）
#define MAX_EPOLL_EVENTS 1024        // epoll一次处理的最大事件数

int main() {
    int epoll_fd, sock_fd, conn_count = 0;
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    struct sockaddr_in server_addr;
    time_t start_time, end_time;
    double time_used;

    // 记录测试开始时间
    start_time = time(NULL);

    // 创建epoll实例（用于高效管理多连接的IO事件）
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return EXIT_FAILURE;
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed (invalid IP)");
        close(epoll_fd);
        return EXIT_FAILURE;
    }

    // 循环创建连接，直到达到目标连接数
    while (conn_count < MAX_CONNECTIONS) {
        // 1. 创建TCP socket
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            perror("socket creation failed");
            continue;  // 跳过本次，尝试下一个连接
        }

        // 2. 设置socket为非阻塞（高并发下避免connect阻塞）
        int flags = fcntl(sock_fd, F_GETFL, 0);
        if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl set nonblock failed");
            close(sock_fd);
            continue;
        }

        // 3. 发起非阻塞连接
        if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            // 非阻塞connect时，EINPROGRESS表示“连接正在进行中”，属于正常情况
            if (errno != EINPROGRESS) {
                perror("connect failed (not EINPROGRESS)");
                close(sock_fd);
                continue;
            }
        }

        // 4. 将socket加入epoll（监控“可写事件”，判断连接是否建立完成）
        event.events = EPOLLOUT;  // 关注“写事件”（非阻塞connect完成后会触发）
        event.data.fd = sock_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event) == -1) {
            perror("epoll_ctl add failed");
            close(sock_fd);
            continue;
        }

        conn_count++;

        // 可选：每创建100个连接，打印一次进度
        if (conn_count % 100 == 0) {
            printf("[Progress] Created %d connections\n", conn_count);
        }
    }

    // 记录测试结束时间，计算耗时
    end_time = time(NULL);
    time_used = difftime(end_time, start_time);

    // 输出最终统计信息
    printf("connections: %d, epoll_fd: %d, time_used: %.2f seconds\n", 
           conn_count, epoll_fd, time_used);

    // （可选）清理资源：关闭所有socket和epoll
    // 注意：若要持续压测服务器，可注释这部分，保持连接
    for (int i = 0; i < MAX_EPOLL_EVENTS; i++) {
        if (events[i].data.fd > 0) {
            close(events[i].data.fd);
        }
    }
    close(epoll_fd);

    return 0;
}
