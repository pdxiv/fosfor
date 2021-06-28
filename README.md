# Fosfor

Fosfor is a "matching engine" for connecting sellers to buyers of formally defined things. It is very small and relatively fast (being written in C). 

## Status

It is currently not feature complete enough to be used for actual production-like scenarios. Here are some of the major TO-DOs:

* Currently doesn't implement a "modify order" command, to modify order volume or cancel an order.
* Timeout of orders is currently not implemented.
* Doesn't send messages for confirmation of order matches.

## Instructions

To build:

```bash
rm fosfor ; gcc -Wall main.c fosfor.c readDatagram.c -o fosfor && ./fosfor
```

To send information with udp broadcast (assuming your network subnet is 192.168.10.0/24):

```bash
nc -b -u 192.168.10.255 2323
```

## Command data types

| Data type      | Bytes | Encodes to                                         |
|----------------|-------|----------------------------------------------------|
| Signed integer | 10    | 32 bit signed integer from -999999999 to 999999999 |
| Character      | 1     | 8859-1/Latin-1 characters                          |

## Commands

All the events in the matching engine are done with commands. The type of command is denoted with a single upper-case alphabetic character.

### C: Create order book

This will create an order book, to put orders in. Every order needs to be in an order book.

| Field name                | Data type      | Description                                                |
|---------------------------|----------------|------------------------------------------------------------|
| priceTickOffset           | Signed integer | The lowest price in the order book                         |
| priceTicks                | Signed integer | Number of price ticks between lowest and highest           |
| orderBufferAllocationSize | Signed integer | How much initial space a price tick should have for orders |
| orderBookId               | Signed integer | Unique ID code of the order book                           |
| name                      | Character[16]  | Unique name of the orderbook                               |

### A: Add order

This will add an order to an order book.

If timeToLive has a value of 0, the order will be a market order of type IOC (immediate or cancel, attempt to match all or part immediately and then immediately cancel any unfilled portion of the order).

| Field name    | Data type          | Description                                        |
|---------------|--------------------|----------------------------------------------------|
| userId        | Signed integer     | The unique ID of the user placing the order        |
| userReference | Signed integer     | The user's unique internal reference for the order |
| orderBookId   | Signed integer     | ID code of the order book                          |
| side          | Character[1] (B/A) | Bid or Ask                                         |
| price         | Signed integer     | Which price tick to place the order in             |
| volume        | Signed integer     | Number of units user wants to buy or sell          |
| timeToLive    | Signed integer     | How long before the order is deleted               |

### D: Delete order book

This will delete an order book. All orders in the deleted order book and their information will disappear.

| Field name  | Data type      | Description               |
|-------------|----------------|---------------------------|
| orderBookId | Signed integer | ID code of the order book |

## Unimplemented Commands

Some commands are yet to be implemented.

### M: Modify order

The combination of userId, userReference, side and price fields are used when a user wants to modify or delete an order that has been placed. It's up to the user to keep track of these fields. The user will need to specify a value for volume and timeToLive, but a negative value will keep the old value. A value of 0 in either volume or timeToLive will delete the order from the order book. The price of an order can not be modified.

| Field name    | Data type       | Description                                        |
|---------------|-----------------|----------------------------------------------------|
| userId        | Signed integer  | The unique ID of the user placing the order        |
| userReference | Signed integer  | The user's unique internal reference for the order |
| orderBookId   | Signed integer  | ID code of the order book                          |
| side          | Character (B/A) | Bid or Ask                                         |
| volume        | Signed integer  | Number of units user wants to buy or sell          |
| timeToLive    | Signed integer  | How long before the order is deleted               |
