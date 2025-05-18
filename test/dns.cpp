#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // ���� Winsock ��

#define LOCAL_PORT 53             // ���ؼ����˿�
#define DNS_SERVER_IP "10.3.9.5"     // ���� DNS ������ IP
#define DNS_SERVER_PORT 53          // ���� DNS �˿�
#define BUFFER_SIZE 512             // DNS ���ݱ���󳤶�

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in localAddr, clientAddr, dnsServerAddr;
    int addrLen = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    // ��ʼ�� Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        fprintf(stderr, "WSAStartup ʧ��\n");
        return 1;
    }

    // ���� UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "socket ����ʧ��\n");
        WSACleanup();
        return 1;
    }

    // ���ñ��ؼ�����ַ
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);

    // �󶨱��ض˿�
    if (bind(sockfd, (struct sockaddr*) & localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "�󶨶˿�ʧ��\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("DNS �м̷����������������˿� %d\n", LOCAL_PORT);

    while (1) {
        // ���տͻ�������
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*) & clientAddr, &addrLen);
        if (recvLen == SOCKET_ERROR) {
            fprintf(stderr, "����ʧ��\n");
            continue;
        }
        printf("��������");

        // �������� DNS ��������ַ
        memset(&dnsServerAddr, 0, sizeof(dnsServerAddr));
        dnsServerAddr.sin_family = AF_INET;
        dnsServerAddr.sin_port = htons(DNS_SERVER_PORT);
        dnsServerAddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

        // ������ת�������� DNS
        if (sendto(sockfd, buffer, recvLen, 0,
            (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr)) == SOCKET_ERROR) {
            fprintf(stderr, "ת������ʧ��\n");
            continue;
        }

        // �������� DNS ��Ӧ
        int respLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (respLen == SOCKET_ERROR) {
            fprintf(stderr, "����������Ӧʧ��\n");
            continue;
        }

        // ���ظ��ͻ���
        if (sendto(sockfd, buffer, respLen, 0,
            (struct sockaddr*) & clientAddr, addrLen) == SOCKET_ERROR) {
            fprintf(stderr, "���ؿͻ���ʧ��\n");
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
