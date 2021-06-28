#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fosfor.h"
#include "readDatagram.h"

#define BUFSIZE 2048 // MTU is probably 1500, so something larger than that

// error - wrapper for perror
void error(char *msg)
{
    perror(msg);
    exit(1);
}

void readDatagram(unsigned short portNumber, orderBook *orderBooks, int *numberOfOrderBooks)
{
    int socketFiledescriptor;         /* socket */
    socklen_t clientAddressLength;    /* byte size of client's address */
    struct sockaddr_in serverAddress; /* server's addr */
    struct sockaddr_in clientAddress; /* client addr */
    struct hostent *clientHostInfo;   /* client host info */
    char messageBuffer[BUFSIZE];      /* message buffer */
    char *hostaddrp;                  /* dotted decimal host addr string */
    int setsockoptFlags;              /* flag value for setsockopt */
    int messageSize;                  /* message byte size */

    // socket: create the parent socket
    socketFiledescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFiledescriptor < 0)
    {
        error("ERROR opening socket");
    }

    // setsockopt: Lets us have multiple services listening on the same port
    setsockoptFlags = 1;
    setsockopt(socketFiledescriptor, SOL_SOCKET, SO_REUSEADDR, (const void *)&setsockoptFlags, sizeof(int));

    // build the server's Internet address
    bzero((char *)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(portNumber);

    // bind: associate the parent socket with a port
    if (bind(socketFiledescriptor, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) < 0)
    {

        error("ERROR on binding");
    }
    //  unsigned int * restrict}
    // main loop: wait for a datagram, then echo it
    clientAddressLength = sizeof(clientAddress);
    while (1)
    {

        // recvfrom: receive a UDP datagram from a client
        bzero(messageBuffer, BUFSIZE);
        messageSize = recvfrom(socketFiledescriptor, messageBuffer, BUFSIZE, 0,
                               (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (messageSize < 0)
        {
            error("ERROR in recvfrom");
        }

        // gethostbyaddr: determine who sent the datagram
        clientHostInfo = gethostbyaddr((const char *)&clientAddress.sin_addr.s_addr,
                                       sizeof(clientAddress.sin_addr.s_addr), AF_INET);
        if (clientHostInfo == NULL)
        {
            error("ERROR on gethostbyaddr");
        }
        hostaddrp = inet_ntoa(clientAddress.sin_addr);
        if (hostaddrp == NULL)
        {
            error("ERROR on inet_ntoa\n");
        }
        printf("server received datagram from %s (%s)\n", clientHostInfo->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", (int)strlen(messageBuffer), messageSize, messageBuffer);

        // sendto: echo the input back to the client
        messageSize = sendto(socketFiledescriptor, messageBuffer, strlen(messageBuffer), 0,
                             (struct sockaddr *)&clientAddress, clientAddressLength);
        if (messageSize < 0)
        {
            error("ERROR in sendto");
        }
    }
}
