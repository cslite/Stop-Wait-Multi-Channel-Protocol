/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#include <stdbool.h>
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
#include "config.h"

packet *lastSent[2];
int fileSize;
int currRead;
int inputFd;

bool prepare2Sockets(int sock[], struct sockaddr_in serverAddr[]){
    //This function creates and initializes the two sockets for the client
    sock[0] = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock[0] < 0){
        fprintf(stderr,"prepare2Sockets: Error in opening first socket.\n");
        return false;
    }
    sock[1] = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock[1] < 0){
        fprintf(stderr,"prepare2Sockets: Error in opening second socket.\n");
        return false;
    }
    memset(&serverAddr[0],0,sizeof(serverAddr[0]));
    memset(&serverAddr[1],0,sizeof(serverAddr[1]));

    //initialize first sockaddr
    serverAddr[0].sin_family = AF_INET;
    serverAddr[0].sin_port = htons(SERVER_PORT);
    serverAddr[0].sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);

    //initialize second sockaddr
    serverAddr[1].sin_family = AF_INET;
    serverAddr[1].sin_port = htons(SERVER_PORT);
    serverAddr[1].sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);

    return true;
}

bool connect2Sockets(int sock[], struct sockaddr_in serverAddr[]){
    if(!prepare2Sockets(sock,serverAddr))
        return false;
    if(connect(sock[0],(struct sockaddr*)serverAddr,sizeof(serverAddr[0])) == -1){
        fprintf(stderr,"connect2Sockets: Unable to connect to Server's Channel 0.\n");
        return false;
    }
    else
        fprintf(stdout,"Connected to server on Channel 0.\n");
    if(connect(sock[1],(struct sockaddr *)serverAddr+1,sizeof(serverAddr[1])) == -1){
        fprintf(stderr,"connect2Sockets: Unable to connect to Server's Channel 1.\n");
        return false;
    }
    else
        fprintf(stdout,"Connected to server on Channel 1.\n");
    return true;
}

packet *makeNextPktFromFile(){
    packet *ptmp = (packet *) malloc(sizeof(packet));
    initPacket(ptmp);
    ptmp->seq = currRead;
    ptmp->ptype = DATA_PKT;
    int actualRead = read(inputFd,ptmp->payload,PACKET_SIZE);
    if(actualRead < 0){
        perror("open");
        free(ptmp);
        return NULL;
    }
    else if(actualRead == 0){
        free(ptmp);
        return NULL;
    }
    ptmp->size = actualRead;
    currRead += actualRead;
    if(currRead == fileSize)
        ptmp->isLastPkt = true;
    else
        ptmp->isLastPkt = false;
    return ptmp;
}

bool sendDataPkt(packet *pkt, int sock[]){
    if(pkt == NULL)
        return false;
    if(DEBUG_MODE)
        fprintf(stderr,"DEBUG: sendDataPkt: sock[0] = %d, sock[1] = %d\n",sock[0],sock[1]);
    if(write(sock[pkt->cid],pkt,sizeof(packet)) <= 0){
        perror("write");
        return false;
    }
    lastSent[pkt->cid] = pkt;
    //Print to Console
    fprintf(stdout,"SENT PKT: Seq. No %u of size %u Bytes from channel %u.\n",pkt->seq,pkt->size,pkt->cid);
    return true;
}

bool receiveAckPkt(packet *pkt, int sock[], int cid){
    if(pkt == NULL)
        return false;
    int nread;
    if((nread = read(sock[cid],pkt,sizeof(packet))) <= 0){
        perror("read");
        return false;
    }
    //Print to console
    fprintf(stdout,"RCVD ACK: for PKT with Seq. No. %u from channel %d.\n",pkt->seq,cid);
    return true;
}

struct timeval resetTimeoutValue;

bool arqSendFile(char *fileName){
    //error checking
    if(fileName == NULL){
        fprintf(stderr,"arqSendFile: Invalid file name received.\n");
        return false;
    }
    fileSize = getFileSize(fileName);
    if(fileSize == -1){
        fprintf(stderr,"arqSendFile: Cannot open file '%s'.\n",fileName);
        return false;
    }
    else if(fileSize == 0){
        fprintf(stderr,"arqSendFile: File '%s' is empty.\n",fileName);
        return false;
    }

    //connect sockets
    int sock[2];
    struct sockaddr_in serverAddr[2];
    if(!connect2Sockets(sock,serverAddr))
        return false;

    currRead = 0;
    lastSent[0] = NULL;
    lastSent[1] = NULL;
    inputFd = open(fileName,O_RDONLY);
    packet *nextPkt;
    fd_set rset, rset0;
    int maxfd = max(sock[0],sock[1]);
    FD_ZERO(&rset0);
    struct timeval timeRemaining;
    struct timeval sendTime[2];
    bool onlyOnePacket = false;
    convMilliSec2Timeval(TIMEOUT_MILLISECONDS,&resetTimeoutValue);

    if(DEBUG_MODE)
        fprintf(stderr,"DEBUG: arqSendFile: Ready to send Packets.\n");

    //send the first packet over channel 0
    nextPkt = makeNextPktFromFile();
    nextPkt->cid = CHANNEL_0;
    if(!sendDataPkt(nextPkt,sock))
        return false;
    //note sending time of ch0 packet
    gettimeofday(&sendTime[0],NULL);
    FD_SET(sock[0],&rset0);
    if(currRead == fileSize){
        //only one packet was required
        onlyOnePacket = true;
    }
    else{
        nextPkt = makeNextPktFromFile();
        nextPkt->cid = CHANNEL_1;
        if(!sendDataPkt(nextPkt,sock))
            return false;
        FD_SET(sock[1],&rset0);
        //note sending time of ch1 packet
        gettimeofday(&sendTime[1],NULL);
    }
    //initialize timeRemaining with timeout value

    timeRemaining = resetTimeoutValue;
    struct timeval tmpTR[2]; //structure for storing temp remaining timevals
    while(lastSent[0] != NULL || lastSent[1] != NULL || currRead != fileSize){
        //set rset
        rset = rset0;
        bool ackOn[2] = {false,false};
        int nread = select(maxfd+1,&rset,NULL,NULL,&timeRemaining);
        if(DEBUG_MODE)
            fprintf(stderr,"DEBUG: arqSendFile: Select returned with %d\n",nread);
        //if ACK is received on channel0
        if(FD_ISSET(sock[0],&rset)){
            if(DEBUG_MODE)
                fprintf(stderr,"DEBUG: arqSendFile: FD is SET on Channel 0\n");
            ackOn[0] = true;
            packet tmpAckPkt;
            if((!receiveAckPkt(&tmpAckPkt,sock,0))||(tmpAckPkt.ptype != ACK_PKT) || (tmpAckPkt.seq != lastSent[0]->seq)){
                fprintf(stderr,"arqSendFile: Error in received packet.\n");
                return false;
            }
            //LastSent[0] was acknowledged
            free(lastSent[0]);
            lastSent[0] = NULL;

            //try to send next packet
            if(currRead != fileSize){
                //more packets available to be sent
                nextPkt = makeNextPktFromFile();
                nextPkt->cid = CHANNEL_0;
                if(!sendDataPkt(nextPkt,sock))
                    return false;
                gettimeofday(&sendTime[0],NULL);
            }
            else FD_CLR(sock[0],&rset0);
            tmpTR[0] = resetTimeoutValue;
        }
        //if ACK is received on channel1
        if(FD_ISSET(sock[1],&rset)){
            if(DEBUG_MODE)
                fprintf(stderr,"DEBUG: arqSendFile: FD is set on Channel 1.\n");
            ackOn[1] = true;
            packet tmpAckPkt;
            if((!receiveAckPkt(&tmpAckPkt,sock,1))||(tmpAckPkt.ptype != ACK_PKT) || (tmpAckPkt.seq != lastSent[1]->seq)){
                fprintf(stderr,"arqSendFile: Error in received packet.\n");
                return false;
            }
            //LastSent[1] was acknowledged
            free(lastSent[1]);
            lastSent[1] = NULL;

            //try to send next packet
            if(currRead != fileSize){
                //more packets available to be sent
                nextPkt = makeNextPktFromFile();
                nextPkt->cid = CHANNEL_1;
                if(!sendDataPkt(nextPkt,sock))
                    return false;
                gettimeofday(&sendTime[1],NULL);
            }
            else FD_CLR(sock[1],&rset0);
            tmpTR[1] = resetTimeoutValue;
        }
        //check if there was a timeout
        double tmpTRd[2] = {0};
        tmpTRd[0] = (ackOn[0] || (lastSent[0] == NULL)) ? TIMEOUT_MILLISECONDS :  findRemainingTime(&sendTime[0],&tmpTR[0]);
        if(!onlyOnePacket)
            tmpTRd[1] = (ackOn[1] || (lastSent[1] == NULL)) ? TIMEOUT_MILLISECONDS : findRemainingTime(&sendTime[1],&tmpTR[1]);
        else
            tmpTRd[1] = TIMEOUT_MILLISECONDS;
        if(DEBUG_MODE)
            fprintf(stderr,"DEBUG: arqSendFile: tmpTRd[0]=%lf, tmpTRd[1]=%lf\n",tmpTRd[0],tmpTRd[1]);
        if(tmpTRd[0] == 0){
            //there was a timeout on ch0
            if(DEBUG_MODE)
                fprintf(stderr,"DEBUG: arqSendFile: Timeout on Channel 0.\n");
            //resend lastSent[0] packet
            sendDataPkt(lastSent[0],sock);
            gettimeofday(&sendTime[0],NULL);
            //set timeout values
            tmpTRd[0] = TIMEOUT_MILLISECONDS;
            tmpTR[0] = resetTimeoutValue;
        }
        if(tmpTRd[1] == 0){
            //there was a timeout on ch1
            if(DEBUG_MODE)
                fprintf(stderr,"DEBUG: arqSendFile: Timeout on Channel 1.\n");
            //resend lastSent[0] packet
            sendDataPkt(lastSent[1],sock);
            gettimeofday(&sendTime[1],NULL);
            //set timeout values
            tmpTRd[1] = TIMEOUT_MILLISECONDS;
            tmpTR[1] = resetTimeoutValue;
        }
        if(DEBUG_MODE)
            fprintf(stderr,"DEBUG: arqSendFile: tmpTRd[0]=%lf, tmpTRd[1]=%lf\n",tmpTRd[0],tmpTRd[1]);
        //set the value of remainingTime
        if(tmpTRd[0] <= tmpTRd[1])
            timeRemaining = tmpTR[0];
        else
            timeRemaining = tmpTR[1];
    }
    fprintf(stdout,"\n[INFO]: File sent successfully.\n");

    close(inputFd);
    close(sock[0]);
    close(sock[1]);
    return true;
}

int main(int argc, char *argv[]){
    if(argc != 2) {
        fprintf(stderr,"Usage: %s <input file>\n", argv[0]);
        exit(1);
    }
    if(!arqSendFile(argv[1])){
        fprintf(stderr,"\nSome Error Occurred!\n");
        return -1;
    }

}

