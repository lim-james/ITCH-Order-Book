#include "book/order_book.h"

#include <print>

void OrderBook::add_order(const nasdaq::AddOrderMessage&) {
    std::println("nasdaq::AddOrderMessage");
      
}

void OrderBook::add_order_mpid(const nasdaq::AddOrderMPIDMessage&) {
    std::println("nasdaq::AddOrderMPIDMessage");
}

void OrderBook::execute_order(const nasdaq::OrderExecutedMessage&) {
    std::println("nasdaq::OrderExecutedMessage");
}

void OrderBook::execute_order_with_price(const nasdaq::OrderExecutedWithPriceMessage&) {
    std::println("nasdaq::OrderExecutedWithPriceMessage");
}

void OrderBook::cancel_order(const nasdaq::OrderCancelMessage&) {
    std::println("nasdaq::OrderCancelMessage");
}

void OrderBook::delete_order(const nasdaq::OrderDeleteMessage&) {
    std::println("nasdaq::OrderDeleteMessage");
}

void OrderBook::replace_order(const nasdaq::OrderReplaceMessage&) {
    std::println("nasdaq::OrderReplaceMessage");
}

