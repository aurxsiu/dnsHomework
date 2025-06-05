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

typedef struct ResourceRecord
{
    uint16_t name;
    uint16_t type;
    uint16_t rrclass;
    uint16_t ttl;
    uint16_t rd_length;
    uint32_t rdata;
}ResourceRecord;
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

//todo ip转为10进制
int search(char* realName) {
    return NULL;
}

void aurPrint(const char* promt,char* str) {
    printf("%s: %s\n", promt, str);
}

int lookup_ip_as_int(char* domain) {
    /*const char* filename = "";
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char ip_str[64];
        char name[256];

        if (line[0] == '#' || line[0] == '\n') continue;

        if (sscanf(line, "%s %s", ip_str, name) == 2) {
            if (strcmp(name, domain) == 0) {
                uint32_t ip = inet_addr(ip_str);
                return ip;
            }
        }
    }

    fclose(file);
    return -1;*/
    return -1;
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
        fprintf(stderr, "Socket 创建失败\n");
        return 1;
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    localAddr.sin_port = htons(LOCAL_PORT);

    if (bind(sockfd, (struct sockaddr*) & localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "端口绑定失败\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // 设置上游 DNS 地址
    memset(&dnsServerAddr, 0, sizeof(dnsServerAddr));
    dnsServerAddr.sin_family = AF_INET;
    dnsServerAddr.sin_port = htons(DNS_SERVER_PORT);
    dnsServerAddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);



    printf("[*] DNS 中继服务器启动，监听端口 %d\n", LOCAL_PORT);

    while (1) {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        printf("\n");
        int recvLen = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
            (struct sockaddr*) & clientAddr, &addrLen);
        if (recvLen == SOCKET_ERROR) continue;

        if (strcmp("127.0.0.1", inet_ntoa(clientAddr.sin_addr)) == 0) {
            printf("get message from local\n");
            printf("recvLen: %d \n", recvLen);
            print_hex((const unsigned char*)buffer, recvLen);



            DNSHeader* dns = (DNSHeader*)buffer;

            printf("报头id：%u    ", ntohs(dns->id));
            printf("源ip地址：%s\n", inet_ntoa(clientAddr.sin_addr));

            DNSQuestion question;
            uint8_t* offset = (uint8_t*)getquestion(&question, buffer + 12);

            char* name = getName(question.qname);
            printf("realName: %s\n", name);

            //只处理A
            //if (question.qtype == 1) {

            //    int searchIp = lookup_ip_as_int(name);

            //    printf("search_ip: %d", searchIp);



            //    if (searchIp > 0) {
            //        printf("使用本地文件返回响应\n");

            //        //flags不管
            //        dns->ancount = ntohs(1);

            //        static const int A_NAME_OFFSET = 0xC00C;
            //        static const uint16_t A_TYPE = 1;
            //        static const uint16_t IN_CLASS = 1;
            //        static const uint16_t A_RDLENGTH = 4;
            //        static const uint32_t ttl = 86400;

            //        ResourceRecord answer;
            //        answer.name = htons(A_NAME_OFFSET);
            //        answer.type = htons(A_TYPE);
            //        answer.rrclass = htons(IN_CLASS);
            //        *(uint32_t*)&answer.ttl = htonl(ttl);
            //        answer.rd_length = htons(A_RDLENGTH);
            //        answer.rdata = searchIp;
            //        memcpy(offset, (uint8_t*)(&answer), 16);
            //        int new_len = recvLen + 16;

            //        print_hex((const unsigned char*)buffer, new_len);

            //        sendto(sockfd, buffer, new_len, 0, (struct sockaddr*) & clientAddr, addrLen);
            //        continue;
            //    }
            //}

            // 转发到上游 DNS
            sendto(sockfd, buffer, recvLen, 0,
                (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr));
            continue;
        }
        else {//默认通过114.114.114.114接收响应
            printf("get message from %s\n", inet_ntoa(clientAddr.sin_addr));

            DNSHeader* resp = (DNSHeader*)buffer;

            printf("[响应] ID=%04x, RCODE=%d, Answer数=%d\n",
                ntohs(resp->id),
                ntohs(resp->flags) & 0x000F,
                ntohs(resp->ancount));
            print_hex((const unsigned char*)buffer, recvLen);


            // 转发回客户端
            sendto(sockfd, buffer, recvLen, 0, (struct sockaddr*) & localAddr, sizeof(localAddr));
        }
       
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}



