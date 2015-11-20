/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port> <filename>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 64
#define HDRSIZE sizeof(int)

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname, *filename;
    char packetbuf[BUFSIZE];
    int filebufsize = BUFSIZE;
    char *filebuf = (char*)malloc(BUFSIZE);
    char *hdrbuf = (char*)malloc(HDRSIZE);
    char *payloadbuf = (char*)malloc(BUFSIZE);
    int acknum = 1;
    int seqnum = 0; // only for receiving

    /* check command line arguments */
    if (argc != 4) {
       fprintf(stderr,"usage: %s <hostname> <port> <filename>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    filename = argv[3];

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* filename requested by the user */
    printf("Requested filename: %s\n", filename);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, filename, strlen(filename), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* server's reply */
    while(1) {
      bzero(packetbuf, BUFSIZE);
      filebufsize += BUFSIZE;
      filebuf = (char*) realloc(filebuf, filebufsize); 
      
      // Receive entire packet
      n = recvfrom(sockfd, packetbuf, BUFSIZE, 0, &serveraddr, &serverlen);
      if (n < 0) 
        error("ERROR in recvfrom");

      // Get header
      memcpy(&seqnum, packetbuf, HDRSIZE);
      printf("Sequence number: %d\n", seqnum);
      // Get payload
      memcpy(payloadbuf, packetbuf+HDRSIZE, BUFSIZE-HDRSIZE);
      printf("Packet content: %s\n", payloadbuf);
      
      // send ACK to server
      memcpy(hdrbuf, &acknum, HDRSIZE);      
      n = sendto(sockfd, hdrbuf, strlen(hdrbuf), 0, &serveraddr, serverlen);
      if (n < 0) 
	      error("ERROR in sendto");
      acknum++;

      strcat(filebuf, packetbuf);
    }
    //TODO: print filebuf.
    // put filebuf into file.
    return 0;
}
