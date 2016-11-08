#include "dcomm.h"
#include <string.h>
#include <iostream>

#define DELAY 500
#define RXQSIZE 8
#define MIN_UPPERLIMIT 4
#define MAX_LOWERLIMIT 2

using namespace std;

Byte rxbuf[RXQSIZE];
QTYPE rcvq = {0, 0, 0, RXQSIZE, rxbuf};
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = false;
Boolean send_xoff = false;

char buffer[2]; //for XON/XOFF signal
struct sockaddr_in clientAddr; //sockaddr client

//Socket
int sockfd;

//Declare Function
static Byte *q_get(QTYPE*, Byte*);

/* q_get returns a pointer to the buffer where data is read, or NULL if buffer is empty.*/
static Byte *q_get(QTYPE *queue, Byte *data)
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

	//update queue after save data to current, count --
	(queue->front) = (((queue->front) + 1) % RXQSIZE + 1) -1;
	(queue->count)--;

	//If the number of characters in the receive buffer is below certain level, then send XON.
    if ((queue->count < MIN_UPPER) && (!send_xon)){
        printf ("Buffer is below MINUPPER, sending XON");
        send_xon = true; //xon flag true
        send_xoff = false; //xoff flag false
        sent_xonxoff = XON;

        buffer[0] = sent_xonxoff;
        int a = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

        if (a > 0){
            cout >> "XON SIGNAL SENT\n";
        }
        else{
            cout >> "XON SIGNAL NOT SENT\n";
        }
    }
    return current;
}
