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

typedef struct Question
{
    char qname[256];
    uint16_t qtype;
    uint16_t qclass;
}DNSQuestion;

typedef struct {
    uint16_t type;
    uint16_t _class;
    uint32_t ttl;
    uint16_t rdlength;
    // �����Ǳ䳤 rdata
} DNSAnswer;
#pragma pack(pop)

void print_hex(const unsigned char* data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}


uint16_t get16bit(uint8_t** buf)
{
    uint16_t rec;
    memcpy(&rec, *buf, 2);
    *buf = *buf + 2;
    return ntohs(rec);
}

char* getquestion(Question* question, char buf[])
{
    int i = 0;
    while (*buf != 0)
    {
        question->qname[i] = *buf;
        buf++;
        i++;
    }
    question->qname[i] = 0;
    buf++;
    question->qtype = get16bit((uint8_t**)&buf);
    question->qclass = get16bit((uint8_t**)&buf);
    return buf;
}

char* getName(char qname[]) {
    int index = 1;
    int realIndex = 0;
    char* realName = (char*)malloc(256);
    memset(realName, 0, 256);
    while(qname[index]!=0){
        if (qname[index] >= '0' && qname[index] <= 'z') {
            realName[realIndex] = qname[index];
            realIndex++;
        }
        else {
            realName[realIndex] = '.';
            realIndex++;
        }
        index++;
    }
    return realName;
}

//todo
int* search(char* realName) {
    return NULL;
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
        fprintf(stderr, "�˿ڰ�ʧ��\n");
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

        //TODO ֻ��QUERYS�ֶο�ʼ
        printf("get\n");
        printf("recvLen: %d \n", recvLen);
        print_hex((const unsigned char*)buffer, recvLen);



        DNSHeader* dns = (DNSHeader*)buffer;

        printf("��ͷid��%u    ", ntohs(dns->id));
        printf("Դip��ַ��%s\n", inet_ntoa(clientAddr.sin_addr));
        
        DNSQuestion question;
        uint8_t* offset = (uint8_t*)getquestion(&question, buffer + 12);

        printf("name: %s,type: %d,class: %d", question.qname,question.qtype,question.qclass);

        //ֻ����A
        if (question.qtype == 1) {
            char* name = getName(question.qname);
            printf("realName: %s\n", name);
            int* searchIp = search(name);






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
        print_hex((const unsigned char*)buffer, respLen);


        // ת���ؿͻ���
        sendto(sockfd, buffer, respLen, 0, (struct sockaddr*) & clientAddr, addrLen);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}



