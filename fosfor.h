#ifndef FOSFOR_H_INCLUDED
#define FOSFOR_H_INCLUDED

#include <inttypes.h>

#define FALSE 0
#define TRUE 1
#define BID 0
#define ASK 1
#define MAXIMUMORDERBOOKS 4096
#define DEFAULTORDERALLOCATIONSIZE 65536

struct orderStruct
{
    int id;
    int timeToLive;
    int volume;
    int userId;
    int userReference;
    int businessDate; // The business date that the order was placed.
};
typedef struct orderStruct order;

struct priceTickStruct
{
    int orderArraySize;  // Size (in entries) of the array containing orders.
    int firstOrderIndex; // Index of the first order in line to be executed.
    int numberOfOrders;  // All orders, including zero-volume orders.
    int activeOrders;    // All non-zero volume orders.
    int volume;          // Total volume of all orders in the price tick.
    order *order;
};
typedef struct priceTickStruct priceTick;

struct orderBookStruct
{
    int priceTickOffset; // Lowest price tick defined.
    int priceTicks;      // Total number of allocated price ticks.
    int orderBufferAllocationSize;
    int orderCounter;    // Number of orders that have been entered.
    priceTick *price[2]; // Bid and ask sides.
    int topOfTheBook[2]; // Top of the book for bid and ask sides.
    int marketState;     // Closed, Halted, Continous trading, Auction, etc.

    // Static information below:
    int id;
    char name[16];
    int timeZone;
};
typedef struct orderBookStruct orderBook;

// Function declarations.
void printOrderBookDepth(orderBook *oBook);
void initOrderBook(orderBook *oBook, int priceTickOffset, int priceTicks, int orderCounter);
void allocateNewOrderBuffer(orderBook *oBook, priceTick *tick); // trying this, not sure why
void printPriceTick(orderBook *oBook, priceTick *tick);
int addOrderToTick(orderBook *oBook, int side, int price, int volume, int timeToLive);
int growPriceTicks(orderBook *oBook, int newTopPrice);
int matchOrder(orderBook *oBook, int side, int price, int volume, int timeToLive);
void reduceVolumeInOrder(priceTick *tick, int volume, int realIndex); // trying this, not sure why
void changeTopOfTheBook(orderBook *oBook, int side, int price);       // trying this, not sure why
void cancelOldOrders(orderBook *oBook, int currentTime);
int modifyOrderVolume(orderBook *oBook, int side, int price, int volume, int id);
orderBook *findOrderBook(orderBook *orderBooks, int numberOfOrderBooks, int id);
int decodeUnsignedInteger(uint32_t *output, char *inputString);
int decodeSignedInteger(int *output, char *inputString);
orderBook *addOrderBook(int *numberOfOrderBooks, orderBook *orderBooks, int priceTickOffset, int priceTicks, int orderBufferAllocationSize, int id, char *name);
int deleteOrderBook(int *numberOfOrderBooks, orderBook *orderBooks, int id);
int sanitizeText(char *text, int length);
int processCommand(char *commandInput, orderBook *orderBooks, int *numberOfOrderBooks);
int readCommandsFromFile(char filename[128], orderBook *orderBooks, int *numberOfOrderBooks);

#endif
