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

### c: Create order book

This will create an order book, to put orders in. Every order needs to be in an order book.

| Field name                | Data type | Description                                                |
|---------------------------|-----------|------------------------------------------------------------|
| priceTickOffset           | int 32    | The lowest price in the order book                         |
| priceTicks                | int 32    | Number of price ticks between lowest and highest           |
| orderBufferAllocationSize | int 32    | How much initial space a price tick should have for orders |
| orderBookId               | int 32    | Unique ID code of the order book                           |
| name                      | fixstr 16 | Unique name of the orderbook                               |

Example of order book creation in roughly equivalent YAML format:

```yaml
---
- "c" # commandType (positive fixint)
- 0 # priceTickOffset (int32)
- 1000 # priceTicks (int32)
- 10 # orderBufferAllocationSize (int32)
- 234 # orderBookId (int32)
- "ACME megacorp" # name (fixstr 16)
```

### a: Add order

This will add an order to an order book.

If `timeToLive` has a value of 0, the order will be a "market order" of type IOC (immediate or cancel, attempt to match all or part immediately and then immediately cancel any unfilled portion of the order). If the combination of `userId`, `userReference`, `OrderBookId`, `side` and `price` are not unique, the behavior should be treated as "undefined".

| Field name    | Data type             | Description                                        |
|---------------|-----------------------|----------------------------------------------------|
| userId        | int 32                | The unique ID of the user placing the order        |
| userReference | int 32                | The user's unique internal reference for the order |
| orderBookId   | int 32                | ID code of the order book                          |
| side          | positive fixint (B/A) | Bid or Ask                                         |
| price         | int 32                | Which price tick to place the order in             |
| volume        | int 32                | Number of units user wants to buy or sell          |
| timeToLive    | int 32                | How long before the order is deleted               |

Example of adding a "bid" order in roughly equivalent YAML format:

```yaml
---
- "a" # commandType (positive fixint)
- 123 # userId (int32)
- 789 # userReference (int32)
- 234 # orderBookId (int32)
- "B" # side (positive fixint (B/A))
- 10 # price (int32)
- 2 # volume (int32)
- 130 # timeToLive (int32)
```

### d: Delete order book

This will delete an order book. All orders in the deleted order book and their information will disappear.

| Field name  | Data type | Description               |
|-------------|-----------|---------------------------|
| orderBookId | int 32    | ID code of the order book |

Example of deleting an order book in roughly equivalent YAML format:

```yaml
---
- "d" # commandType (positive fixint)
- 234 # orderBookId (int32)
```

### m: Modify order (not yet implemented!)

The combination of `userId`, `userReference`, `orderBookId`, `side` and `price` fields are used when a user wants to modify or delete an order that has been placed. It's up to the user to keep track of these fields. The user will need to specify a value for volume and timeToLive, but a negative value will keep the old value. A value of 0 in either `volume` or `timeToLive` will delete the order from the order book. The price of an order can not be modified. A deleted order can not be modified further.

| Field name    | Data type             | Description                                        |
|---------------|-----------------------|----------------------------------------------------|
| userId        | int 32                | The unique ID of the user placing the order        |
| userReference | int 32                | The user's unique internal reference for the order |
| orderBookId   | int 32                | ID code of the order book                          |
| side          | positive fixint (B/A) | Bid or Ask                                         |
| price         | int 32                | Which price tick the order is placed in            |
| volume        | int 32                | Number of units user wants to buy or sell          |
| timeToLive    | int 32                | How long before the order is deleted               |

Example of deleting an order in roughly equivalent YAML format:

```yaml
---
- "m" # commandType (positive fixint)
- 123 # userId (int32)
- 789 # userReference (int32)
- 234 # orderBookId (int32)
- "B" # side (positive fixint (B/A))
- 10 # price (int32)
- 0 # volume (int32)
- -1 # timeToLive (int32)
```
