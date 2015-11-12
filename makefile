all: servermake clientmake

servermake: udpserver.c
	gcc udpserver.c -o server -w

clientmake: udpclient.c
	gcc udpclient.c -o client -w
