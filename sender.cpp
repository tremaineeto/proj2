/*
 A simple server in the internet domain using TCP
 The port number is passed as an argument
 This version runs forever, forking off a separate
 process for each connection
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>  /* for the waitpid() system call */
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fstream>
#include <sstream>
#include <stdbool.h>

#include "packet.h"

void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, pid, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);  

	if (sockfd < 0)
		error("ERROR opening socket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory
	//fill in address info
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			 sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	
	listen(sockfd,5);  //5 simultaneous connection at most
	
	struct packet incoming, outgoing;
  
	clilen = sizeof(cli_addr);

	while (1) {
		printf("Waiting for request...\n");
		if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
			error("ERROR receiving request.\n");
		}
		  
		printf("packet type: %c\n", incoming.type);
		printf("packet seqNum: %d\n", incoming.seqNum);
		printf("packet size: %d\n", incoming.size);
		printf("packet data: %s\n", incoming.data);
		
		std::ifstream request(incoming.data, std::ios::in | std::ios::binary);
		printf("Client requesting filename '%s': ", incoming.data);
		
		if(request) {
			int packets_needed;

			// Determine how many packets are needed for file
			struct stat file_info;
			stat(incoming.data, &file_info);
			if (file_info.st_size % MAX_PACKET_SIZE) {
				packets_needed = (file_info.st_size / MAX_PACKET_SIZE ) + 1;
			}
			else {
				packets_needed = (file_info.st_size / MAX_PACKET_SIZE );
			}
			
			printf("File found. %d packet(s) will be needed.\n", packets_needed);
			printf("**********************************************\n");
			
			int packet_number = 0;
			int last_acked = 0;

			while (packet_number < packets_needed) {
				memset((char *) &outgoing, 0, sizeof(struct packet));
				
				outgoing.build_packet('D', packet_number, "Valid filename.\n");
				if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
					error("ERROR sending message to client.\n");
				}
				packet_number++;
			}
		}
		else {
			printf("File not found. Sending FIN.\n", incoming.data);
			printf("**********************************************\n");
			memset((char *) &outgoing, 0, sizeof(struct packet));
			outgoing.build_packet('F', 0, "File Not Found.");	// File not found; send FIN message

			if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
				error("ERROR sending message to client.\n");
			}
		}
	}
	return 0;
}
