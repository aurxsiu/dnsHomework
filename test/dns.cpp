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
    uint32_t ttl;
    uint16_t rd_length;
    uint32_t rdata;
}ResourceRecord;

//接收请求,获取id,用id为序,保存MeLo,修改id为管理版本,转发,
typedef struct MessageLog {
    struct sockaddr_in clientAddress;
    uint16_t id;
};

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

void aurPrint(const char* promt,char* str) {
    printf("%s: %s\n", promt, str);
}

uint32_t lookup_ip_as_int(char* domain) {
    const char* filename = "C:\\Users\\aurxsiu\\Desktop\\dns.txt";
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
                printf("search result: %s", ip_str);
                uint32_t ip = inet_addr(ip_str);
                return ip;
            }
        }
    }

    fclose(file);
    return -1;
}



typedef struct LOG {
    uint32_t ip;
    char* domain;
}LOG;
LOG* logs = (LOG*)malloc(65536 * sizeof(LOG));
int size = 0;
void add_log(uint32_t ip,char* domain) {
    LOG* new_log = (LOG*)malloc(sizeof(LOG));
    new_log->ip = ip;
    new_log->domain = domain;
    logs[size] = *new_log;
    size++;
    return;
}
uint32_t get_ip(char* domain) {
    for (int a = 0; a < size; a++) {
        LOG log = logs[a];
        if (strcmp(domain, log.domain) == 0) {
            return log.ip;
        }
    }
    return -1;
}

uint16_t current_id = 0;
struct MessageLog* mlogs = (MessageLog*)malloc(65536 * sizeof(MessageLog));
uint16_t add_current_id() {
    current_id++;
    if (current_id == 256) {
        current_id = 0;
    }
    return current_id;
}


//id为local_id
void sendToDns(SOCKET socket,char* buffer,int len,int flags,const sockaddr* to,int tolen,uint16_t id, sockaddr_in client,DNSHeader* header) {
    printf("currentManagedId: %d\n", current_id);
    uint16_t get_id = add_current_id();
    struct MessageLog new_message_log;
    new_message_log.clientAddress = client; new_message_log.id = id;
    mlogs[get_id] = new_message_log;
    header->id = get_id;
    sendto(socket, buffer, len, flags, to, tolen);
    printf("发送完毕");
}

//id为dns_id || managed_id
void sendToLocalFromDns(SOCKET socket, char* buffer, int len, int flags,DNSHeader* header,uint16_t id){
    struct MessageLog get_message_log = mlogs[id];
    header->id = get_message_log.id;

    sendto(socket, buffer, len, flags, (struct sockaddr*)&(get_message_log.clientAddress), sizeof(get_message_log.clientAddress));
}

int main() {
    struct sockaddr_in* log = (struct sockaddr_in*)malloc(65536 * sizeof(struct sockaddr_in));


    WSADATA wsaData;
    SOCKET sockfd;
    struct sockaddr_in localAddr, clientAddr, dnsServerAddr,acceptAddr;
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

    memset(&acceptAddr, 0, sizeof(acceptAddr));
    acceptAddr.sin_family = AF_INET;
    acceptAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptAddr.sin_port = htons(LOCAL_PORT);

    if (bind(sockfd, (struct sockaddr*) & acceptAddr, sizeof(acceptAddr)) == SOCKET_ERROR) {
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
        if (strcmp("10.29.221.78", inet_ntoa(clientAddr.sin_addr)) == 0) {
            continue;   
        }
        if (strcmp("114.114.114.114", inet_ntoa(clientAddr.sin_addr)) == 0) {//默认通过114.114.114.114接收响应
            printf("get message from %s\n", inet_ntoa(clientAddr.sin_addr));

            DNSHeader* resp = (DNSHeader*)buffer;

            printf("[响应] ID=%04x, RCODE=%d, Answer数=%d\n",
                ntohs(resp->id),
                ntohs(resp->flags) & 0x000F,
                ntohs(resp->ancount));
            //print_hex((const unsigned char*)buffer, recvLen);

            DNSQuestion question;
            uint8_t* offset = (uint8_t*)getquestion(&question, buffer + 12);

            uint16_t record_num = ntohs(resp->ancount);
            ResourceRecord* record = (ResourceRecord*)offset;
            for(int a = 0;a<record_num;a++){
                printf("record%d:\n", a);
                uint16_t type = ntohs(record->type);
                printf("record type: %d\n", type);
                if (type == 1) {
                    uint32_t ip = record->rdata;
                    //printf("ip: %d\n", ip);
                    char* name = getName(question.qname);
                    //printf("realName: %s\n", name);
                    char* nameCopy = (char*)malloc(30 * sizeof(char));
                    strcpy(nameCopy, name);
                    add_log(ip, nameCopy);
                    printf("存储ip-domain: %d-%s\n", ip, name);
                }

                uint16_t* len_p = (uint16_t*)(&(record->rd_length));
                uint16_t len = get16bit((uint8_t**)(&len_p));
                //printf("record len: %d\n", len);
                record = (ResourceRecord*)((uint8_t*)record + 16 + (len - 4));
            }

   
            
            
            
                
            struct sockaddr_in to_return = log[resp->id];

            //// 转发回客户端
            //sendto(sockfd, buffer, recvLen, 0, (struct sockaddr*) & to_return, sizeof(to_return));
            
            sendToLocalFromDns(sockfd, buffer, recvLen, 0, resp, resp->id);
            printf("发送回客户端\n");
        }
        else {
            printf("get message from local %s\n", inet_ntoa(clientAddr.sin_addr));
            printf("recvLen: %d \n", recvLen);
            //print_hex((const unsigned char*)buffer, recvLen);



            DNSHeader* dns = (DNSHeader*)buffer;

            printf("报头id：%u    ", ntohs(dns->id));
            printf("源ip地址：%s %d\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);

            

            DNSQuestion question;
            uint8_t* offset = (uint8_t*)getquestion(&question, buffer + 12);

            char* name = getName(question.qname);
            printf("realName: %s\n", name);
            printf("type: %d\n", question.qtype);

            //只处理A
            if (question.qtype == 1) {

                uint32_t searchIp = lookup_ip_as_int(name);


                if (searchIp == -1) {
                    searchIp = get_ip(name);
                }

                if ((int)searchIp != 0&&(int)searchIp != -1) {
                    printf("查询到地址,使用本地文件返回响应\n");

                    //flags不管
                    dns->ancount = ntohs(1);
                    dns->flags = dns->flags + 128;

                    static const int A_NAME_OFFSET = 0xC00C;
                    static const uint16_t A_TYPE = 1;
                    static const uint16_t IN_CLASS = 1;
                    static const uint16_t A_RDLENGTH = 4;
                    static const uint32_t ttl = 86400;

                    ResourceRecord answer;
                    answer.name = htons(A_NAME_OFFSET);
                    answer.type = htons(A_TYPE);
                    answer.rrclass = htons(IN_CLASS);
                    *(uint32_t*)&answer.ttl = htonl(ttl);
                    answer.rd_length = htons(A_RDLENGTH);
                    answer.rdata = searchIp;
                    printf("%d\n", sizeof(answer));
                    memcpy(offset, (uint8_t*)(&answer), sizeof(answer));
                    int new_len = recvLen + sizeof(answer);

                    print_hex((const unsigned char*)buffer, new_len);

                    sendto(sockfd, buffer, new_len, 0, (struct sockaddr*) & clientAddr, addrLen);
                    continue;
                }
                else {
                    if (searchIp == 0) {
                        printf("查询到0.0.0.0\n");
                        dns->flags += 768;
                        dns->flags += 128;
                        print_hex((const unsigned char*)buffer, recvLen);
                        sendto(sockfd, buffer, recvLen, 0, (struct sockaddr*) & clientAddr, addrLen);


                        continue;
                    }
                }
            }

            printf("未在内存中或文件中查询到ip,查询dns\n");
            // 转发到上游 DNS
            /*sendto(sockfd, buffer, recvLen, 0,
                (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr));*/
            sendToDns(sockfd, buffer, recvLen, 0, (struct sockaddr*) & dnsServerAddr, sizeof(dnsServerAddr), dns->id, clientAddr, dns);

            continue;
        }
       
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}



