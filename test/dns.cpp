#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define LOCAL_PORT 53
#define DNS_SERVER_IP "114.114.114.114"
#define DNS_SERVER_PORT 53
#define BUFFER_SIZE 512

#pragma pack(push, 1)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} DNSHeader;

typedef struct {
    uint16_t type;
    uint16_t _class;
} DNSQuestion;

typedef struct {
    uint16_t type;
    uint16_t _class;
    uint32_t ttl;
    uint16_t rdlength;
    // �����Ǳ䳤 rdata
} DNSAnswer;
#pragma pack(pop)

void aur_get(char* buffer) {
    printf("%s\n", buffer);
}


// ���� QNAME ����
int parse_qname(const unsigned char* buffer, int offset, char* out) {
    int pos = offset;
    int jumped = 0;
    int jump_pos = 0;
    char* ptr = out;

    while (1) {
        unsigned char len = buffer[pos];

        if (len == 0) {
            pos++;
            break;
        }

        // ���ѹ��ָ�루����λΪ 11��
        if ((len & 0xC0) == 0xC0) {
            if (!jumped) jump_pos = pos + 2;
            int pointer = ((len & 0x3F) << 8) | buffer[pos + 1];
            pos = pointer;
            jumped = 1;
            continue;
        }

        pos++;
        for (int i = 0; i < len; i++) {
            *ptr++ = buffer[pos++];
        }
        *ptr++ = '.';
    }

    if (ptr != out) *(ptr - 1) = '\0'; // ȥ�����һ����
    else *ptr = '\0';

    return jumped ? jump_pos : pos;
}

// ���� Answer ��
void parse_answers(const unsigned char* buffer, int offset, int ancount) {
    int pos = offset;

    for (int i = 0; i < ancount; i++) {
        char name[256] = { 0 };
        int name_end = parse_qname(buffer, pos, name);
        pos = name_end;

        DNSAnswer* answer = (DNSAnswer*)&buffer[pos];
        uint16_t type = ntohs(answer->type);
        uint16_t rdlen = ntohs(answer->rdlength);
        pos += sizeof(DNSAnswer);

        printf("  [��Ӧ] ����=%d, ����=%s\n", type, name);

        if (type == 1 && rdlen == 4) {  // A��¼
            struct in_addr addr;
            memcpy(&addr, &buffer[pos], 4);
            printf("      A��¼ IP: %s\n", inet_ntoa(addr));
        }
        else if (type == 5) {         // CNAME
            char cname[256] = { 0 };
            parse_qname(buffer, pos, cname);
            printf("      CNAME: %s\n", cname);
        }

        pos += rdlen;
    }
}

int main() {
    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in localAddr, clientAddr, dnsServerAddr;
    int addrLen = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "Socket ����ʧ��\n");
        return 1;
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);

    if (bind(sockfd, (struct sockaddr*) & localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "�˿ڰ�ʧ�ܣ������Թ���Ա�������\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // �������� DNS ��ַ
    memset(&dnsServerAddr, 0, sizeof(dnsServerAddr));
    dnsServerAddr.sin_family = AF_INET;
    dnsServerAddr.sin_port = htons(DNS_SERVER_PORT);
    dnsServerAddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

    printf("[*] DNS �м̷����������������˿� %d\n", LOCAL_PORT);

    while (1) {
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*) & clientAddr, &addrLen);
        if (recvLen == SOCKET_ERROR) continue;

        aur_get(buffer);

        DNSHeader* dns = (DNSHeader*)buffer;

        if (ntohs(dns->qdcount) > 0) {
            char domain[256] = { 0 };
            int qname_end = parse_qname((unsigned char*)buffer, sizeof(DNSHeader), domain);
            DNSQuestion* q = (DNSQuestion*)&buffer[qname_end];

            printf("[����] ID=%04x ��ѯ: %s, ����=%d\n",
                ntohs(dns->id), domain, ntohs(q->type));
        }

        // ת�������� DNS
        sendto(sockfd, buffer, recvLen, 0,
            (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr));

        // ������Ӧ
        int respLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (respLen == SOCKET_ERROR) continue;

        DNSHeader* resp = (DNSHeader*)buffer;

        printf("[��Ӧ] ID=%04x, RCODE=%d, Answer��=%d\n",
            ntohs(resp->id),
            ntohs(resp->flags) & 0x000F,
            ntohs(resp->ancount));

        parse_answers((unsigned char*)buffer, recvLen - (respLen - sizeof(DNSHeader)), ntohs(resp->ancount));

        // ת���ؿͻ���
        sendto(sockfd, buffer, respLen, 0, (struct sockaddr*) & clientAddr, addrLen);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
