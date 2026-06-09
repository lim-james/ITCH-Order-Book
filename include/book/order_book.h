#pragma once

#include "market_data/messages.h"

#include <map>

class OrderBook {

    using side_t   = char;
    using price_t  = std::uint32_t;
    using shares_t = std::uint64_t;

public:

    OrderBook() = default;
    ~OrderBook() noexcept = default;

    OrderBook(const OrderBook&) = delete;
    void operator=(const OrderBook&) = delete;

    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;

    void add_order(side_t side, price_t price, shares_t shares);
    void execute_order(side_t side, price_t price, shares_t executed_shares);
    void cancel_order(side_t side, price_t price, shares_t cancelled_shares);
    void delete_order(side_t side, price_t price, shares_t deleted_shares);
    void replace_order(side_t side,
                       price_t old_price, shares_t old_shares,
                       price_t new_price, shares_t new_shares);

private:

    using order_collection_t = std::map<price_t, nasdaq::NumShares8>;

    order_collection_t buy_;
    order_collection_t sell_;

    inline order_collection_t& get_order_side(side_t side) {
        switch (side) {
            case 'B': return buy_;
            case 'S': return sell_;
            default:  std::unreachable();
        }
    }

    void remove_shares(order_collection_t& collection, 
                       price_t price, shares_t shares);

};
