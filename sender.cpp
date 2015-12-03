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
	FILE *filename;
	srand(time(NULL));		// added
	fd_set readfds;
	struct timeval timeout = {1, 0}; // tv_sec and tv_usec
	
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");			// TODO: eventually add window_size, loss_prob, corrupt_prob
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);  

	if (sockfd < 0)
		error("ERROR opening socket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory
	//fill in address info
	portno = atoi(argv[1]);
	// TODO: add window argument
	// TODO: add loss probability argument
	// TODO: add corruption probability argument
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// TODO: add check that the loss probability and corruption probabilities are between 0 and 1.0
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
			 sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	
	listen(sockfd,5);  //5 simultaneous connection at most
	
	struct packet incoming, outgoing;
  
	clilen = sizeof(cli_addr);

	while (1) {
		printf("\nWaiting for request...\n");
		if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
			error("ERROR receiving request.\n");
		}
		  
		printf("Packet type: %c\n", incoming.type);
		printf("Packet seqNum: %d\n", incoming.seqNum);
		printf("Packet size: %d\n", incoming.size);
		printf("Packet data: %s\n", incoming.data);
		
		//std::ifstream request(incoming.data, std::ios::in | std::ios::binary);


		printf("Client requesting filename '%s': ", incoming.data);

		filename = fopen(incoming.data, "rb");
		//printf("hi");

		if(filename) {
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

				FD_ZERO(&readfds);
				FD_SET(sockfd, &readfds);

				int returnedFromSelect = 0; 		// temp value

				returnedFromSelect = select(sockfd + 1, &readfds, NULL, NULL, &timeout);	 // https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html

				// http://stackoverflow.com/questions/7637765/why-fd-set-fd-zero-for-select-inside-of-loop

				if (returnedFromSelect == -1) {
					error("ERROR during select call");
				}

				else if (FD_ISSET(sockfd, &readfds) == true) {
					if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
						error("ERROR receiving packet.\n");
						packet_number = 0;
						continue;
					}

					// TODO: model LOSS

					// TODO: model CORRUPTION

					printf("Packet type: %c\n", incoming.type);
					printf("Packet seqNum: %d\n", incoming.seqNum);
					printf("Packet size: %d\n", incoming.size);
					printf("Packet data: %s\n", incoming.data);

					if (incoming.seqNum >= last_acked) {
						last_acked = incoming.seqNum + 1;
						printf("\nGBN: Window slided. Now at %d out of %d\n", packet_number, packets_needed);
						packet_number = 0;
					}
					else if (incoming.type != 'A') {			// ACK!
						printf("\nDon't slide window; NOT AN ACK. seqNum: %d\n", incoming.seqNum);
					}

					// TODO: UNCOMMENT WHEN YOU DEFINE WINDOWSIZE
					// else if (incoming.seqNum > packet_number + windowSize) {		// need to define window_size (argv[2])
						// printf("\nDon't slide window; ACK not expected. seqNum: %d\n", incoming.seqNum);
					// }
					else if (packet_number > incoming.seqNum) {
						printf("\nDon't slide window; ACK not expected. seqNum: %d\n", incoming.seqNum);
					}
				} 

				else {
					if (packets_needed <= last_acked + packet_number) {
						continue;
					}
					// TODO: UNCOMMENT WHEN YOU DEFINE WINDOWSIZE
					// else if (windowSize <= packet_number) {
					// 	printf("\nERROR on the last_acked value %d", last_acked);
					//  packet_number = 0;
					// }
					else if (packets_needed <= packet_number + last_acked) {
						printf("\nERROR on the last_acked value %d", last_acked);
						packet_number = 0;
					}

					memset((char *) &outgoing, 0, sizeof(struct packet));
					
					outgoing.build_packet('D', packet_number + last_acked, "Valid filename.\n");		// added last_acked

					int tempPacketAllocation = outgoing.seqNum * MAX_PACKET_SIZE;

					fseek(filename, tempPacketAllocation, SEEK_SET);

					outgoing.size = fread(outgoing.data, 1, MAX_PACKET_SIZE, filename);

					if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
						error("ERROR sending message to client.\n");
					}

					printf("Packet type: %c\n", outgoing.type);
					printf("Packet seqNum: %d\n", outgoing.seqNum);
					printf("Packet size: %d\n", outgoing.size);
					printf("Packet data: %s\n", outgoing.data);
				}

				// Now, we need to send the FIN
				printf("Sending FIN.\n", incoming.data);
				printf("**********************************************\n");
				memset((char *) &outgoing, 0, sizeof(struct packet));
				
				outgoing.build_packet('F', packet_number, "Valid filename.\n");		// added last_acked

				if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
					error("ERROR sending FIN message.\n");
				}

				printf("Packet type: %c\n", outgoing.type);
				printf("Packet seqNum: %d\n", outgoing.seqNum);
				printf("Packet size: %d\n", outgoing.size);
				printf("Packet data: %s\n", outgoing.data);

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

		// now have to send the 'F' for FIN
		memset((char *) &outgoing, 0, sizeof(struct packet));
		outgoing.build_packet('F', 0, "FIN");	

		if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
			error("ERROR sending FIN.\n");
		}

		printf("Packet type: %c\n", outgoing.type);
		printf("Packet seqNum: %d\n", outgoing.seqNum);
		printf("Packet size: %d\n", outgoing.size);
		printf("Packet data: %s\n", outgoing.data);

		// TODO: wait for the FIN ACK here
	}
	return 0;
}
