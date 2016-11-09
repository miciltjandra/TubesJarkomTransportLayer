#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <fstream>

#include "dcomm.h"

//Global Variables

int socket_fd, port_number;
struct hostent *server;
struct sockaddr_in server_address;
char last_signal = XON;

void *get_signal(void* thread_id);

int main(int argc, char* argv[]) {
	//check if all arguments provided
	if (argc < 4) {
		fprintf(stderr, "ERROR. Usage: %s IP/Hostname port file", argv[0]);
		exit(0);
	}

	//get port number
	port_number = atoi(argv[2]);
	printf("%d\n", port_number);

	//create socket
	printf("Membuat socket untuk koneksi ke %s:%s\n", argv[1], argv[2]);
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0) {
		fprintf(stderr, "Socket error");
		exit(0);
	}

	//set server_address
	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server = gethostbyname(argv[1]);
	if (server == 0) {
		fprintf(stderr, "Unknown host");
		exit(0);
	}
	bcopy((char *)server->h_addr, 
        (char *)&server_address.sin_addr,
         server->h_length);
	server_address.sin_port = htons(port_number);

	//size of server address
	unsigned int length = sizeof(server_address);

	//create a new thread
	pthread_t child;
	int thr = pthread_create(&child, NULL, get_signal, NULL);
	if (thr) {
		fprintf(stderr, "Cannot create thread");
		exit(0);
	}

	//Membuka file yang akan dikirim
	std::ifstream f(argv[3], std::ifstream::binary);
	if (!f) {
		fprintf(stderr, "Error: file not found");
		exit(0);
	} else {
		printf("Opening file %s\n", argv[3]);
	}

	//byte ke berapa yang dikirim
	int count = 1;

	//buffer untuk mengirim
	char buf[2];
	int n;

	//baca hingga EOF
	while(f.get(buf[0])) {
		while (last_signal == XOFF) { //jika belum mendapat XON
			printf("Menunggu XON...\n");
			sleep(1);
		}
		//kirim buffer
		printf("Mengirim byte ke-%d: ‘%c’\n", count, buf[0]);
		count++;
		n = sendto(socket_fd,buf,
	    1,0,(const struct sockaddr *)&server_address,length);
	    usleep(200);
	}
	buf[0] = Endfile;
	printf("Mengirim byte ke-%d: Endfile\n", count);
	n = sendto(socket_fd,buf,
	    1,0,(const struct sockaddr *)&server_address,length);

	//sleep(10000);
	return 0;
}

void *get_signal(void* thread_id) {
	struct sockaddr_in from;
	unsigned int length = sizeof(from);
	while (true) {
    	char buffer[MAXLEN];
		int n = recvfrom(socket_fd, buffer, MAXLEN, 0, (struct sockaddr *)&from, &length);
		if (n < 0) {
			fprintf(stderr, "Error receiving"); 
		}

		if (buffer[0] == XOFF) {
			printf("XOFF diterima.\n");
			last_signal = XOFF;
		}
		else if (buffer[0] == XON) {
			printf("XON diterima.\n");
			last_signal = XON;
		}
  	}
  	pthread_exit(0);
}
