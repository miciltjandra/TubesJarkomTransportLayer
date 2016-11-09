#include "dcomm.h"
#include <string.h>
#include <iostream>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>

#define DELAY 500
#define RXQSIZE 8
#define MIN_UPPERLIMIT 4
#define MAX_LOWERLIMIT 2

Byte rxbuf[RXQSIZE];
QTYPE rcvq = {0, 0, 0, RXQSIZE, rxbuf};
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = false;
Boolean send_xoff = false;

//Socket
int sockfd, newsockfd, portno;
socklen_t clilen;
char buffer[256];
struct sockaddr_in serv_addr, cli_addr;


//Declare Function
Byte *q_get(QTYPE*, Byte*);
Byte *rcvchar(int sockfd, QTYPE *queue);
void nom_nom_q(QTYPE *queue);
void error(const std::string& msg);



int main(int argc, char *argv[]){
  Byte c;


  /* Insert code here to bind socket to the port number given in argv[1]. */
  if (argc < 2) {
      fprintf(stderr,"ERROR, no port provided\n");
      exit(1);
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
     error("ERROR opening socket");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
           error("ERROR on binding");

  /* Initialize XON/XOFF flags */
  send_xon = true;
  send_xoff = false;

  /* Create child process */

  std::thread nom_nom_engine(nom_nom_q, rxq);

  while(1){
    c = *(rcvchar(sockfd,rxq));
    std::cout << "Receive : " << (int) c << " " << c << std::endl;

    if( c == Endfile ){
      std::terminate();
      exit(0);
    }
  }

  return 0;
}



void nom_nom_q(QTYPE *queue){
  std::chrono::milliseconds nom_nom_speed(2000);
  Byte data;

  while(1){
    /* Call q_get */
    Byte* r = q_get(queue, &data);
    if(r)
      std::cout << "NomNomEngine: Nom Nom " << *r << "\n";
    else
      std::cout << "NomNomEngine: Nothing to Nom Nom\n";

    /* Can introduce some delay here. */
    std::this_thread::sleep_for(nom_nom_speed);
  }
}


Byte* rcvchar(int sockfd, QTYPE *queue){
  /*
  Insert code here.
  Read a character from socket and put it to the receive
  buffer.
  If the number of characters in the receive buffer is above
  certain level, then send XOFF and set a flag (why?).
  Return a pointer to the buffer where data is put.
  */

  int n;

  // allocate buffer
  Byte *buffer = queue->data + queue->rear;

  // recv data
  bzero(buffer,1);

  n = recvfrom(sockfd, buffer, 1, 0, (struct sockaddr *)&cli_addr, &clilen);
  if (n < 0) error("ERROR reading from socket");

  (queue->rear) = (((queue->rear) + 1) % RXQSIZE + 1) -1;
  queue->count ++;

  // check XOFF condition
  if( queue->count > MAX_LOWERLIMIT && !send_xoff){
    std::cout << "Buffer is above MAX_LOWERLIMIT, sending XOFF\n";
    sent_xonxoff = XOFF;

    n = sendto(sockfd, &sent_xonxoff, sizeof(sent_xonxoff), 0, (struct sockaddr *)&cli_addr, clilen);

    if (n > 0){
        std::cout << "XOFF SIGNAL SENT\n";
        send_xoff = true;
        send_xon = false;
    }
    else{
        std::cout << "XOFF SIGNAL NOT SENT\n";
    }

  }

  return buffer;
}



/* q_get returns a pointer to the buffer where data is read, or NULL if buffer is empty.*/
Byte *q_get(QTYPE *queue, Byte* data)
{
    Byte *current;
    if (!queue->count) {
        //buffer is empty
        return (NULL);
    }

    /*
	Increment front index and check for wraparound.
	*/

    //Retrieve data from buffer, save it to "current" and "data"
	current = &queue->data[queue->front];

  *data = *current;

	//update queue after save data to current, count --
	(queue->front) = (((queue->front) + 1) % RXQSIZE + 1) -1;
	(queue->count)--;

	//If the number of characters in the receive buffer is below certain level, then send XON.
  if ((queue->count < MIN_UPPERLIMIT) && (!send_xon)){
      std::cout << "Buffer is below MINUPPER, sending XON\n";
      sent_xonxoff = XON;

      int a = sendto(sockfd, &sent_xonxoff, sizeof(sent_xonxoff), 0, (struct sockaddr *)&cli_addr, clilen);

      if (a > 0){
          std::cout << "XON SIGNAL SENT\n";
          send_xon = true; //xon flag true
          send_xoff = false; //xoff flag false
      }
      else{
          std::cout << "XON SIGNAL NOT SENT\n";
      }
  }
  return current;
}


void error(const std::string& msg){
  std::cerr << msg << std::endl;
  exit(1);
}
