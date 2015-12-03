
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>

#include "packet.h"

void error(char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd; //Socket descriptor
	int portno, n;
	socklen_t servlen;
	struct sockaddr_in serv_addr;
	FILE *filename;
	srand(time(NULL));		// added
	struct hostent *server; //contains tons of information, including the server's IP address
	// Packet buffer should fit up to 1KB
	if (argc < 4) {
	   fprintf(stderr,"usage %s hostname portnumber filename\n", argv[0]);
	   exit(0);
	}
	
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
	if (sockfd < 0) 
		error("ERROR opening socket");
	
	server = gethostbyname(argv[1]); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //initialize server's address
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	servlen = sizeof(serv_addr);

	struct packet incoming, outgoing;
	memset((char *) &incoming, 0, sizeof(struct packet));
	memset((char *) &outgoing, 0, sizeof(struct packet));
	
	// Build Request
	outgoing.build_packet( 'R', 0, argv[3]);     // 'R'equest packet, seqNo 0, w/ filename (argv[3]) in buffer


	printf("**********************************************\n");
	printf("PACKET SENT\nPacket type: %c\nPacket seqNum: %d\nPacket size: %d\n",
			outgoing.type, outgoing.seqNum, outgoing.size);
	if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, servlen) < 0) {
		error("ERROR sending request to server!\n");
	}
	int seqNum_needed = 0;
	char *teststring = argv[3];
	filename = fopen(strcat(argv[3], "_copy"), "wb");
	printf("%s\n", argv[3]);
	if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, &servlen) < 0) {
		error("ERROR receiving reply from server!\n");
	}
	while (1) {
		if (incoming.type == 'F') break;	// When we receive a FIN message, transfer is done
		printf("**********************************************\n");
		printf("\t\tPACKET RECEIVED\n");
		printf("\t\tPacket type: %c\n", incoming.type);
		printf("\t\tPacket seqNum: %d\n", incoming.seqNum);
		printf("\t\tPacket size: %d\n", incoming.size);
		
		/*******************
		 * CHECK SEQUENCE NO
		 ******************/
		if (incoming.seqNum == seqNum_needed) {
			if (incoming.type != 'D') {
				printf("Packet ignored; NOT DATA. seqNum: %d\n", incoming.seqNum);
				continue;
			}
			// This is the packet we're waiting for!
			fwrite(incoming.data, 1, incoming.size, filename);
			outgoing.build_packet( 'A', seqNum_needed, "");
			seqNum_needed++;
		}
		else if (incoming.seqNum > seqNum_needed ) {
			printf("Packet ignored; Wrong seqNum: %d Need: %d\n", incoming.seqNum, seqNum_needed);
			continue;
		}
		else if (incoming.seqNum < seqNum_needed ) {
			printf("Packet ignored; Wrong seqNum: %d Need: %d\n", incoming.seqNum, seqNum_needed);
			outgoing.build_packet( 'A', incoming.seqNum, "");
		}

		/*******************
		 * SEND ACK MESSAGE
		 ******************/
		if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, servlen) < 0) {
			error("ERROR sending ACK to server!\n");
		}
		printf("**********************************************\n");
		printf("ACK SENT\n");
		printf("Packet type: %c\n", outgoing.type);
		printf("Packet seqNum: %d\n", outgoing.seqNum);
		printf("Packet size: %d\n", outgoing.size);


		/**********************
		 * RECIEVE NEXT MESSAGE
		 *********************/
		if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, &servlen) < 0) {
			error("ERROR receiving reply from server!\n");
		}
	}
	printf("**********************************************\n");
	printf("Received FIN message.\nMessage: %s\n", incoming.data);

	/************************
	 * SEND FIN ACK TO SERVER
	 ***********************/
	outgoing.build_packet( 'F', 0, "FIN");
	if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, servlen) < 0) {
		error("ERROR sending FIN ACK to server!\n");
	}
	printf("**********************************************\n");
	printf("FIN ACK SENT\n");
	printf("Packet type: %c\n", outgoing.type);
	printf("Packet seqNum: %d\n", outgoing.seqNum);
	printf("Packet size: %d\n", outgoing.size);
	printf("Shutting down.\n");

	fclose(filename);
	return 0;
}
