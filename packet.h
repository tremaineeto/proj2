#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
using namespace std;

const int MAX_PACKET_SIZE = 1024;

struct packet {
  char type;    // Type of packet:
                // 'R' = request
                // 'D' = data
                // 'A' = ACK
                // 'F' = FIN (end communication)

  int seqNum;   // Sequence number
  int size;     // Size of packet data
  char data[MAX_PACKET_SIZE];

  void build_packet(char desired_type, int des_seqNum, char * buffer);
};

void packet::build_packet(char desired_type, int des_seqNum, char * buffer) {
  type = desired_type;
  seqNum = des_seqNum;
  size = strlen(buffer);
  strcpy(data, buffer);
}