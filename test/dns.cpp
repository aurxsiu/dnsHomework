#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // 链接 Winsock 库

#define LOCAL_PORT 53             // 本地监听端口
#define DNS_SERVER_IP "10.3.9.5"     // 上游 DNS 服务器 IP
#define DNS_SERVER_PORT 53          // 上游 DNS 端口
#define BUFFER_SIZE 512             // DNS 数据报最大长度

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in localAddr, clientAddr, dnsServerAddr;
    int addrLen = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        fprintf(stderr, "WSAStartup 失败\n");
        return 1;
    }

    // 创建 UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "socket 创建失败\n");
        WSACleanup();
        return 1;
    }

    // 设置本地监听地址
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);

    // 绑定本地端口
    if (bind(sockfd, (struct sockaddr*) & localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "绑定端口失败\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("DNS 中继服务器启动，监听端口 %d\n", LOCAL_PORT);

    while (1) {
        // 接收客户端请求
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*) & clientAddr, &addrLen);
        if (recvLen == SOCKET_ERROR) {
            fprintf(stderr, "接收失败\n");
            continue;
        }
        printf("接收请求");

        // 配置上游 DNS 服务器地址
        memset(&dnsServerAddr, 0, sizeof(dnsServerAddr));
        dnsServerAddr.sin_family = AF_INET;
        dnsServerAddr.sin_port = htons(DNS_SERVER_PORT);
        dnsServerAddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

        // 将请求转发给上游 DNS
        if (sendto(sockfd, buffer, recvLen, 0,
            (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr)) == SOCKET_ERROR) {
            fprintf(stderr, "转发上游失败\n");
            continue;
        }

        // 接收上游 DNS 响应
        int respLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (respLen == SOCKET_ERROR) {
            fprintf(stderr, "接收上游响应失败\n");
            continue;
        }

        // 返回给客户端
        if (sendto(sockfd, buffer, respLen, 0,
            (struct sockaddr*) & clientAddr, addrLen) == SOCKET_ERROR) {
            fprintf(stderr, "返回客户端失败\n");
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
