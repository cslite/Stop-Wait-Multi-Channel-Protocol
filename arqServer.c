/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#include "packet.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>
#include "config.h"

int nextRequiredOffset;
int nextOOOoffset;
int lastPktOffset;
char oooBuffer[OOO_BUFFER_SIZE+5];
int outputFd;
bool receivedLastPkt;
bool pendingLastPktWrite;

void initServerGlobals(){
    nextRequiredOffset = 0;
    nextOOOoffset = 0;
    lastPktOffset = -1;
    receivedLastPkt = false;
    pendingLastPktWrite = true;
    srand(time(0));
}

bool prepareServerSocket(int *sock, struct sockaddr_in *serverAddr){
    *sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock[0] < 0){
        fprintf(stderr,"prepareServerSocket: Error in opening socket.\n");
        return false;
    }
    memset(serverAddr,0,sizeof(*serverAddr));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons(SERVER_PORT);
    serverAddr->sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(*sock,(struct sockaddr*)serverAddr,sizeof(*serverAddr)) < 0){
        fprintf(stderr,"prepareServerSocket: Error in binding server socket.\n");
        return false;
    }

    if(listen(*sock,2) < 0){
        fprintf(stderr,"prepareServerSocket: Error in listen.\n");
        return false;
    }

    return true;
}

bool loadDataOnBuffer(packet *pkt){
    if(pkt == NULL)
        return false;
    if(((pkt->seq) - nextRequiredOffset) >= OOO_BUFFER_SIZE){
        //This is outside buffer limits, need to drop this packet
        return false;
    }
    int len = pkt->size;
    for(int i=0; i<len; i++){
        oooBuffer[(pkt->seq) - nextRequiredOffset + i] = (pkt->payload)[i];
    }
    return true;
}


bool takePacketLite(){
    //Handle based on PDR
    int x = rand()%100;
    if(x <= PACKET_DROP_RATE)
        return true;
    else
        return false;
}

int handleDataAvailable(int client[], int clidx, packet *tmpPkt){
    int nread = read(client[clidx],tmpPkt,sizeof(packet));
    if(nread <= 0){
        perror("read");
        return -1;
    }
    //print to console
    fprintf(stdout,"RCVD PKT: Seq. No %u of size %u Bytes from channel %u.\n",tmpPkt->seq,tmpPkt->size,tmpPkt->cid);
    if(!takePacketLite()){
        if(tmpPkt->seq < nextRequiredOffset){
            //suppose timeout for prev packet happened before ACK reached there
            //discard such packets
            return 0;
        }
        if(loadDataOnBuffer(tmpPkt)){
            if(tmpPkt->isLastPkt)
                receivedLastPkt = true;
            if(tmpPkt->seq == nextRequiredOffset){
                if(receivedLastPkt)
                    pendingLastPktWrite = false;
                if(DEBUG_MODE)
                    fprintf(stderr,"DEBUG: outputFD = %d\n",outputFd);
                if(nextRequiredOffset != nextOOOoffset){
                    write(outputFd,oooBuffer,(nextOOOoffset-nextRequiredOffset));
                    nextRequiredOffset = nextOOOoffset;
                }
                else{
                    write(outputFd,oooBuffer,tmpPkt->size);
                    nextRequiredOffset = nextOOOoffset = nextRequiredOffset + tmpPkt->size;
                }

            }
            else{
                nextOOOoffset = tmpPkt->seq + tmpPkt->size;
            }
        }
        else return 0;
        //if loadOnBuffer fails, we are dropping this packet as buffer is full
    }
    else return 0;
    return 1;
}

bool sendAckPkt(packet *pkt, int client[], int cid){
    if(pkt == NULL)
        return false;
    if(write(client[cid],pkt,sizeof(packet)) <= 0){
        perror("write");
        return false;
    }
    //Print to Console
    fprintf(stdout,"SENT ACK: for PKT with Seq. No %u from channel %u.\n",pkt->seq,cid);
    return true;
}

bool arqReceiveFile(char *saveFileName){
    //error checking
    if(saveFileName == NULL){
        fprintf(stderr,"arqReceiveFile: Invalid file name received.\n");
        return false;
    }
    FILE *fp = fopen(saveFileName,"w");
    outputFd = fileno(fp);
    if(outputFd == -1){
        fprintf(stderr,"arqReceiveFile: Unable to open file for writing.\n");
        return false;
    }

    struct sockaddr_in serverAddr;
    int listenfd;
    if(!prepareServerSocket(&listenfd,&serverAddr))
        return false;
    int client[2];
    unsigned int cliLen[2];
    struct sockaddr_in clientAddr[2];

    fprintf(stdout,"Server running at port %d.\n",SERVER_PORT);

    //accept 2 connections
    cliLen[0] = sizeof(clientAddr[0]);
    cliLen[1] = sizeof(clientAddr[1]);
    client[0] = accept(listenfd,(struct sockaddr*)&clientAddr[0],&cliLen[0]);
    printf("Connected to client %s on Channel 0.\n",inet_ntoa(clientAddr[0].sin_addr));
    client[1] = accept(listenfd,(struct sockaddr*)&clientAddr[1],&cliLen[1]);
    printf("Connected to client %s on Channel 1.\n",inet_ntoa(clientAddr[1].sin_addr));

    //close listenfd
    close(listenfd);

    fd_set rset, rset0;
    int maxfd = max(client[0],client[1]);
    FD_ZERO(&rset0);
    FD_SET(client[0],&rset0);
    FD_SET(client[1],&rset0);

    initServerGlobals();

    while(pendingLastPktWrite){
        rset = rset0;
        int nread = select(maxfd+1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(client[0],&rset)){
            //received packet on ch0
            packet tmpPkt;
            int ret = handleDataAvailable(client,0,&tmpPkt);
            if(ret == -1)
                return false;
            if(ret == 1){
                //you need to send ACK
                tmpPkt.ptype = ACK_PKT;
                tmpPkt.payload[0] = '\0';
                if(!sendAckPkt(&tmpPkt,client,0))
                    return false;
            }
        }
        if(FD_ISSET(client[1],&rset)){
            //received packet on ch1
            packet tmpPkt;
            int ret = handleDataAvailable(client,1,&tmpPkt);
            if(ret == -1)
                return false;
            if(ret == 1){
                //you need to send ACK
                tmpPkt.ptype = ACK_PKT;
                tmpPkt.payload[0] = '\0';
                if(!sendAckPkt(&tmpPkt,client,1))
                    return false;
            }
        }
    }

    fprintf(stdout,"File received successfully.\n");

    close(outputFd);
    close(client[0]);
    close(client[1]);
    return true;
}

int main(int argc, char *argv[]){
    if(argc != 2) {
        fprintf(stderr,"Usage: %s <output file>\n", argv[0]);
        exit(1);
    }
    if(!arqReceiveFile(argv[1])){
        fprintf(stderr,"Some Error Occurred!\n");
        return -1;
    }
}