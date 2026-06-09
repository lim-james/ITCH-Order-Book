#pragma once

#include "messages.h"
#include "stse/stse.hpp"
#include "spsc/spsc_queue.h"
#include "book/order_book.h" 

#include <unordered_map>

#include <print>

template<std::size_t N>
class MarketDataHandler {
    
    using consumer_t = SPSCConsumer<std::byte, N>;

public:

    MarketDataHandler(consumer_t&& consumer, OrderBook&& book)
        : consumer_queue_{std::forward<consumer_t&&>(consumer)} 
        , order_book_{std::forward<OrderBook&&>(book)} {}
    
    ~MarketDataHandler() noexcept = default;

    bool poll() {
        static constexpr std::size_t HEADER_SIZE = stse::serial_size_v<nasdaq::MessageHeader>;

        auto header_bytes   = std::array<std::byte, HEADER_SIZE>{};
        auto consume_status = consumer_queue_.try_pop_many(HEADER_SIZE, header_bytes);

        if (consume_status == ConsumeFailure::NONE) {
            auto [message_header] = stse::deserialize<nasdaq::MessageHeader>(
                header_bytes
            ).objects;

            std::println(
                "[MARKET DATA HANDLER] Read {} bytes | Message Type '{}'",
                message_header.packet_size,
                message_header.message_type
            );
            parse_and_dispatch_message(message_header);
        }

        return consume_status != ConsumeFailure::BUFFER_CLOSED;
    }

private:

    consumer_t consumer_queue_;
    OrderBook order_book_;

    using reference_num_t = std::uint64_t;
    struct OrderRecord {
        typename OrderBook::side_t   side; 
        typename OrderBook::price_t  price; 
        typename OrderBook::shares_t shares; 
    };

    std::unordered_map<reference_num_t, OrderRecord> order_records_;

    template<typename message_t>
    message_t parse_message() {
        static constexpr std::size_t PACKET_SIZE = stse::serial_size_v<message_t>;
        std::array<std::byte, PACKET_SIZE> buffer{};
        consumer_queue_.try_pop_many(PACKET_SIZE, buffer);
        auto [message] = stse::deserialize<message_t>(buffer).objects;
        std::println("[MARKET DATA HANDLER] STSE deserializing {} bytes", PACKET_SIZE);
        return message;
    }

    void skip_message(std::size_t packet_size) {
        std::println("[MARKET DATA HANDLER] Skipping {} bytes", packet_size);
        consumer_queue_.try_skip_many(packet_size - sizeof(nasdaq::MessageType));
    }

    void add_order(const nasdaq::AddOrderMessage& message) {
        using side_t = typename OrderBook::side_t;
        const auto side = message.buy_sell_indicator == 'B' ? side_t::BUY : side_t::SELL;

        order_records_[message.order_reference_number] = {
            .side   = side,
            .price  = message.price, 
            .shares = message.shares
        };
        order_book_.add_order(side, message.price, message.shares);
    }

    void add_order_mpid(const nasdaq::AddOrderMPIDMessage& message) {
        using side_t = typename OrderBook::side_t;
        const auto side = message.buy_sell_indicator == 'B' ? side_t::BUY : side_t::SELL;

        order_records_[message.order_reference_number] = {
            .side   = side,
            .price  = message.price, 
            .shares = message.shares
        };
        order_book_.add_order(side, message.price, message.shares);
    }

    void execute_order(const nasdaq::OrderExecutedMessage& message) {
        auto order_entry = order_records_.find(message.order_reference_number);
        auto& [side, price, shares] = order_entry->second;

        shares -= message.executed_shares;
        if (shares == 0) order_records_.erase(order_entry);

        order_book_.execute_order(side, price, message.executed_shares);
    }

    void execute_order_with_price(const nasdaq::OrderExecutedWithPriceMessage& message) {
        auto order_entry = order_records_.find(message.order_reference_number);
        auto& [side, price, shares] = order_entry->second;

        shares -= message.executed_shares;
        if (shares == 0) order_records_.erase(order_entry);

        order_book_.execute_order(side, price, message.executed_shares);
    }

    void cancel_order(const nasdaq::OrderCancelMessage& message) {
        auto order_entry = order_records_.find(message.order_reference_number);
        auto& [side, price, shares] = order_entry->second;

        shares -= message.cancelled_shares;
        if (shares == 0) order_records_.erase(order_entry);

        order_book_.cancel_order(side, price, message.cancelled_shares);
    }

    void delete_order(const nasdaq::OrderDeleteMessage& message) {
        auto order_entry = order_records_.find(message.order_reference_number);
        auto& [side, price, shares] = order_entry->second;
        order_records_.erase(order_entry);
        order_book_.cancel_order(side, price, shares);
    }

    void replace_order(const nasdaq::OrderReplaceMessage& message) {
        auto original_order_entry = order_records_.find(message.original_order_reference_number);
        auto& [side, price, shares] = original_order_entry->second;

        order_records_.erase(original_order_entry);

        order_records_[message.new_order_reference_number] = {
            .side   = side,
            .price  = message.price, 
            .shares = message.shares
        };

        order_book_.replace_order(side, price, shares, message.price, message.shares);
    }

    void parse_and_dispatch_message(const nasdaq::MessageHeader& message_header) {
        switch (message_header.message_type) {
            case 'A': add_order(parse_message<nasdaq::AddOrderMessage>()); break;
            case 'F': add_order_mpid(parse_message<nasdaq::AddOrderMPIDMessage>()); break;
            case 'E': execute_order(parse_message<nasdaq::OrderExecutedMessage>()); break;
            case 'C': execute_order_with_price(parse_message<nasdaq::OrderExecutedWithPriceMessage>()); break;
            case 'X': cancel_order(parse_message<nasdaq::OrderCancelMessage>()); break;
            case 'D': delete_order(parse_message<nasdaq::OrderDeleteMessage>()); break;
            case 'U': replace_order(parse_message<nasdaq::OrderReplaceMessage>()); break;
            default:  skip_message(message_header.packet_size);
        }
    }

};
