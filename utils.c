/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#include "packet.h"
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include <sys/time.h>
#include "config.h"

/*
 * Tests whether strings s1 and s2 are equal
 */
bool equals(char *s1, char *s2){
    if(s1 == NULL && s2 == NULL)
        return true;
    else if(s1 == NULL || s2 == NULL)
        return false;
    else
        return (strcmp(s1,s2) == 0);
}

/*
 * Returns the size of the given file
 * Returns -1 if the file can't be opened.
 */
int getFileSize(char *file){
    int fd = open(file,O_RDONLY);
    if(fd == -1)
        return -1;
    int size = lseek(fd,0,SEEK_END);
    close(fd);
    return size;
}

int max(int x, int y){
    return (x>y)?x:y;
}

//bool encodePkt2Str(packet *pkt, char *encodedStr){
//    if(pkt == NULL || encodedStr == NULL)
//        return false;
//    sprintf(encodedStr,"%u#%u#%u#%u#%u#",pkt->size,pkt->seq,pkt->isLastPkt,pkt->ptype,pkt->cid);
//    int len = strlen(encodedStr);
//    if((pkt->ptype) == DATA_PKT){
//        for(uint i=0; i<(pkt->size);i++){
//            encodedStr[len+i] = (pkt->payload)[i];
//        }
//    }
//    return true;
//}
//
//bool decodeStr2Pkt(packet *decodedPkt, char *encStr){
//    if(decodedPkt == NULL || encStr == NULL){
//        fprintf(stderr,"decodeStr2Pkt: Received NULL argument.\n");
//        return false;
//    }
//    char *str = strdup(encStr);
//    int len = strlen(str);
//    int dataStartPos = 0;
//    int hashCount = 0;
//    for(int i = 0; i<len; i++){
//        if(str[i] == '#')
//            hashCount++;
//        if(hashCount == 5){
//            //data will start from next position
//            dataStartPos = i+1;
//            str[i] = '\0';
//            break;
//        }
//    }
//    if(hashCount != 5){
//        fprintf(stderr,"decodeStr2Pkt: Invalid format for the encoded string.\n");
//        return false;
//    }
//    unsigned int ilp = 0;
//    sscanf(str,"%u#%u#%u#%u#%u#",&(decodedPkt->size),&(decodedPkt->seq),&ilp,&(decodedPkt->ptype),&(decodedPkt->cid));
//    decodedPkt->isLastPkt = ilp;
//    for(uint i=0; i<(decodedPkt->size); i++){
//        (decodedPkt->payload)[i] = encStr[dataStartPos+i];
//    }
//    free(str);
//    return true;
//}

void initPacket(packet *pkt){
    if(pkt == NULL)
        return;
    pkt->size = 0;
    pkt->seq = 0;
    pkt->isLastPkt = false;
    pkt->ptype = ACK_PKT;
    pkt->cid = CHANNEL_0;
    memset(pkt->payload,0,sizeof(pkt->payload));
}

double convTimeval2MilliSec(struct timeval *tv){
    double tt;
    tt = (tv->tv_sec)*1e6;
    tt = (tt + tv->tv_usec)* 1e-3;
    return tt;
}

void convMilliSec2Timeval(double milliSec, struct timeval *tv){
    long long usec = ((milliSec*1e3) + 0.5);
    tv->tv_sec = usec/1000000;
    tv->tv_usec = usec % 1000000;
}

double findRemainingTime(struct timeval *startTime, struct timeval *remainingTime){
    struct timeval currTime, tmpDiff;
    gettimeofday(&currTime,NULL);
    timerclear(remainingTime);
    timersub(&currTime,startTime,&tmpDiff);
    if(convTimeval2MilliSec(&tmpDiff) < TIMEOUT_MILLISECONDS){
        struct timeval timeoutVal;
        convMilliSec2Timeval(TIMEOUT_MILLISECONDS,&timeoutVal);
        timersub(&timeoutVal,&tmpDiff,remainingTime);
        return convTimeval2MilliSec(remainingTime);
    } else
        return 0;
}