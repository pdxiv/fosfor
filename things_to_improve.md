# Things to fix, optimize and/or improve

## High priority

* Implement "modify order" command
* Implement "shutdown" command, to do a clean shutdown. Should include a "time" argument to schedule a shutdown to a specific time (if time is zero, shut down immediately).
* readCommandsFromFile() failing to read a basic data file should shut down the program
* Get rid of the "side" character in "Add order" and "Modify order" commands. It makes the data look unsymmetrical and this makes troubleshooting harder. Instead, signify "Bid" and "Ask" sides with the sign of the order volume, so that "-100" would be a "Bid on 100 units" and "100" would be "Ask on 100 units".
* Add output messages (via STDOUT or UDP) for:
  + confirmation that orders have been placed
  + confirmation that orders have been modified
  + information that an order has been matched
  + information/confirmation that an orderbook has been created
  + information/confirmation that an orderbook has been deleted
  + information that orders in deleted order books have been deleted
* Implement order cancellation on timeout. For the purposes of matching, it's only relevant to check timeouts when attempting to match a trade in an orderbook and price tick. Otherwise a lot of time will be spent scanning through the order books and price ticks every time a new timestamp is received. (For the purposes of market information, It's good to have this information as soon as possible, in contrast. In that case a different shadow order book could be maintained, just information purposes.) Updates of time information could be done by creating a separate dedicated time service sending time commands periodically. 

## Low priority

* Add "fill or kill" market order type (execute match completely or not at all). Somehow.
* printOrderBookDepth() will segfault, if fed with a bad pointer
* Create a wrapper function for modifyOrderVolume() to handle and differentiate between:
  + full and partial matches with logging and user info.
  + users modifying the volume of their orders with logging and user info.
* Relative tick prices with a price offset. There should be different offsets for both bid and ask sides to optimize memory usage.
* Configurable "price limits" to stop users from placing orders too far from the "top of the book" (thus wasting memory and performance).
* Rollback order/trade functionality. This could be used for compensation after post-trade risk management is triggered.
* Price tick table translation. There should be some way to translate prices between real-world price format and tick format.
* Notification mechanisms for trader feedback and market data.
* User trade history.
* Order ID should be relative to business date.
* Every order book should have a defined timezone.
* Order book data, containing:
  + Time zone
  + Business date
  + Open & Close times (if any)
  + Non-trading day table reference
* Modify growPriceTicks() to grow data both up and down
* Price tick order array defragmenter (remove holes from cancelled orders)
* Price tick order array size shrinking
* Minimum quantity for "immediate or cancel" orders (ie. with ttl 0).
* Circuit breakers to take some action (halt market? start auction?) if there is an excessive amount of market volatility.
* User portfolios including rudimentary risk management.
* Auction execution function (close order entry, un-cross order book, re-enable order entry)
