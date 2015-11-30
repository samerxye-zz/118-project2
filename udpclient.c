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
#include <limits.h>

#define PKTSIZE 64
#define HDRSIZE sizeof(int)
#define PAYLOADSIZE (PKTSIZE-HDRSIZE)

/* error - wrapper for perror */
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
    int filebufsize = PAYLOADSIZE; // Accumulates payload from packets
    char *filebuf = (char*)malloc(1);
    char packetbuf[PKTSIZE]; // Data for entire packet: hdrbuf + payloadbuf
    char *hdrbuf = (char*)malloc(HDRSIZE); // Packet header data
    char *payloadbuf = (char*)malloc(PKTSIZE); // Packet payload data
    int acknum = 0; // Sequence number of next packet that client expects from server
    int seqnum = 0; // Sequence number of packet received from server.

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

    /* send the filename to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, filename, strlen(filename), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* Main loop: receive packet, send ACK, repeat */
    while(1) {
      bzero(packetbuf, PKTSIZE);
      filebufsize += PAYLOADSIZE;
      filebuf = (char*) realloc(filebuf, filebufsize); 
      
      // Receive entire packet
      n = recvfrom(sockfd, packetbuf, PKTSIZE, 0, &serveraddr, &serverlen);
      if (n < 0) 
        error("ERROR in recvfrom");

      // Get header
      memcpy(&seqnum, packetbuf, HDRSIZE);
      printf("SEQ#: %d, ACK#: %d\n", seqnum, acknum);
      if (seqnum == INT_MAX) break;
      // Get payload
      memcpy(payloadbuf, packetbuf+HDRSIZE, PAYLOADSIZE);
      printf("Packet content: %s\n", payloadbuf);
      
      // GBN---Only accept packet if in order
      // ie. sequence number of packet == expected packet
      // resend same acknum
      if (seqnum != acknum) {
        memcpy(hdrbuf, &acknum, HDRSIZE);      
	n = sendto(sockfd, hdrbuf, strlen(hdrbuf), 0, &serveraddr, serverlen);
	if (n < 0) 
		error("ERROR in sendto");	   
      }

      // send ACK to server
      acknum++;
      memcpy(hdrbuf, &acknum, HDRSIZE);      
      n = sendto(sockfd, hdrbuf, strlen(hdrbuf), 0, &serveraddr, serverlen);
      if (n < 0) 
	      error("ERROR in sendto");

      strcat(filebuf, payloadbuf);
    }
    printf("Successfuly received file! File contents:\n%s\n", filebuf);

    /* store file contents into new file */
    FILE *fp;
    strcat(filename, "-client");
    fp = fopen(filename, "w+");
    if (fp == NULL)
	    error("ERROR in fopen");
    fputs(filebuf, (FILE*) fp);
    return 0;
}
