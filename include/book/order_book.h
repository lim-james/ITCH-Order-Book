#pragma once

#include "market_data/messages.h"

#include <map>

class OrderBook {

public:

    OrderBook() = default;
    ~OrderBook() noexcept = default;

    OrderBook(const OrderBook&) = delete;
    void operator=(const OrderBook&) = delete;

    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;

    void add_order(const nasdaq::AddOrderMessage&);
    void add_order_mpid(const nasdaq::AddOrderMPIDMessage&);
    void execute_order(const nasdaq::OrderExecutedMessage&);
    void execute_order_with_price(const nasdaq::OrderExecutedWithPriceMessage&);
    void cancel_order(const nasdaq::OrderCancelMessage&);
    void delete_order(const nasdaq::OrderDeleteMessage&);
    void replace_order(const nasdaq::OrderReplaceMessage&);

private:

    struct Entry {

        auto operator<=>(const Entry&) const = default;
    };

    using order_collection_t = std::map<nasdaq::Price4, nasdaq::NumShares8>;

    order_collection_t bids_;
    order_collection_t asks_;

};
