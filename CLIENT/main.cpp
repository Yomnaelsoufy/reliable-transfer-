#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include <arpa/inet.h>
#include <iostream>

int sockfd;
std::string s;
struct sockaddr_in servaddr;
socklen_t len;
struct packet{
    bool end;
        uint16_t len;
        uint16_t startedsequence;
        uint32_t seqno;
        char data[5000];
                };
struct ack_packet{
        uint16_t len;
        uint32_t ackno;
                    };
void writeinfile(std::string tmp){
    std::ofstream MyFile(tmp);
    MyFile << s;
}
void Go_backN(){
    int expected=1;
    ack_packet * ack = (struct ack_packet*)(malloc(sizeof(struct ack_packet)));
    struct timeval timer;
    fd_set  timeout;
    while (true) {
        packet *cpkt2= (struct packet*)(malloc(sizeof(struct packet)));
        struct packet *cpkt= (struct packet*)(malloc(sizeof(struct packet)));
        recvfrom(sockfd, cpkt, sizeof(*cpkt),
                 0,    (struct sockaddr *) &servaddr,
                 &len);
        if(cpkt->startedsequence!=65535)
            expected=cpkt->startedsequence;
        s+=cpkt->data;
        ack_packet * a1 = (struct ack_packet*)(malloc(sizeof(struct ack_packet)));
        if (cpkt->seqno!=expected)
        {
            sendto(sockfd, ack, sizeof (*ack),
                   0, (const struct  sockaddr*)&servaddr, sizeof(servaddr));}
        else    {
            a1->ackno=cpkt->seqno;

            timer.tv_sec=1;
            timer.tv_usec=0;
            FD_ZERO(&timeout);
            FD_SET(sockfd,&timeout);
            int rdy;
            expected++;
            rdy=select(sockfd+1,&timeout, nullptr, nullptr,&timer);
            if (rdy<1){
                sendto(sockfd,a1, sizeof(*a1),
                       0,(const struct sockaddr*)&servaddr, sizeof(servaddr));
            }
            else{
            recvfrom(sockfd, cpkt2, sizeof(*cpkt2),
                     0, (struct sockaddr *) &servaddr,
                     &len);
            if (cpkt2->seqno==expected){
                s+=cpkt2->data;
                a1->ackno=expected;expected++;
            }
            sendto(sockfd,a1, sizeof (*a1),
                                    0,(const struct  sockaddr*)&servaddr, sizeof(servaddr));

        }}
        ack->ackno=a1->ackno;
        if (cpkt->end ==true|| cpkt2->end==true)break;
    }
}
std::string tmp;
void stopandwait(){
    int expected=1;
    ack_packet * ack = static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
    while (true) {
        packet *cpkt = static_cast<packet *>(malloc(sizeof(struct packet)));
        recvfrom(sockfd, cpkt, sizeof(*cpkt),
                 0, (struct sockaddr *) &servaddr,
                 &len);
        if(cpkt->startedsequence!=65535)
            expected=cpkt->startedsequence;
        s+=cpkt->data;
        ack_packet * a1 = static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
        a1->ackno=cpkt->seqno;
        if (cpkt->seqno!=expected)
        {
            sendto(sockfd,ack, sizeof (*ack),
                   0,(const struct  sockaddr*)&servaddr, sizeof(servaddr));
        }
        else    {
            expected++;
            sendto(sockfd,a1, sizeof (*a1),
                   0,(const struct  sockaddr*)&servaddr, sizeof(servaddr));}
        if (cpkt->end==true)
            break;
        ack=a1;

    }
}
void init(){
    std::ifstream fi("client.in");
    const char *v;
    std::string val;
    getline(fi,val);
    struct sockaddr_in sa;
    v=val.c_str();
    inet_pton(AF_INET,v,&(servaddr.sin_addr.s_addr));
    getline(fi,val);
    unsigned short port=(unsigned short ) stoul(val,NULL,0);
    getline(fi,val);
     tmp = val.c_str();
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    // Filling server information
    //servaddr.sin_family =&(sa);
    servaddr.sin_port = htons(port);
   // servaddr.sin_addr.s_addr = INADDR_ANY;
}
int main() {
    init();
    packet *p = static_cast<packet *>(malloc((sizeof(struct packet))));
    strcpy(p->data, tmp.c_str());
    sendto(sockfd, p, sizeof(*p),
           0, (const struct sockaddr *) &servaddr,
           sizeof(servaddr));
    ////////////////////////////////////////////////////////////
    // Go_backN();
   stopandwait();
    writeinfile(tmp);
    close(sockfd);
    return 0;
}