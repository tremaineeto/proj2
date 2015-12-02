
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
	struct hostent *server; //contains tons of information, including the server's IP address
	// Packet buffer should fit up to 1KB
	char buffer[1000];
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

	printf("SENT:\tType: %c\tSeq No: %d\tSize: %d\tMsg: Requesting filename '%s'\n",
			outgoing.type, outgoing.seqNum, outgoing.size, outgoing.data);
	printf("**********************************************\n");
	if (sendto(sockfd, &outgoing, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, servlen) < 0) {
		error("ERROR sending request to server!\n");
	}

	if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, &servlen) < 0) {
		error("ERROR receiving reply from server!\n");
	}

	// Hasn't received FIN message
	while (incoming.type != 'F') {
		printf("packet type: %c\n", incoming.type);
		printf("packet seqNum: %d\n", incoming.seqNum);
		printf("packet size: %d\n", incoming.size);
		printf("packet data: %s\n", incoming.data);

		if (recvfrom(sockfd, &incoming, sizeof(struct packet), 0, (struct sockaddr*) &serv_addr, &servlen) < 0) {
			error("ERROR receiving reply from server!\n");
		}
	}
	
	printf("Server sent FIN message. Shutting down.\nMessage: %s\n", incoming.data);
	printf("**********************************************\n");
	return 0;
}
