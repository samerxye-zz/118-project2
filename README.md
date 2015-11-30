# CS118 project 2
## Sam Ye / Henry Lee

#### Requirements:
- client requests file from server, server delivers it.
- SR or GBN protocl (GBN probably easier: simply drop out-of-order packets)
- reliable data delivery.
- header info is up to us.
- message format is up to us
- emulate packet loss (define variable Pl)
- emulate packet corruption (define variable Pc)


## TODO:
1. implement header info and append to each packet
  - What info do we need for **reliable** delivery? for GBN?
  - CURRENTLY IMPLEMENTED: 
    - ACK# from client to server is the SEQ# of the next expected packet
    - SEQ# from server to client is the SEQ# of the current packet  
  - Implement window size
    - two processes: one child receiving acks, one sending packets?
    - how to simultaneously read packets and send...
    - packets have ack and seq number....
    	    
2. implement handshake
  - SYN, SYNACK, END
  - close also? FIN, ACK
3. emulate packet loss
   