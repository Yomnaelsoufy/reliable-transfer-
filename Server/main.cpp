// Server side implementation of UDP client-server model
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <list>
#include <chrono>
#include <iostream>

using namespace std::chrono;
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
unsigned short port;
double plp;
std::string s;
int sockfd;
auto noOfpacketsent=0;
std::list<packet>lst;
socklen_t len;
struct sockaddr_in servaddr, cliaddr;
std::list<packet> l;
//slow start 1
//congestion avoidance 2
//fast recovery 3
int cwnd=1, ssthred=8, dupAckcount=0,state=1,i=0;
int seq;
void read_file(std::string fname){
    std::ifstream f(fname.c_str());
    if(f){
        std::ostringstream ss;
        ss<<f.rdbuf();
        s=ss.str();
    }
}
void threedACK(){
    state=3;
    ssthred=cwnd/2;
    int lstlen=lst.size();
    while (lstlen!=0){lst.pop_back();i-=5000;lstlen--;}
    cwnd=ssthred+3;
}
void timeout(){
        struct timeval timer;
        fd_set  timeout;
        timer.tv_usec=0;
        timer.tv_sec=3;
        FD_ZERO(&timeout);
        FD_SET(sockfd,&timeout);
        int rdy;
        rdy=select(sockfd+1,&timeout, nullptr, nullptr,&timer);
        if (rdy<1){
            sendto(sockfd,&l.front(), sizeof(l.front()),
                   0,(const struct sockaddr*)&cliaddr, sizeof(cliaddr));
                           noOfpacketsent++;
            int size=lst.size();
            while (size>1){i-=l.back().len;lst.pop_back();size--;}
            ssthred=cwnd/2;
            cwnd=1;
            dupAckcount=0;
            state=1;
        }
}
void stopandwait(double plp){
    for(int i=0;i<s.length();i+=5000) {
        packet *pkt= static_cast<packet *>(malloc(sizeof(struct packet)));
        if(i==0){
            pkt->startedsequence=seq;
        }
        else pkt->startedsequence=-1;
        pkt->seqno=seq++;
        if(i+5000<s.length()){
            pkt->end= false;
            strcpy(pkt->data,s.substr(i,5000).c_str());}
        else {pkt->end= true;
            strcpy(pkt->data,s.substr(i,s.length()-i).c_str());
        }

        if(((rand()%10)*100*plp)==0){}
        else {
            noOfpacketsent++;
            sendto(sockfd, pkt, sizeof(*pkt),
                0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));}
        //receive ack
        ack_packet *ackpkt= static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
        while (true){
        struct timeval timer;
        fd_set  timeout;
        timer.tv_usec=0;
        timer.tv_sec=2;
        FD_ZERO(&timeout);
        FD_SET(sockfd,&timeout);
        int rdy;
        rdy= select(sockfd+1,&timeout,nullptr, nullptr,&timer);
        if (rdy<1){
            noOfpacketsent++;
            sendto(sockfd,pkt, sizeof (*pkt),
                   0,(const struct sockaddr*)&cliaddr, sizeof(cliaddr));
        }
        else break;}
        if(pkt->end)break;
        recvfrom(sockfd, ackpkt, sizeof(*ackpkt),
                 0, (struct sockaddr *) &cliaddr,
                 &len);
        if(seq-1>ackpkt->ackno){
            i-=5000;
            seq--;
        }
    }
}
void Go_backN(double plp){
    while (i<s.length())
     {
        for(int j=0;j<cwnd;j++){
            if (i >= s.length()) break;
            if(cwnd==lst.size())break;
            packet *pkt= static_cast<packet *>(malloc(sizeof(struct packet)));
            if(i==0){
                pkt->startedsequence=seq;
            }
            else pkt->startedsequence=-1;
            pkt->seqno=seq++;
            if(i+5000<s.length()){
                pkt->end= false;
                pkt->len=5000;
                strcpy(pkt->data,s.substr(i,5000).c_str());
                i+=5000;}
            else {
                pkt->end= true;
                pkt->len=s.length()-i;
                strcpy(pkt->data,s.substr(i,s.length()-i).c_str());
                i = s.length();
            }
            l.push_back(*pkt);
            double dob= plp*(rand()%10)*100;
            if((dob)==0){
                continue;
            }
            else {sendto(sockfd, pkt, sizeof(*pkt),
                   0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
                noOfpacketsent++;}

        }
        if(state==1){
            timeout();
            ack_packet *ackpkt= static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
            int firstseqinlst=l.front().seqno;
            recvfrom(sockfd, ackpkt, sizeof(*ackpkt),
                     0, (struct sockaddr *) &cliaddr,
                     &len);
            if(firstseqinlst>ackpkt->ackno){
                dupAckcount++;
                if (dupAckcount==3){
                    threedACK();
                }
            }
        else{
            cwnd+=1;
            dupAckcount=0;
        }
    }
        else if(state==2){//congestion avoidance
            //newACK
            timeout();
            ack_packet *ackpkt= static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
                 int firstseqinlst=l.front().seqno;
                   recvfrom(sockfd, ackpkt, sizeof(*ackpkt),
                      0, (struct sockaddr *) &cliaddr,
                      &len);
            if(firstseqinlst>ackpkt->ackno){
                dupAckcount++;
                if (dupAckcount==3){
                    threedACK();
                }
            }
            else{
                cwnd+=(1/cwnd);
                dupAckcount=0;
            }
        }
        else{//fastrecovery
            timeout();
            ack_packet *ackpkt= static_cast<ack_packet *>(malloc(sizeof(struct ack_packet)));
            int firstseqinlst=l.front().seqno;
            recvfrom(sockfd, ackpkt, sizeof(*ackpkt),
                     0, (struct sockaddr *) &cliaddr,
                     &len);
            if(firstseqinlst>ackpkt->ackno){
                cwnd+=1;
            }
            else{
                cwnd=ssthred;
                dupAckcount=0;
                state=2;
            }
        }
        if(i==s.length()&&!l.empty()){
            int lstlen=lst.size();
            while (lstlen!=0){
                lst.pop_back();
                i-=5000;
            }
        }
    }
    close(sockfd);
}
void init(){
    std::ifstream fi("server.in");
    std::string val;
    getline(fi,val);
    port=(unsigned short ) stoul(val,NULL,0);
    int random;
    getline(fi,val);
    random = stoi(val);
    getline(fi,val);
    plp= stod(val);
    srand(random);
    seq=rand()%10;
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
              sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}
int main() {
    init();
    packet * p= static_cast<packet *>(malloc(sizeof(struct packet)));
    len = sizeof(cliaddr);  //len is value/result
    recvfrom(sockfd, p, sizeof(*p)
            ,0, ( struct sockaddr *) &cliaddr,
                 &len);
    std::string sss=p->data;
    read_file(sss);
    using clock=std::chrono::system_clock;
    using sec=std::chrono::duration<double>;
    const auto st=clock ::now();
    //  Go_backN(plp);
     stopandwait(plp);
        const sec tim =clock ::now()-st;
        std::cout<<double (noOfpacketsent)/tim.count()<<std::endl;
    }