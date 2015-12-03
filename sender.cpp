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
	int sockfd, newsockfd, portno, pid, n, window_size;
	float p_loss, p_corrupt;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	FILE *filename;
	srand(time(NULL));		// added
	fd_set readfds;
	struct timeval timeout = {1, 0}; // tv_sec and tv_usec
	
	if (argc < 5) {
		fprintf(stderr,"Usage: %s <Port> <Window Size> <Loss Prob> <Corrupt Prob>\n", argv[0]);
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);  

	if (sockfd < 0)
		error("ERROR opening socket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory
	//fill in address info
	portno = atoi(argv[1]);
	window_size = atoi(argv[2]);
	p_loss = atof(argv[3]);
	p_corrupt = atof(argv[4]);

	if (portno < 0) error("ERROR invalid Port number\n");
	if (window_size < 0) error("ERROR invalid Window Size (must be at least 1)\n");
	if (p_loss < 0.0 || p_loss > 1.0) error("ERROR invalid Loss Prob (must be between 0.0 and 1.0)\n");
	if (p_corrupt < 0.0 || p_corrupt > 1.0) error("ERROR invalid Corrupt Prob (must be between 0.0 and 1.0)\n");

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
	
	printf("\nWaiting for request...\n");
	while (1) {
		if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
			printf("Packet lost\n");
			continue;
		}

		printf("**********************************************\n");
		printf("\t\tPACKET RECEIVED\n");
		printf("\t\tPacket type: %c\n", incoming.type);
		printf("\t\tPacket seqNum: %d\n", incoming.seqNum);
		printf("\t\tPacket size: %d\n", incoming.size);
		printf("\t\tPacket data: %s\n", incoming.data);
		printf("**********************************************\n");
		if (incoming.type != 'R') {
			printf("Not a request packet. Ignoring.\n");
			printf("**********************************************\n");
			printf("\nWaiting for request...\n");
			continue;
		}
		printf("Client requesting filename '%s': ", incoming.data);

		filename = fopen(incoming.data, "rb");

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
			
			int packet_number = 0;
			int last_acked = 0;

			while (last_acked < packets_needed) {	// When last_acked == packets_needed, client has received all data

				FD_ZERO(&readfds);
				FD_SET(sockfd, &readfds);

				int returnedFromSelect = 0; 		// temp value

				returnedFromSelect = select(sockfd + 1, &readfds, NULL, NULL, &timeout);	 // https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html

				// http://stackoverflow.com/questions/7637765/why-fd-set-fd-zero-for-select-inside-of-loop

				if (returnedFromSelect == -1) {
					error("ERROR during select call");
				}

				else if (FD_ISSET(sockfd, &readfds)) {
					if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
						error("ERROR receiving packet.\n");
						packet_number = 0;
						continue;
					}

					// SIMULATED LOSS
					if ( ((float)rand()/(float)RAND_MAX) < p_loss) {
						printf("**********************************************\n");
						printf("Lost packet (Simulated). seqNum: %d\n", incoming.seqNum);
						packet_number = 0;
						continue;
					}
					// SIMULATED CORRUPTION
					if ( ((float)rand()/(float)RAND_MAX) < p_corrupt) {
						printf("**********************************************\n");
						printf("Packet corrupted (Simulated). seqNum: %d\n", incoming.seqNum);
						packet_number = 0;
						continue;
					}

					// CHECK ACK
					if (incoming.type == 'A') {		// ACK!
						printf("**********************************************\n");
						printf("\t\tACK RECEIVED\n");
						printf("\t\tPacket type: %c\n", incoming.type);
						printf("\t\tPacket seqNum: %d\n", incoming.seqNum);
						printf("\t\tPacket size: %d\n", incoming.size);
						if (incoming.seqNum > last_acked + window_size) {		// seqNum too high
							printf("GBN: Window not slided; ACK not expected. seqNum: %d\n", incoming.seqNum);
						}
						else if (incoming.seqNum >= last_acked) {						// seqNum acceptable
							// if (incoming.seqNum - last_acked > packet_number) {
							// 	packet_number = 0;
							// }
							// else {
							// 	packet_number = packet_number - (incoming.seqNum - last_acked);
							// }
							last_acked = incoming.seqNum + 1;
							printf("GBN: Window slid over. Now at %d out of %d\n", last_acked, packets_needed);
							packet_number = 0;
						}
						else if (incoming.seqNum < last_acked) {						// seqNum too low
							printf("GBN: Window not slided; ACK not expected. seqNum: %d\n", incoming.seqNum);
						}
					}
					else {			// NOT AN ACK!
						printf("**********************************************\n");
						printf("GBN: Window not slided; NOT AN ACK. seqNum: %d\n", incoming.seqNum);
					}
				}

				else {
					if (packet_number >= window_size || last_acked + packet_number >= packets_needed) {
						printf("**********************************************\n");
						printf("Packet timed out. seqNum: %d\n", last_acked);
						packet_number = 0;
					}
					if (last_acked >= packets_needed) break;	// All packets have been ACKed
					memset((char *) &outgoing, 0, sizeof(struct packet));
					
					outgoing.build_packet('D', packet_number + last_acked, "");		// added last_acked

					int tempPacketAllocation = outgoing.seqNum * MAX_PACKET_SIZE;

					fseek(filename, tempPacketAllocation, SEEK_SET);

					outgoing.size = fread(outgoing.data, 1, MAX_PACKET_SIZE, filename);

					if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
						error("ERROR sending message to client.\n");
					}

					printf("**********************************************\n");
					printf("PACKET SENT\n");
					printf("Packet type: %c\n", outgoing.type);
					printf("Packet seqNum: %d\n", outgoing.seqNum);
					printf("Packet size: %d\n", outgoing.size);
					packet_number++;
				}
			}
		}
		else {
			printf("File not found. Sending FIN.\n", incoming.data);
			memset((char *) &outgoing, 0, sizeof(struct packet));
			outgoing.build_packet('F', 0, "File Not Found.");	// File not found; send FIN message

			if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
				error("ERROR sending message to client.\n");
			}
		}

		// now have to send the 'F' for FIN
		memset((char *) &outgoing, 0, sizeof(struct packet));
		outgoing.build_packet('F', 0, "");	

		if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
			error("ERROR sending FIN.\n");
		}
		printf("**********************************************\n");
		printf("FIN SENT\n");
		printf("Packet type: %c\n", outgoing.type);
		printf("Packet seqNum: %d\n", outgoing.seqNum);
		printf("Packet size: %d\n", outgoing.size);
		
		// Wait for the FIN ACK here
		while (1) {
			FD_ZERO(&readfds);
			FD_SET(sockfd, &readfds);
			if (select(sockfd+1, &readfds, NULL, NULL, &timeout) < 0) {
				error("ERROR on select\n");
			}
			else if (FD_ISSET(sockfd, &readfds)) {
				if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, &clilen) < 0) {
					printf("Packet lost\n");
					continue;
				}
				if (incoming.type == 'F') {
					printf("**********************************************\n");
					printf("\t\tFIN ACK RECEIVED\n");
					printf("\t\tPacket type: %c\n", incoming.type);
					printf("\t\tPacket seqNum: %d\n", incoming.seqNum);
					printf("\t\tPacket size: %d\n", incoming.size);
					printf("\nWaiting for request...\n");
					break;
				}
				else {
					if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &cli_addr, clilen) < 0) {
						error("ERROR sending FIN.\n");
					}
					printf("**********************************************\n");
					printf("\t\tPACKET RECEIVED\n");
					printf("\t\tPacket type: %c\n", incoming.type);
					printf("\t\tPacket seqNum: %d\n", incoming.seqNum);
					printf("\t\tPacket size: %d\n", incoming.size);
					printf("**********************************************\n");
					printf("FIN RESENT\n");
					printf("Packet type: %c\n", outgoing.type);
					printf("Packet seqNum: %d\n", outgoing.seqNum);
					printf("Packet size: %d\n", outgoing.size);
				}
			}
		}
	}
	return 0;
}
