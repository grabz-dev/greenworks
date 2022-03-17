## Types

### SteamItemDetails

Represents Steam SDK [`SteamItemDetails_t`](https://partner.steamgames.com/doc/api/ISteamInventory#SteamItemDetails_t), describing an item in a user's inventory.

* `m_itemId`
* `m_iDefinition`
* `m_unQuantity`
* `m_unFlags`

## Methods

### greenworks.consumeItem(item_consume, un_quantity, success_callback, [error_callback])

* `item_consume` String: The item instance id to consume.
* `un_quantity` String: The number of items in that stack to consume.
* `success_callback` Function(item_arr)
  * `item_arr` Array<SteamItemDetails>: Array with the current state of the consumed item(s) after the consumption.
* `error_callback` Function(err)

Consumes an amount of an item from the player's inventory.
The success callback will be fired after the item is consumed, if the player owns the required amount of the specified item.
Instead, the error callback will be fired if the operation failed for any reason.

### greenworks.startPurchase(item_arr, success_callback, [error_callback])
* `item_arr` Array<{ item_def: Number, quantity: Number }>: Array of items to begin the purchase of.
  * `item_def` Number: Item definition ID.
  * `quantity` Number: Quantity to purchase.
* `success_callback` Function()
* `error_callback` Function(err)

Begins a Steam purchase of the specified quantities of the specified items.
The callbacks are fired on initiation of the purchase.
To have a callback for after the user finalized the purchase, listen for the [`micro-txn-authorization-response` event](https://github.com/grabz-dev/greenworks/blob/master/docs/events.md).

### greenworks.getAllItems(success_callback, [error_callback])
* `success_callback` Function(item_arr)
  * `item_arr` Array<SteamItemDetails>: Array containing all of the items in the user's inventory.
* `error_callback` Function(err)

Retrieves the current state of the user's inventory.
This function is subject to rate limits, and may return cached results if called too frequently. It is suggested that you call this function only when you are about to display the user's full inventory, or if you expect that the inventory may have changed.