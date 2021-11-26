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

The data types correspond to the datatypes used in MessagePack

| Data type       | Encodes to                                |
|-----------------|-------------------------------------------|
| int 32          | 32 bit signed integer                     |
| positive fixint | 7 bit unsigned integer or ASCII character |
| fixstr          | 1-31 length byte array                    |

## Commands

All the events in the matching engine are done with commands. Commands are encoded as arrays (fixarray) in the MessagePack data format. The type of command is denoted with a single upper-case alphabetic character.

For example a "Create order book" command could be formulated in roughly equivalent JSON format as:

```json
[
    "c",
    0,
    1000,
    10,
    234,
    "ACME megacorp"
]
```

### c: Create order book

This will create an order book, to put orders in. Every order needs to be in an order book.

| Field name                | Data type | Description                                                |
|---------------------------|-----------|------------------------------------------------------------|
| priceTickOffset           | int 32    | The lowest price in the order book                         |
| priceTicks                | int 32    | Number of price ticks between lowest and highest           |
| orderBufferAllocationSize | int 32    | How much initial space a price tick should have for orders |
| orderBookId               | int 32    | Unique ID code of the order book                           |
| name                      | fixstr 16 | Unique name of the orderbook                               |

### a: Add order

This will add an order to an order book.

If timeToLive has a value of 0, the order will be a market order of type IOC (immediate or cancel, attempt to match all or part immediately and then immediately cancel any unfilled portion of the order).

| Field name    | Data type             | Description                                        |
|---------------|-----------------------|----------------------------------------------------|
| userId        | int 32                | The unique ID of the user placing the order        |
| userReference | int 32                | The user's unique internal reference for the order |
| orderBookId   | int 32                | ID code of the order book                          |
| side          | positive fixint (B/A) | Bid or Ask                                         |
| price         | int 32                | Which price tick to place the order in             |
| volume        | int 32                | Number of units user wants to buy or sell          |
| timeToLive    | int 32                | How long before the order is deleted               |

### d: Delete order book

This will delete an order book. All orders in the deleted order book and their information will disappear.

| Field name  | Data type | Description               |
|-------------|-----------|---------------------------|
| orderBookId | int 32    | ID code of the order book |

### m: Modify order (not yet implemented!)

The combination of userId, userReference, side and price fields are used when a user wants to modify or delete an order that has been placed. It's up to the user to keep track of these fields. The user will need to specify a value for volume and timeToLive, but a negative value will keep the old value. A value of 0 in either volume or timeToLive will delete the order from the order book. The price of an order can not be modified.

| Field name    | Data type             | Description                                        |
|---------------|-----------------------|----------------------------------------------------|
| userId        | int 32                | The unique ID of the user placing the order        |
| userReference | int 32                | The user's unique internal reference for the order |
| orderBookId   | int 32                | ID code of the order book                          |
| side          | positive fixint (B/A) | Bid or Ask                                         |
| volume        | int 32                | Number of units user wants to buy or sell          |
| timeToLive    | int 32                | How long before the order is deleted               |
