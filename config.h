/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#ifndef CN_P1_CONFIG_H
#define CN_P1_CONFIG_H

//Packet Size in number of bytes
#define PACKET_SIZE 100
//Timeout in Milliseconds (for resending a packet)
#define TIMEOUT_MILLISECONDS 2000
//Rate for dropping data packets
#define PACKET_DROP_RATE 10 //in percentage (example 10 for 10%)

/***SERVER CONFIGURATION***/
#define SERVER_PORT 9002
#define SERVER_IP_ADDR "127.0.0.1"
//Maximum number of packets that can be buffered
#define NUM_PACKETS_IN_BUFFER 10


#define OOO_BUFFER_SIZE (NUM_PACKETS_IN_BUFFER*PACKET_SIZE)
#define OVERALL_PACKET_SIZE PACKET_SIZE + 50

#define DEBUG_MODE 0


#endif //CN_P1_CONFIG_H
