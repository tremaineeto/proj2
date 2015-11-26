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
#include <stdbool.h>

void parse(char *buffer, char** response_buffer, int *buffer_length)        // function to parse response & create our headers
{
    char *temp_response_buffer;
    temp_response_buffer = (char*) calloc(*buffer_length, sizeof(char));
    char *req_type, *file_name, *html_version;
    req_type = strtok(buffer, " ");     // tokenize by spaces
    file_name = strtok(NULL, " ");
    file_name++;    // Get rid of the slash in first char
    html_version = strtok(NULL, "\r");  // look for carriage returns and tokenize by them
    
    if (access(file_name, F_OK) == -1)    // FILE DOESN'T EXIST
    {
        strncpy(*response_buffer, "HTTP/1.1", 8);    // first call must be strncpy; explicitly writing
        strcat(*response_buffer, " 404 Not Found\r\n");     // 404 status code corresponds to file not found
        strcat(*response_buffer, "Server: CS118 Project\r\n");      // coding our server as "CS118 project"
        
        // Date and Time
        time_t t = time(NULL);
        struct tm * date_and_time;
        date_and_time = gmtime(&t);
        strcat(*response_buffer, "Date: ");
        // Day of the week
        switch (date_and_time->tm_wday) {           // turns the integer response (days after Sunday) into a string
            case 0:
                strcat(*response_buffer, "Sun, ");
                break;
            case 1:
                strcat(*response_buffer, "Mon, ");
                break;
            case 2:
                strcat(*response_buffer, "Tue, ");
                break;
            case 3:
                strcat(*response_buffer, "Wed, ");
                break;
            case 4:
                strcat(*response_buffer, "Thu, ");
                break;
            case 5:
                strcat(*response_buffer, "Fri, ");
                break;
            case 6:
                strcat(*response_buffer, "Sat, ");
                break;
        }
        // Date of the month
        if (date_and_time->tm_mday < 10) {              // appends 0 to front to always get 2 digits
            strcat(*response_buffer, "0");
        }
        char int_string[5];  // To convert integers to strings (year = need 5)
        int n;
        n = sprintf(int_string, "%d", date_and_time->tm_mday);
        strcat(*response_buffer, int_string);  // change second argument to string
        // Month
        switch (date_and_time->tm_mon) {            // converts integer (months after January) to strings
            case 0:
                strcat(*response_buffer, " Jan ");
                break;
            case 1:
                strcat(*response_buffer, " Feb ");
                break;
            case 2:
                strcat(*response_buffer, " Mar ");
                break;
            case 3:
                strcat(*response_buffer, " Apr ");
                break;
            case 4:
                strcat(*response_buffer, " May ");
                break;
            case 5:
                strcat(*response_buffer, " Jun ");
                break;
            case 6:
                strcat(*response_buffer, " Jul ");
                break;
            case 7:
                strcat(*response_buffer, " Aug ");
                break;
            case 8:
                strcat(*response_buffer, " Sep ");
                break;
            case 9:
                strcat(*response_buffer, " Oct ");
                break;
            case 10:
                strcat(*response_buffer, " Nov ");
                break;
            case 11:
                strcat(*response_buffer, " Dec ");
                break;
        }
        // Year
        n = sprintf(int_string, "%d", date_and_time->tm_year + 1900);  // Epoch time (years after 1900)
        strcat(*response_buffer, int_string);
        // Hour
        strcat(*response_buffer, " ");
        if (date_and_time->tm_hour < 10) {      // ensures 2 digits for hour
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_hour);
        strcat(*response_buffer, int_string);
        // Minutes
        strcat(*response_buffer, ":");
        if (date_and_time->tm_min < 10) {       // ensures 2 digits for minutes
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_min);
        strcat(*response_buffer, int_string);
        // Seconds
        strcat(*response_buffer, ":");
        if (date_and_time->tm_sec < 10) {       // ensures 2 digits for seconds
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_sec);
        strcat(*response_buffer, int_string);
        strcat(*response_buffer, " GMT\r\n");   // time zone
        
        // Content-Type
        strcat(*response_buffer, "Content-Type: text/html; charset=utf-8\r\n");    // content-type of ERROR page is always html
        
        // Content-Length
        strcat(*response_buffer, "Content-Length: 89\r\n");     // we counted the number of characters
        
        // Connection
        strcat(*response_buffer, "Connection: keep-alive\r\n");
        
        // 404 Error Message
        strcat(*response_buffer, "\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1></BODY></HTML>");
    }
    
    else  // Valid filename
    {
        
        strncpy(*response_buffer, "HTTP/1.1", 8);
        if (access(file_name, R_OK) == -1) {        // Permission Error
            strcat(*response_buffer, " 403 Forbidden\r\n");
        }
        else {
            strcat(*response_buffer, " 200 OK\r\n");    // 200 error code corresponds to an OK (valid) response
        }
        strcat(*response_buffer, "Connection: keep-alive\r\n");
        strcat(*response_buffer, "Server: CS118 Project\r\n");
        
        // Date and Time
        time_t t = time(NULL);
        struct tm * date_and_time;
        // date_and_time = localtime(&t);
        date_and_time = gmtime(&t);
        strcat(*response_buffer, "Date: ");
        // Day of the week
        switch (date_and_time->tm_wday) {       // turns the integer response (days after Sunday) into a string
            case 0:
                strcat(*response_buffer, "Sun, ");
                break;
            case 1:
                strcat(*response_buffer, "Mon, ");
                break;
            case 2:
                strcat(*response_buffer, "Tue, ");
                break;
            case 3:
                strcat(*response_buffer, "Wed, ");
                break;
            case 4:
                strcat(*response_buffer, "Thu, ");
                break;
            case 5:
                strcat(*response_buffer, "Fri, ");
                break;
            case 6:
                strcat(*response_buffer, "Sat, ");
                break;
        }
        // Date of the month
        if (date_and_time->tm_mday < 10) {          // ensuring 2 digits for the month
            strcat(*response_buffer, "0");
        }
        char int_string[5];  // to convert integers to strings (year = need 5)
        int n;
        n = sprintf(int_string, "%d", date_and_time->tm_mday);
        strcat(*response_buffer, int_string);  // change second argument to string
        // Month
        switch (date_and_time->tm_mon) {
            case 0:
                strcat(*response_buffer, " Jan ");
                break;
            case 1:
                strcat(*response_buffer, " Feb ");
                break;
            case 2:
                strcat(*response_buffer, " Mar ");
                break;
            case 3:
                strcat(*response_buffer, " Apr ");
                break;
            case 4:
                strcat(*response_buffer, " May ");
                break;
            case 5:
                strcat(*response_buffer, " Jun ");
                break;
            case 6:
                strcat(*response_buffer, " Jul ");
                break;
            case 7:
                strcat(*response_buffer, " Aug ");
                break;
            case 8:
                strcat(*response_buffer, " Sep ");
                break;
            case 9:
                strcat(*response_buffer, " Oct ");
                break;
            case 10:
                strcat(*response_buffer, " Nov ");
                break;
            case 11:
                strcat(*response_buffer, " Dec ");
                break;
        }
        // Year
        n = sprintf(int_string, "%d", date_and_time->tm_year + 1900);  // Epoch time
        strcat(*response_buffer, int_string);
        // Hour
        strcat(*response_buffer, " ");
        if (date_and_time->tm_hour < 10) {
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_hour);
        strcat(*response_buffer, int_string);
        // Minutes
        strcat(*response_buffer, ":");
        if (date_and_time->tm_min < 10) {
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_min);
        strcat(*response_buffer, int_string);
        // Seconds
        strcat(*response_buffer, ":");
        if (date_and_time->tm_sec < 10) {
            strcat(*response_buffer, "0");
        }
        n = sprintf(int_string, "%d", date_and_time->tm_sec);
        strcat(*response_buffer, int_string);
        strcat(*response_buffer, " GMT\r\n");
        
        if (access(file_name, R_OK) == -1) {        // Permission Error
            // Content Type
            strcat(*response_buffer, "Content-Type: text/html; charset=utf-8\r\n");  // ERROR page is always of type html
            // Content-Length
            strcat(*response_buffer, "Content-Length: 89\r\n");
            // Data
            strcat(*response_buffer, "\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD><BODY><H1>403 Forbidden</H1></BODY></HTML>");
        }
        else {
            // Content Type
            int i;
            char cont_type = 't';
            for (i=0; i < strlen(file_name); i++) {     // checking to see what type of file the extension is
                if (file_name[i] == '.') {
                    switch (file_name[i+1]) {
                        case 'j':                   // checking for the j in jpeg
                            cont_type = 'i';        // "image"
                            strcat(*response_buffer, "Content-Type: image/jpeg\r\n");
                            break;
                        case 'g':                   // checking for the g in gif
                            cont_type = 'i';        // "image"
                            strcat(*response_buffer, "Content-Type: image/gif\r\n");
                            break;
                        default:                    // default is HTML
                            cont_type = 't';        // "text"
                            strcat(*response_buffer, "Content-Type: text/html; charset=utf-8\r\n");
                            break;
                    }
                    break;
                }
            }
            if (i == strlen(file_name)) {    // accounting for no extension
                strcat(*response_buffer, "Content-Type: text/html; charset=utf-8\r\n");
            }
            
            // Get File Info
            struct stat attrib;
            int ret_val;
            ret_val = stat(file_name, &attrib);
            
            // Content-Length
            char file_size[20];
            n = sprintf(file_size, "%d", attrib.st_size);
            strcat(*response_buffer, "Content-Length: ");       // adding content length to buffer
            strcat(*response_buffer, file_size);
            strcat(*response_buffer, "\r\n");
            
            // Last-Modified
            date_and_time = gmtime(&(attrib.st_mtime));
            strcat(*response_buffer, "Last-Modified: ");
            // Day of the week
            switch (date_and_time->tm_wday) {
                case 0:
                    strcat(*response_buffer, "Sun, ");
                    break;
                case 1:
                    strcat(*response_buffer, "Mon, ");
                    break;
                case 2:
                    strcat(*response_buffer, "Tue, ");
                    break;
                case 3:
                    strcat(*response_buffer, "Wed, ");
                    break;
                case 4:
                    strcat(*response_buffer, "Thu, ");
                    break;
                case 5:
                    strcat(*response_buffer, "Fri, ");
                    break;
                case 6:
                    strcat(*response_buffer, "Sat, ");
                    break;
            }
            // Date of the month
            if (date_and_time->tm_mday < 10) {
                strcat(*response_buffer, "0");
            }
            n = sprintf(int_string, "%d", date_and_time->tm_mday);
            strcat(*response_buffer, int_string);  // change second argument to string
            // Month
            switch (date_and_time->tm_mon) {            // converts integer (months after January) to strings
                case 0:
                    strcat(*response_buffer, " Jan ");
                    break;
                case 1:
                    strcat(*response_buffer, " Feb ");
                    break;
                case 2:
                    strcat(*response_buffer, " Mar ");
                    break;
                case 3:
                    strcat(*response_buffer, " Apr ");
                    break;
                case 4:
                    strcat(*response_buffer, " May ");
                    break;
                case 5:
                    strcat(*response_buffer, " Jun ");
                    break;
                case 6:
                    strcat(*response_buffer, " Jul ");
                    break;
                case 7:
                    strcat(*response_buffer, " Aug ");
                    break;
                case 8:
                    strcat(*response_buffer, " Sep ");
                    break;
                case 9:
                    strcat(*response_buffer, " Oct ");
                    break;
                case 10:
                    strcat(*response_buffer, " Nov ");
                    break;
                case 11:
                    strcat(*response_buffer, " Dec ");
                    break;
            }
            // Year
            n = sprintf(int_string, "%d", date_and_time->tm_year + 1900);  // Epoch time
            strcat(*response_buffer, int_string);
            // Hour
            strcat(*response_buffer, " ");
            if (date_and_time->tm_hour < 10) {      // ensuring two digits for hour
                strcat(*response_buffer, "0");
            }
            n = sprintf(int_string, "%d", date_and_time->tm_hour);
            strcat(*response_buffer, int_string);
            // Minutes
            strcat(*response_buffer, ":");
            if (date_and_time->tm_min < 10) {       // ensuring two digits for minutes
                strcat(*response_buffer, "0");
            }
            n = sprintf(int_string, "%d", date_and_time->tm_min);
            strcat(*response_buffer, int_string);
            // Seconds
            strcat(*response_buffer, ":");
            if (date_and_time->tm_sec < 10) {       // ensuring two digits for seconds
                strcat(*response_buffer, "0");
            }
            n = sprintf(int_string, "%d", date_and_time->tm_sec);
            strcat(*response_buffer, int_string);
            strcat(*response_buffer, " GMT\r\n");
            
            // Resize "response_buffer" so that data can fit no matter the size
            *buffer_length = attrib.st_size+strlen(*response_buffer)+2;    // +2 for '\r\n' between header and content
            *response_buffer = (char*) realloc(*response_buffer, (*buffer_length)*(sizeof(char)));
            strcat(*response_buffer, "\r\n");
            
            // Data
            if (cont_type == 'i') {    // If content is a picture, must treat it differently (mmap and memcpy)
                char *img_src;
                int fp;
                fp = open(file_name, O_RDONLY);
                img_src = mmap(NULL, attrib.st_size, PROT_READ, MAP_PRIVATE, fp, 0);
                
                strncpy(temp_response_buffer, *response_buffer, (*buffer_length)*sizeof(char));
                
                memcpy(*response_buffer, img_src, attrib.st_size);

                close(fp);
            }
            else {              // content is not a picture, so can read line by line
                FILE *fp;
                fp = fopen(file_name, "r");
                
                char *line = NULL;
                size_t len = 0;
                ssize_t read = 0;
                while ( (read = getline(&line, &len, fp)) != -1) {
                    strcat(*response_buffer, line);
                }
                
                fclose(fp);
            }
        }
    }
}

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  //create socket
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
    
    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);     // add this code to infinite while
    
        if (newsockfd < 0)
            error("ERROR on accept");
    
        pid = fork(); // forks a new process
        if (pid < 0)
            error("ERROR on fork");
    
        if (pid == 0)  { // pid resulting from fork()
            close(sockfd);
            int buffer_length = 1000;   // our own reasonable buffer length
    
            int n;
            char buffer[256];
            char *response_buffer, *file_name;
            response_buffer = (char*) calloc(buffer_length, sizeof(char));      // calloc for safety
            int rb_len = 0;
    
            memset(buffer, 0, 256);  //reset memory
    
            //read client's message
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            printf("Here is the message:\n%s\n\n",buffer);
    
            // Create response
            parse(buffer, &response_buffer, &buffer_length);
    
            //reply to client
            n = write(newsockfd, response_buffer, buffer_length);
            if (n < 0) error("ERROR writing to socket");
    
            exit(0);
        }
        else // returns the child's pid to the parent
            close(newsockfd);
    }
    return 0;
}
