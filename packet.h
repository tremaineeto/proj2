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

  void build_request_packet(char * filename);
};

void packet::build_request_packet(char *filename) {
  type = 'R';
  seqNum = 0;
  size = strlen(filename) + 1;
  strcpy(data, filename);
}

