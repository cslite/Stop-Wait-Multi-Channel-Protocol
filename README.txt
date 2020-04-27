| Computer Networks Assignment- Q1	|
| Submitted by: 					|
| Tussank Gupta					 	|
| 2016B3A70528P					 	|
-------------------------------------

-----
Files
-----
arqClient.c
arqServer.c
utils.c
utils.h
packet.h
config.h
makefile
README.txt
client (executable binary)
server (executable binary)

---------
Execution
---------

Step 1: make
(This will compile and generate executables for client and server)

Step 2: ./server output.txt
('output.txt' here is the name of the output file which is taken as a command line argument)

Step 3: ./client input.txt
('input.txt' here is the name of the input file which is taken as a command line argument)

Input file name and output file name is given as command line argument to client and server respectively as described in above steps.

----------------------
DEFAULT SPECIFICATIONS
----------------------
These default settings can easily be modified in the file 'config.h'. You need to use 'make' after making any changes to this config file.

1. PACKET_SIZE = 100
(Number of bytes of payload in a packet)
2. TIMEOUT_MILLISECONDS = 2000
(The time after which a packet is retransmitted if ACK is not received. Please put this time in milliseconds.)
3. PACKET_DROP_RATE = 10
(The percentage of packets which are randomly dropped by the server. Put a value between 0 and 100)
4. NUM_PACKETS_IN_BUFFER = 10
(Number of out-of-order packets that can be stored in the server's buffer at a time. Packets are automatically dropped when the buffer is full.)

---------------------
DEFAULT CONFIGURATION
---------------------
These default settings can easily be modified in the file 'config.h'. You need to use 'make' after making any changes to this config file.

1. SERVER_PORT = 9002
(The port on which server will be accepting connections.)
2. SERVER_IP_ADDR = "127.0.0.1"
(Server's IP Address. This IP is used when running server and client on the same machine.)

---------------------
IMPORTANT ASSUMPTIONS
---------------------

1. Buffer size is limited and can be changed in 'config.h'. So, any out-of-order packets received once the buffer is full, will automatically be dropped regardless of the PACKET_DROP_RATE. This is to have a real world scenario where buffer space is limited.
2. As the sequence numbers represent the offset in the input file, It is assumed that input file will not be so large, that it becomes impossible to represent its sequence number in an 'unsigned int'.
3. Having a PACKET_DROP_RATE of 0 will not ensure that no packets will be dropped as packets can be dropped even when buffer is full. Adjust buffer size in 'config.h' to adjust packets dropping due to buffer overflow.

-----------
METHODOLOGY
-----------

Server makes one listening socket and accepts two clients. Client makes two sockets to maintain two connections.

To drop a packet, server generates a random integer in the range [0,100) and drops the packet if this random integer is less than or equal to the PACKET_DROP_RATE.

To handle multiple connections at Client as well as server, 'select' is used with FDs of two connection sockets in its read FD-SET.

To handle timeouts and retransmission, the client keeps for each channel a copy of the last sent packet and the time at which it was sent. In every iteration, The select call is given the timeout value as minimum of the two channel's timeout value in its timeout argument and whenever a select call returns due to a timeout, both channels' last sent packets are checked for any timeouts by taking a difference of current time and the send time (which was noted earlier). Timed out packets are retransmitted and their sent time is reset with the current time. 

In normal cases, select returns when one of the socket is available for reading, ACK Packet is read, and last sent for that channel is reset. Then, a new data packet is prepared (if there are more bytes in the input file) and sent over the same channel.

The termination condition for the client program is achieved when both of the channels have no pending unacknowledged packets and the input file has been read completely.

Maintaing the Buffer:
Buffer is a character array which stores payload from packets and its size is adjustable in 'config.h'.
Since we are dealing with a stop and wait protocol, and the packets are created after an ACK is received, we can have only 1 out-of-order packet at a time. 
A packet's payload is copied to buffer using the following algorithm:

int len = strlen(pkt->payload);
for(int i=0; i<len; i++){
    oooBuffer[(pkt->seq) - nextRequiredOffset + i] = (pkt->payload)[i];
}

Whenver the packet's sequence is same as the nextRequiredOffset, the buffer is emptied and nextRequiredOffset is updated accordingly.

The server is terminated once the last packet of the input file is written to the file.