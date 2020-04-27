/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
#ifndef CN_P1_UTILS_H
#define CN_P1_UTILS_H

#include <stdbool.h>
#include "packet.h"
#include <sys/time.h>

bool equals(char *s1, char *s2);
int getFileSize(char *file);
bool encodePkt2Str(packet *pkt, char *encodedStr);
bool decodeStr2Pkt(packet *decodedPkt, char *encStr);
void initPacket(packet *pkt);
int max(int x, int y);
double convTimeval2MilliSec(struct timeval *tv);
void convMilliSec2Timeval(double milliSec, struct timeval *tv);
double findRemainingTime(struct timeval *startTime, struct timeval *remainingTime);

#endif //CN_P1_UTILS_H


