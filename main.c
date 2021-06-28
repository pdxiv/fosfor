#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h> // For using sleep()
#include "fosfor.h"
#include "readDatagram.h"

int main(void)
{
    srand(time(NULL));

    // Create order book data structure
    int numberOfOrderBooks = 0;
    orderBook orderBooks[MAXIMUMORDERBOOKS];

    // "Spin up" base data from file at the start of the session
    readCommandsFromFile("basedata.txt", orderBooks, &numberOfOrderBooks);

    // Show information about each orderbook
    for (int t = 0; t < numberOfOrderBooks; t++)
    {
        orderBook *oBook = findOrderBook(orderBooks, numberOfOrderBooks, orderBooks[t].id);
        if (oBook != NULL)
        {
            printOrderBookDepth(oBook);
        }
        else
        {
            printf("-D-: Unable to display order book info. Can't find order book with id %d.\n", oBook->id);
        }
    }

    // Start receiving commands over UDP
    unsigned short udpListenPort = 2323;
    readDatagram(udpListenPort, orderBooks, &numberOfOrderBooks);

    // sleep(600); // Sleep for 10 minutes, to give time to measure things
    return 0;
}
