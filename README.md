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
  a. ACK#
     - First implement stop-and-wait. Only send out next packet upon receiving an ACK. (ie. window size = 1)
     - Implement Seq#
     - Implement window size
2. implement handshake
  - SYN, SYNACK, END
  - close also? FIN, ACK
3. emulate packet loss
   