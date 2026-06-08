#pragma once

#include "messages.h"
#include "stse/stse.hpp"
#include "spsc/spsc_queue.h"
#include "book/order_book.h" 

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
        static constexpr std::size_t HEADER_SIZE = stse::serial_size_v<nasdaq::MessagaHeader>;

        auto header_bytes   = std::array<std::byte, HEADER_SIZE>{};
        auto consume_status = consumer_queue_.try_pop_many(HEADER_SIZE, header_bytes);

        if (consume_status == ConsumeFailure::NONE) {
            auto [message_header] = stse::deserialize<nasdaq::MessagaHeader>(
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

    void parse_and_dispatch_message(nasdaq::MessagaHeader message_header) {
        switch (message_header.message_type) {
            case 'A': order_book_.add_order(parse_message<nasdaq::AddOrderMessage>()); break;
            case 'F': order_book_.add_order_mpid(parse_message<nasdaq::AddOrderMPIDMessage>()); break;
            case 'E': order_book_.execute_order(parse_message<nasdaq::OrderExecutedMessage>()); break;
            case 'C': order_book_.execute_order_with_price(parse_message<nasdaq::OrderExecutedWithPriceMessage>()); break;
            case 'X': order_book_.cancel_order(parse_message<nasdaq::OrderCancelMessage>()); break;
            case 'D': order_book_.delete_order(parse_message<nasdaq::OrderDeleteMessage>()); break;
            case 'U': order_book_.replace_order(parse_message<nasdaq::OrderReplaceMessage>()); break;
            default:  skip_message(message_header.packet_size);
        }
    }

};
