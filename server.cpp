#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <unistd.h>
using namespace std;

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
     sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     //fill in address info
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);	//5 simultaneous connection at most
     
     //accept connections
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);         
     if (newsockfd < 0) 
       error("ERROR on accept");
     
     int n, msgsize = 0, maxsize = 256;
     char buffer[256], *msg = (char*) malloc(maxsize);   			 
     memset(msg, 0, 256);	//reset memory
     
     //read client's message
     do {
       memset(buffer, 0, 256);
       n = read(newsockfd,buffer,255);
       msgsize += n;
       if (msgsize >= maxsize) {
	 maxsize *= 2;
	 msg = (char*) realloc(msg, maxsize);
       }
       strcat(msg, buffer);
       if (n < 255) {
	 msg[msgsize] = '\0';
	 break;
       }
     } while (n > 0);

     printf("REQUEST MESSAGE:\n%s", msg); //message

     // extract filename
     int i, k;
     char fname[256];
     for (k = 0, i = 0; msg[i] != '\0'; i++) {
       if (msg[i] == '/') {
	 k = ++i;
	 while(msg[i++] != ' ');
	 break;
       }
     }
     strncpy(fname, &msg[k], i-k-1);
     fname[i-k-1] = '\0';
     
     // get filetype
     char ftype[256];
     for (i = 0; fname[i] != '\0'; i++) 
       if (fname[i] == '.') break;     
     strncpy(ftype, &fname[i+1], strlen(fname)-i);
     ftype[strlen(fname)-i] = '\0';
     char *ctype = (char*) malloc(256);
     if (!strcmp(ftype, "html"))
       ctype = "text/html";
     else if (!strcmp(ftype, "jpg") || !strcmp(ftype, "jpeg"))
       ctype = "image/jpeg";
     else if (!strcmp(ftype, "gif"))
       ctype = "image/gif";
     else 
       ctype = "text/plain";
     
     // open file and read contents
     FILE *fp;
     fp = fopen(fname, "rb");
     if (fp == NULL)
       error("ERROR opening file");
     fseek(fp, 0, SEEK_END);
     long fsize = ftell(fp);
     fseek(fp, 0, SEEK_SET);
     
     string fcontent;
     fcontent.resize(fsize);
     if(fread(&fcontent[0], 1, fsize, fp) < 0)
       error("ERROR reading file");
     fclose(fp);
     
     // create header
     char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n";
     sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", ctype);
     
     // response message
     string headerString = header;
     string fcontentString = fcontent;
     string response = headerString + fcontent;
     n = write(newsockfd,&response[0],response.size());
     if (n < 0) error("ERROR writing to socket");
          
     close(newsockfd);//close connection 
     close(sockfd);

     return 0; 
}

