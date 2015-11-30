/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define PKTSIZE 64
#define HDRSIZE sizeof(int)
#define PAYLOADSIZE (PKTSIZE-HDRSIZE)
#define WINSIZE 3
#define TIMEOUT 1 //seconds

pthread_mutex_t lock;

/* error - wrapper for perror */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[PKTSIZE]; /* initial message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char packetbuf[PKTSIZE]; // Entire packet's data
  char* hdrbuf = (char*)malloc(HDRSIZE); // Packet header
  char payloadbuf[PKTSIZE]; // Packet payload
  int seqnum = 0; // Sequence number of packet that server is sending
  /* mmap = shared variable across processes */
  int *acknum = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
		     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *acknum = 0; // Sequence number of packet that client is expecting
  

  /* check command line arguments */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* socket: create the parent socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* bind: associate the parent socket with a port */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* main loop: wait for a file request */
  clientlen = sizeof(clientaddr);
  while (1) {
    bzero(buf, PKTSIZE);
    bzero(packetbuf, PKTSIZE);

    /* recvfrom = receive packet */
    n = recvfrom(sockfd, buf, PKTSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* gethostbyaddr: determine who sent the datagram */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("Server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("Server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    /* open and read file. send file contents via packets. */
    FILE *fp;
    fp = fopen(buf, "r");
    if (fp == NULL) 
	    error("ERROR in fopen");

    pid_t pid = fork();

    /* Parent process sends packets */
    if (pid > 0) {
      while (1) {
	// Payload: get file contents relative to sequence number
	if (fseek((FILE*)fp, seqnum*(PAYLOADSIZE-1), SEEK_SET)) error("ERROR in fseek");
	if (fgets(payloadbuf, PAYLOADSIZE, (FILE*)fp) == NULL) {
	  // Only break out of loop if all packets have been ACKed
	  // Even if we reached EOF, in-flight packets can be lost, so we stay in loop
	  pthread_mutex_lock(&lock);
	  if (seqnum == *acknum)  {
	    pthread_mutex_unlock(&lock);		
	    break;
	  }
	  pthread_mutex_unlock(&lock);		
	}
	else {
	  // Header: sequence number
	  memcpy(hdrbuf, &seqnum, HDRSIZE);

	  // Packet = payload + header
	  memcpy(packetbuf, hdrbuf, HDRSIZE);
	  memcpy(packetbuf+HDRSIZE, payloadbuf, PAYLOADSIZE);
	  printf("Packet #%d: %s\n", seqnum, packetbuf+HDRSIZE);
	  
	  // Send packet
	  n = sendto(sockfd, packetbuf, PKTSIZE, 0,
		     (struct sockaddr *) &clientaddr, clientlen);
	  if (n < 0) 
		  error("ERROR in sendto");
	  seqnum++;
	}
	// Wait for window size to open.
	time_t timer = time(0);
	while (1) {
	  pthread_mutex_lock(&lock);
	  if (seqnum < *acknum+WINSIZE) {
	    pthread_mutex_unlock(&lock);		
	    break;
	  }
	  pthread_mutex_unlock(&lock);		
	  // Retransmit on timeout
	  if (TIMEOUT < time(0) - timer) {
	    printf("TIMEOUT: retransmitting\n");
	    pthread_mutex_lock(&lock);
	    seqnum = *acknum;
	    pthread_mutex_unlock(&lock);
	  }
	}
      }
      // Wait for remaining ACK's to arrive
      fclose(fp);
      printf("Successfully sent file!\n");
      kill(pid, SIGKILL); // End child process
    }
    /* Child process receives ACK packets from client */
    else if (pid == 0) { 
      while (1) {
	      //usleep(200000);
	n = recvfrom(sockfd, hdrbuf, HDRSIZE, 0,
		     (struct sockaddr *) &clientaddr, &clientlen);
	if (n < 0) 
		error("ERROR in recvfrom");
	pthread_mutex_lock(&lock);
	memcpy(acknum, hdrbuf, HDRSIZE);
	printf("Expected packet#: %d\n", *acknum);
	pthread_mutex_unlock(&lock);		
      }
    }
    else
      error("ERROR in fork");	 

    /* Let client know that file was successfuly sent */
    seqnum = INT_MAX;
    memcpy(hdrbuf, &seqnum, HDRSIZE);
    n = sendto(sockfd, hdrbuf, HDRSIZE, 0,
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
	    error("ERROR in sendto");
    exit(0);
  }
}
