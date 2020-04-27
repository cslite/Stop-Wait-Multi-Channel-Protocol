/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#ifndef CN_P1_PACKET_H
#define CN_P1_PACKET_H

#include <stdbool.h>
#include "config.h"


typedef enum{
    DATA_PKT, ACK_PKT
} pktType;

typedef enum{
    CHANNEL_0, CHANNEL_1
} channelId;

struct packet{
    unsigned int size;  //size of the packet in bytes
    unsigned int seq;   //offset of the first byte of the packet
    bool isLastPkt;
    pktType ptype;
    channelId cid;
    char payload[PACKET_SIZE+1];
};
typedef struct packet packet;


#endif //CN_P1_PACKET_H
