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
  
    while (1) {
        clilen = sizeof(cli_addr);
      //   if (recvfrom(sockfd, &incoming, sizeof(packet), 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)) < 0) {
        //   error("File request lost.\n");
        //   continue;
        // }
        printf("GOT THIS FAR\n");
        n = recvfrom(sockfd, &incoming, sizeof(packet), 0, (struct sockaddr*) &cli_addr, &clilen);
        printf("GOT THIS FAR\n");
        while (n < 0) {
        	printf("Packet loss.\n");
        	n = recvfrom(sockfd, &incoming, sizeof(packet), 0, (struct sockaddr*) &cli_addr, &clilen);	
        }
          
        printf("packet type: %c\n", incoming.type);
        printf("packet seqNum: %d\n", incoming.seqNum);
        printf("packet size: %d\n", incoming.size);
        printf("packet data: %s\n", incoming.data);
        
        
//         char buffer[1000];
//         char *response_buffer, *file_name;
//         response_buffer = (char*) calloc(buffer_length, sizeof(char));      // calloc for safety
//         int rb_len = 0;

//         memset(buffer, 0, 1000);  //reset memory

//         //read client's message
//         n = read(newsockfd,buffer,1000);
//         if (n < 0) error("ERROR reading from socket");
//         printf("Here is the filename:\n%s\n\n",buffer);
        
//         std::ifstream request(buffer, std::ios::in | std::ios::binary);

//         // Create response
//         // parse(buffer, &response_buffer, &buffer_length);
        
//         if (request) {
//             printf("Success! File name found");
//         }
//         else {
//             printf("Unsuccessful! File name not found.");
//         }

//         //reply to client
//         n = write(newsockfd, response_buffer, buffer_length);
//         if (n < 0) error("ERROR writing to socket");

        // exit(0);
    }
    return 0;
}
