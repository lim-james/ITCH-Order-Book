#include "book/order_book.h"

#include <print>

void OrderBook::add_order(side_t side, price_t price, shares_t shares) {
    get_order_side(side)[price] += shares;
}

void OrderBook::execute_order(side_t side, price_t price, shares_t executed_shares) {
    remove_shares(get_order_side(side), price, executed_shares);
}

void OrderBook::cancel_order(side_t side, price_t price, shares_t cancelled_shares) {
    remove_shares(get_order_side(side), price, cancelled_shares);
}

void OrderBook::delete_order(side_t side, price_t price, shares_t deleted_shares) {
    remove_shares(get_order_side(side), price, deleted_shares);
}

void OrderBook::replace_order(side_t side, 
                              price_t old_price, shares_t old_shares,
                              price_t new_price, shares_t new_shares) {
    auto& order_collection = get_order_side(side);
    remove_shares(order_collection, old_price, old_shares);
    order_collection[new_price] += new_shares;
}

void OrderBook::remove_shares(order_collection_t& collection, 
                              price_t price, shares_t shares) {
    auto entry = collection.find(price);
    entry->second -= shares;
    if (entry->second == 0) collection.erase(entry); 
}
