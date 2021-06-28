#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h> // For using sleep()
#include "fosfor.h"

// Basic order book functions below

// Rather than to pass the same old values to every function as variables, we're
// passing a struct, containing the most common values that functions need. This
// is especially useful for wrapper functions. This way we can simply pass a
// pointer for  the struct and fill it with data.
struct functionParamStruct
{
    int userId, userReference, orderBookId, volume, timeToLive, priceTickOffset,
        priceTicks, orderBufferAllocationSize, side;
    signed int price;
    char *name;
    orderBook *orderBookNow;
    priceTick *tick;
    order *order;
};
typedef struct functionParamStruct functionParam;

// printOrderBookDepth(): Print debugging info about an order book.
// Argument 1: Pointer to an order book.
void printOrderBookDepth(orderBook *oBook)
{
    printf("\"%.16s\" ID: %d\n", oBook->name, oBook->id);
    for (int t = 0; t < oBook->priceTicks; t++)
        for (int u = BID; u <= ASK; u++)
        {
            if (u == 0)
            {
                printf("price: %5d(%5d)\t", t + oBook->priceTickOffset, t);
                printf("bid: %4d:%4d:%7d\t", oBook->price[u][t].numberOfOrders, oBook->price[u][t].activeOrders, oBook->price[u][t].volume);
            }
            else
                printf("ask: %4d:%4d:%7d\n", oBook->price[u][t].numberOfOrders, oBook->price[u][t].activeOrders, oBook->price[u][t].volume);
        }
    printf("price ticks in order book: %d\n", oBook->priceTicks);
    printf("bottom price tick for bid & ask: %d\n", oBook->priceTickOffset);
    printf("top price tick for bid & ask: %d\n", oBook->priceTicks + oBook->priceTickOffset - 1);
    printf("Bid top of the book: %d\n", oBook->topOfTheBook[BID] + oBook->priceTickOffset);
    printf("Ask top of the book: %d\n", oBook->topOfTheBook[ASK] + oBook->priceTickOffset);
}

// initOrderBook(): Initialize a newly defined order book.
// Argument 1: Pointer to a newly defined order book to initialize.
// Argument 2: The lowest price tick the order book should be defined to have.
// Argument 3: The number of allocated price ticks the order book needs.
// Argument 4: Start value for the order counter. Normally this would be 0.
void initOrderBook(orderBook *oBook, int priceTickOffset, int priceTicks, int orderCounter)
{
    oBook->priceTickOffset = priceTickOffset;
    oBook->priceTicks = priceTicks;
    oBook->orderCounter = orderCounter;
    oBook->topOfTheBook[BID] = 0;
    oBook->topOfTheBook[ASK] = priceTicks - 1;
    if (priceTicks == 0)
        oBook->topOfTheBook[ASK] = 0; // Ugly hack...
    // Allocate memory for price ticks.
    oBook->price[BID] = (priceTick *)malloc(sizeof(priceTick) * oBook->priceTicks);
    oBook->price[ASK] = (priceTick *)malloc(sizeof(priceTick) * oBook->priceTicks);
    for (int u = 0; u < 2; u++)
        for (int t = 0; t < oBook->priceTicks; t++)
        {
            oBook->price[u][t].orderArraySize = 0;
            oBook->price[u][t].firstOrderIndex = 0;
            oBook->price[u][t].numberOfOrders = 0;
            oBook->price[u][t].order = NULL;
            oBook->price[u][t].activeOrders = 0;
            oBook->price[u][t].volume = 0;
        }
}

// allocateNewOrderBuffer(): Grow tick buffer for a price tick.
// Argument 1: Pointer to order book.
// Argument 2: Pointer to price tick that we need to reallocate memory buffer for.
//
// This is used when we need to grow the order buffer for a price tick.
// For shrinking an price tick order buffer we may need to design a separate
// function, or modify this one to do this intelligently.
void allocateNewOrderBuffer(orderBook *oBook, priceTick *tick)
{
    int bufferChunk = oBook->orderBufferAllocationSize;
    int newBufferSize = bufferChunk * ((tick->orderArraySize / bufferChunk) + 1);
    printf("-W-: Price tick order buffer too small (%d). Allocating larger one.\n", tick->orderArraySize);
    order *newOrder = (order *)malloc(sizeof(order) * newBufferSize);
    // Copy data from the old tick buffer to the new one.
    printf("-D-: Copy data from old tick buffer to new one...\n");
    for (int t = 0; t < tick->orderArraySize; t++)
    { // Copy old array to new one.
        int realIndex = (t + tick->firstOrderIndex) % tick->orderArraySize;
        newOrder[t].volume = tick->order[realIndex].volume;
        newOrder[t].id = tick->order[realIndex].id;
        newOrder[t].timeToLive = tick->order[realIndex].timeToLive;
    }
    tick->firstOrderIndex = 0;
    free(tick->order);
    tick->orderArraySize = newBufferSize;
    tick->order = newOrder;
}

// printPriceTick(): Print the content of a price tick.
// Argument 1: Pointer to order book.
// Argument 2: Pointer to the price tick we want to print.
void printPriceTick(orderBook *oBook, priceTick *tick)
{
    printf("First order index: %d\tNumber of orders: %d\n", tick->firstOrderIndex, tick->numberOfOrders);
    for (int t = 0; t < tick->numberOfOrders; t++)
    {
        int realIndex = (t + tick->firstOrderIndex) % tick->orderArraySize;
        printf("index: %d\t", realIndex);
        printf("i: %d\tv: %d\tt:%d\n", tick->order[realIndex].id, tick->order[realIndex].volume, tick->order[realIndex].timeToLive);
    }
}

// addOrderToTick(): Add a new order to a price tick.
// Argument 1: Pointer to order book.
// Argument 2: Bid/ask side (0/1).
// Argument 3: Price tick.
// Argument 4: Volume.
// Argument 5: Time to live.
// Returns: Order ID of the order that has been placed.
//
// This would typicallly be used:
// - in a pre-trading/auction phase.
// - after matching has been completed with volume remaining.
int addOrderToTick(orderBook *oBook, int side, int price, int volume, int timeToLive)
{
    // If the new price is larger than the number of allocated price ticks, allocate more!
    if (price > oBook->priceTicks)
        growPriceTicks(oBook, price);
    priceTick *tick = &(oBook->price[side][price]);
    // Do we need to allocate a larger price tick buffer? If so, we also need to
    // copy the old buffer data into the new buffer.
    if (tick->orderArraySize < tick->numberOfOrders + 1)
        allocateNewOrderBuffer(oBook, tick);
    order *currentOrder = &tick->order[(tick->firstOrderIndex + tick->numberOfOrders) % tick->orderArraySize];
    // Change "top of the book".
    if (tick->volume == 0 && volume != 0)
    {
        if (side == 0)
        {
            if (oBook->topOfTheBook[side] < price)
                oBook->topOfTheBook[side] = price;
        }
        else if (oBook->topOfTheBook[side] > price)
            oBook->topOfTheBook[side] = price;
    }
    // Add new order volume to price tick volume.
    tick->volume = tick->volume + volume;
    currentOrder->volume = volume;
    currentOrder->timeToLive = timeToLive;
    tick->numberOfOrders++;
    tick->activeOrders++;
    currentOrder->id = oBook->orderCounter;
    oBook->orderCounter++;
    return currentOrder->id;
}

// growPriceTicks(): Allocate new price ticks if they aren't high enough.
// Argument 1: Pointer to order book.
// Argument 2: Value of new upper price.
// Returns: 0 - Error.
// Returns: 1 - Execution successful.
int growPriceTicks(orderBook *oBook, int newTopPrice)
{
    printf("-D-: Running growPriceTicks()\n");
    printf("-D-: Old no. price ticks: %d\tNew no. price ticks: %d\n", oBook->priceTicks, newTopPrice + 1);
    if (newTopPrice < oBook->priceTicks)
        return FALSE;
    printf("-D-: We're trying to go on...\n");
    priceTick *newTicks[2];
    newTicks[BID] = (priceTick *)malloc(sizeof(priceTick) * (newTopPrice + 1));
    newTicks[ASK] = (priceTick *)malloc(sizeof(priceTick) * (newTopPrice + 1));
    // Copy old price ticks into the new space, at the beginning.
    for (int u = 0; u < 2; u++)
        for (int t = 0; t < oBook->priceTicks; t++)
        {
            newTicks[u][t].orderArraySize = oBook->price[u][t].orderArraySize;
            newTicks[u][t].firstOrderIndex = oBook->price[u][t].firstOrderIndex;
            newTicks[u][t].numberOfOrders = oBook->price[u][t].numberOfOrders;
            newTicks[u][t].order = oBook->price[u][t].order;
            newTicks[u][t].activeOrders = oBook->price[u][t].activeOrders;
            newTicks[u][t].volume = oBook->price[u][t].volume;
        }
    // Initialize the new price ticks to empty values.
    for (int u = 0; u < 2; u++)
        for (int t = oBook->priceTicks; t <= newTopPrice; t++)
        {
            newTicks[u][t].orderArraySize = 0;
            newTicks[u][t].firstOrderIndex = 0;
            newTicks[u][t].numberOfOrders = 0;
            newTicks[u][t].order = NULL;
            newTicks[u][t].activeOrders = 0;
            newTicks[u][t].volume = 0;
        }
    for (int u = 0; u < 2; u++)
    {
        free(oBook->price[u]);
        oBook->price[u] = newTicks[u];
    }
    oBook->priceTicks = newTopPrice + 1;
    printf("-D-: New priceTicks = %d\n", oBook->priceTicks);
    if (oBook->price[ASK][oBook->topOfTheBook[ASK]].activeOrders == 0)
        oBook->topOfTheBook[ASK] = newTopPrice;
    return TRUE;
}

// matchOrder(): Match an order with the order book. If remaining volume, add order to order book.
// Argument 1: Pointer to order book.
// Argument 2: Bid/ask side (0/1).
// Argument 3: Price tick.
// Argument 4: Volume.
// Argument 5: Time to live.
// Returns: Undefined
int matchOrder(orderBook *oBook, int side, int price, int volume, int timeToLive)
{
    // If the new price is larger than the number of allocated price ticks, allocate more!
    if (price >= oBook->priceTicks)
        printf("-D-: We need to grow price ticks\n");
    if (price >= oBook->priceTicks)
        growPriceTicks(oBook, price);
    int oppositeSide;
    if (side == 0)
        oppositeSide = 1;
    else
        oppositeSide = 0;
    int topOfTheBook = oBook->topOfTheBook[oppositeSide];
    // Iterate until volume of new is 0 or there are no more orders to match against.
    while (oBook->price[oppositeSide][topOfTheBook].volume > 0 && volume != 0)
    {
        if (topOfTheBook > price && side == 0)
            break;
        if (topOfTheBook < price && side == 1)
            break;
        priceTick *tick = &oBook->price[oppositeSide][topOfTheBook];
        order *currentOrder = &tick->order[tick->firstOrderIndex];
        if (currentOrder->volume - volume <= 0)
        { // Is the price of the order in the book smaller than our order?
            volume = volume - currentOrder->volume;
            modifyOrderVolume(oBook, oppositeSide, topOfTheBook, 0, currentOrder->id);
            printf("-D-: We completely emptied the volume of an order in the book\n");
        }
        else
        {
            modifyOrderVolume(oBook, oppositeSide, topOfTheBook, currentOrder->volume - volume, currentOrder->id);
            volume = 0;
        }
        topOfTheBook = oBook->topOfTheBook[oppositeSide];
    }
    // If time to live == 0, we have a market order.
    if (timeToLive > 0 && volume > 0)
        addOrderToTick(oBook, side, price, volume, timeToLive);
    return TRUE;
}

// reduceVolumeInOrder(): Generic function for reducing the volume of an order.
// Argument 1: Pointer to a bid/ask price tick.
// Argument 2: Volume.
// Argument 3: Index pointing to a specific order in the order array.
void reduceVolumeInOrder(priceTick *tick, int volume, int realIndex)
{
    // Subtract price tick volume with difference between new and old order.
    tick->volume = tick->volume - tick->order[realIndex].volume + volume;
    tick->order[realIndex].volume = volume;
    if (volume == 0)
    { // If volume is zero, cancel order.
        tick->activeOrders--;
        if (realIndex == tick->firstOrderIndex)
        { // If order is in front of queue, move queue index.
            tick->firstOrderIndex = (tick->firstOrderIndex + 1) % tick->orderArraySize;
            tick->numberOfOrders--;
        }
    }
}

// changeTopOfTheBook(): Change "top of the book" if a price tick has 0 volume.
// Argument 1: Pointer to an order book.
// Argument 2: Bid/ask side (0/1).
// Argument 3: Price.
void changeTopOfTheBook(orderBook *oBook, int side, int price)
{
    if ((oBook->price[side][price].volume == 0) && (oBook->topOfTheBook[side] == price))
    {
        if (side == 0)
        {
            int oldTopOfTheBook = oBook->topOfTheBook[side];
            oBook->topOfTheBook[BID] = 0;
            for (int u = oldTopOfTheBook - 1; u >= 0; u--)
            {
                if (oBook->price[side][u].activeOrders > 0)
                {
                    oBook->topOfTheBook[side] = u;
                    break;
                }
            }
        }
        else
        {
            side = 1;
            oBook->topOfTheBook[side] = oBook->priceTicks - 1;
            for (int u = price + 1; u < oBook->priceTicks; u++)
            {
                if (oBook->price[side][u].activeOrders > 0)
                {
                    oBook->topOfTheBook[side] = u;
                    break;
                }
            }
        }
    }
}

// cancelOldOrders(): Cancel expired orders by setting their volume to 0.
// Argument 1: Pointer to an order book.
// Argument 2: Time that we want to use as reference when removing old orders.
void cancelOldOrders(orderBook *oBook, int currentTime)
{
    for (int t = 0; t < oBook->priceTicks; t++)
    {
        for (int u = BID; u <= ASK; u++)
        {
            priceTick *tick = &(oBook->price[u][t]);
            for (int v = 0; v < tick->numberOfOrders; v++)
            {
                int realIndex = (v + tick->firstOrderIndex) % tick->orderArraySize;
                if (tick->order[realIndex].timeToLive <= currentTime)
                {
                    reduceVolumeInOrder(tick, 0, realIndex);
                    changeTopOfTheBook(oBook, u, t);
                }
            }
        }
    }
}

// modifyOrderVolume(): Modify volume of order.
// Argument 1: Pointer to order book.
// Argument 2: Bid/ask side (0/1).
// Argument 3: Price tick.
// Argument 4: Volume.
// Argument 5: Order ID
// Returns: 0 - Order successfully modified.
// Returns: 1 - Error: order not found in price tick.
// Returns: 2 - Error: new volume not smaller than old volume.
// Returns: 3 - Error: volume cannot be smaller than 0.
int modifyOrderVolume(orderBook *oBook, int side, int price, int volume, int id)
{
    int returnStatus = 1; // Default to "order not found".
    if (volume >= 0)
    {
        priceTick *tick = &(oBook->price[side][price]);
        for (int t = 0; t < tick->numberOfOrders; t++)
        {
            int realIndex = (t + tick->firstOrderIndex) % tick->orderArraySize;
            if (tick->order[realIndex].id == id)
            {
                if (volume <= tick->order[realIndex].volume)
                { // Only allowed to shrink volume.
                    reduceVolumeInOrder(tick, volume, realIndex);
                    changeTopOfTheBook(oBook, side, price);
                    returnStatus = 0; // We found the order and we successfully modified it.
                }
                else
                    returnStatus = 2; // Error: new volume not smaller than old volume.
            }
        }
    }
    else
        returnStatus = 3; // Error: volume cannot be smaller than 0.
    return returnStatus;
}

// findOrderBook(): find an order book in an array by ID.
// Argument 1: Pointer to order book array.
// Argument 2: Number of order books in array.
// Argument 3: Order book ID that we want to find.
// Returns: Pointer to a found order book. Otherwise returns NULL.
orderBook *findOrderBook(orderBook *orderBooks, int numberOfOrderBooks, int id)
{
    for (int t = 0; t < numberOfOrderBooks; t++)
        if (orderBooks[t].id == id)
            return &orderBooks[t];
    return NULL;
}

// Command parser functions below

// decodeSignedInteger(): Decode 10 character signed integer between -999999999 & 999999999
// Argument 1: Pointer to an integer for output.
// Argument 2: Pointer to a 10 character string.
// Returns: 0 - Decoding failed.
// Returns: 1 - Decoding completed successfully.
int decodeSignedInteger(int *output, char *inputString)
{
    int readMode = 0; // 0: start. 1: reading digits.
    int isNegative = FALSE;
    *output = 0;
    // Something is wrong if the first character in the number isn't a space or a minus
    if (inputString[0] != ' ' && inputString[0] != '-')
    {
        return FALSE;
    }
    for (int t = 0; t < 10; t++)
    {
        char thisChar = inputString[t];
        if ((thisChar >= '0' && thisChar <= '9'))
        {
            readMode = 1;
            *output *= 10;
            *output += thisChar - '0';
        }
        else if ((readMode == 0) && thisChar == '-')
        {
            readMode = 1;
            isNegative = TRUE;
        }
        else if (readMode == 1)
        {
            return FALSE;
        }
        if (readMode == 0 && thisChar != ' ')
        {
            return FALSE;
        }
    }
    if (readMode == 1)
    {
        if (isNegative)
        {
            *output = -*output;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// addOrderBook(): A new order book to the process.
// Argument 1: Pointer to variable that keeps track of how many order books.
// Argument 2: Pointer to array of order books that we want to add to.
// Argument 3: Offset for mapping an array to price ticks.
// Argument 4: Number of price ticks that we want to use.
// Argument 5: Size of the chunk that we allocate order buffers in.
//             (Bigger improves performance. Can be dynamically changed later if
//             the need arises.)
// Argument 6: Unique order book ID number.
// Returns 1: Pointer to newly created order book.
// Returns 2: NULL pointer, for error condition.
orderBook *addOrderBook(int *numberOfOrderBooks, orderBook *orderBooks, int priceTickOffset, int priceTicks, int orderBufferAllocationSize, int id, char *name)
{
    printf("-D-: Attempting to add order book %d\n", id);
    if (*numberOfOrderBooks >= MAXIMUMORDERBOOKS)
    {
        printf("-D-: Order book limit exceeded. Failed to add order book %d\n", id);
        return NULL;
    }
    if (findOrderBook(orderBooks, *numberOfOrderBooks, id) != NULL)
    {
        printf("-D-: Failed to add pre-existing order book %d\n", id);
        return NULL;
    }
    if (orderBufferAllocationSize <= 0)
    {
        printf("-D-: Illegal order buffer allocation size: %d. must greater than 1. Failed to add order book %d\n", orderBufferAllocationSize, id);
        return NULL;
    }
    (*numberOfOrderBooks)++;
    initOrderBook(&orderBooks[*numberOfOrderBooks - 1], priceTickOffset, priceTicks, 0);
    orderBooks[*numberOfOrderBooks - 1].orderBufferAllocationSize = orderBufferAllocationSize;
    orderBooks[*numberOfOrderBooks - 1].id = id;
    strcpy(orderBooks[*numberOfOrderBooks - 1].name, name);
    sanitizeText(orderBooks[*numberOfOrderBooks - 1].name, 16);
    printf("-D-: Newly created order book name: %s\n", orderBooks[*numberOfOrderBooks - 1].name);
    return &orderBooks[*numberOfOrderBooks - 1];
}

// deleteOrderBook(): Remove an order book previously created by addOrderBook().
// Argument 1: Pointer to variable that keeps track of how many order books.
// Argument 2: Pointer to an array of order books containing our deletee.
// Argument 3: ID for the order book that we want to delete.
// Returns 0: Failed to find order book for deletion.
// Returns 1: Successfully deleted order book.
int deleteOrderBook(int *numberOfOrderBooks, orderBook *orderBooks, int id)
{
    printf("-D-: Attempting to delete order book %d\n", id);
    orderBook *oBook;
    oBook = findOrderBook(orderBooks, *numberOfOrderBooks, id);
    if (oBook == NULL)
    {
        printf("-D-: Failed to delete non-existing order book %d\n", id);
        return FALSE;
    }
    int indexOfOrderBook;
    for (int u = BID; u <= ASK; u++)
    {
        orderBook *oBook = findOrderBook(orderBooks, *numberOfOrderBooks, id);
        oBook = findOrderBook(orderBooks, *numberOfOrderBooks, id);
        // Free allocated memory for order arrays.
        for (int t = 0; t < oBook->priceTicks; t++)
            free((oBook->price[u])[t].order);
        // Free allocated memory for price tick arrays.
        free(oBook->price[u]);
    }
    // Remove the order book from the array.
    indexOfOrderBook = 0;
    for (int t = 0; t < *numberOfOrderBooks; t++)
    {
        if (indexOfOrderBook)
            orderBooks[t - 1] = orderBooks[t];
        if (&(orderBooks[t]) == oBook)
            indexOfOrderBook = 1;
    }
    (*numberOfOrderBooks)--;
    printf("-D-: Successfully deleted order book %d\n", id);
    return TRUE;
}

// sanitizeText(): Only preserve characters with ascii value 32 - 126. Turn
//                 everything including and after a \0 character to blank space.
// Argument 1: String.
// Argument 2: Length of string to be sanitized.
int sanitizeText(char *text, int length)
{
    int nullFlag = FALSE;
    for (int t = 0; t < length; t++)
    {
        if (*text == '\0')
            nullFlag = TRUE;
        if (nullFlag)
            *text = ' ';
        if (*text < 32)
            *text = ' ';
        if (*text > 126)
            *text = ' ';
        text++;
    }
    return TRUE;
}

// processCommand(): Decode protocol text string for matching engine operations.
// Argument 1: String (char array pointer). This contains the data to parse.
// Argument 2: A pointer to an order book. (Shouldn't this be an pointer to order book array??)
// Argument 3: Number of order books in the process.
int processCommand(char *commandInput, orderBook *orderBooks, int *numberOfOrderBooks)
{
    int userId, userReference, orderBookId, volume, timeToLive, priceTickOffset, priceTicks, orderBufferAllocationSize;
    signed int price;
    unsigned int side;
    char *name;
    orderBook *orderBookNow;
    switch (*commandInput)
    {
    case 'A': // Add order.
        commandInput++;
        if (decodeSignedInteger(&userId, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&userReference, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&orderBookId, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if ((orderBookNow = findOrderBook(orderBooks, *numberOfOrderBooks, orderBookId)) == NULL)
        {
            printf("-D-: Failed to add order to non-existant order book %d\n", orderBookId);
            return TRUE;
        }
        if (*commandInput == 'B' || *commandInput == 'A')
        {
            side = 66 - *commandInput;
            commandInput++;
        }
        else
            return FALSE;
        if (decodeSignedInteger(&price, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&volume, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&timeToLive, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (*commandInput == '\n')
            matchOrder(orderBookNow, side, price, volume, timeToLive);
        else
            return FALSE;
        break;
    case 'C': // Create order book.
        commandInput++;
        //C        1        1     4096        1
        if (decodeSignedInteger(&priceTickOffset, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&priceTicks, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&orderBufferAllocationSize, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (decodeSignedInteger(&orderBookId, commandInput))
            commandInput += 10;
        else
            return FALSE;
        name = commandInput;
        commandInput += 16;
        if (*commandInput == '\n')
            addOrderBook(numberOfOrderBooks, orderBooks, priceTickOffset, priceTicks, orderBufferAllocationSize, orderBookId, name);
        else
            return FALSE;
        break;
    case 'D': // Delete order book.
        commandInput++;
        if (decodeSignedInteger(&orderBookId, commandInput))
            commandInput += 10;
        else
            return FALSE;
        if (*commandInput == '\n')
            deleteOrderBook(numberOfOrderBooks, orderBooks, orderBookId);
        else
            return FALSE;
        break;
    default:
        printf("-W-: Invalid command character: %c\n", *commandInput);
        return FALSE;
    }
    return TRUE;
}

// readCommandsFromFile(): Read a file with text protocol data into the protocol parser.
// Argument 1: The name of a file to be read.
// Returns 0: Opening file failed.
// Returns 1: File reading successful.
int readCommandsFromFile(char filename[128], orderBook *orderBooks, int *numberOfOrderBooks)
{
    long unsigned int commandCounter = 0;
    FILE *fileHandle;
    char commandString[128];
    fileHandle = fopen(filename, "rt"); // Open file for reading. "rt" means "read text".
    if (fileHandle == NULL)
        return FALSE;
    while (fgets(commandString, 128, fileHandle) != NULL)
    {
        printf("%s", commandString);
        if (!processCommand(commandString, orderBooks, numberOfOrderBooks))
            printf("-W-: Error parsing command string: %s", commandString);
        commandCounter++;
        printf("-D-: Processed commands: %lu\n", commandCounter);
    }
    fclose(fileHandle);
    return TRUE;
}
