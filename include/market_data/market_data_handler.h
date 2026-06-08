#pragma once

#include "messages.h"
#include "stse/stse.hpp"
#include "spsc/spsc_queue.h"
#include "book/order_book.h" 

template<std::size_t N>
class MarketDataHandler {
    
    using consumer_t = SPSCConsumer<std::byte, N>;

public:

    MarketDataHandler(consumer_t&& consumer, OrderBook&& book)
        : consumer_queue_{std::forward<consumer_t&&>(consumer)} 
        , order_book_{std::forward<OrderBook&&>(book)} {}
    
    ~MarketDataHandler() noexcept = default;

    bool poll() {
        static auto get_message_type = [this](std::byte byte) {
            return static_cast<nasdaq::MessageType>(byte);
        };

        using final_failure_t = std::expected<nasdaq::MessageType, ConsumeFailure>;

        static auto handle_message_body = [this](nasdaq::MessageType message_type) -> final_failure_t {
            parse_and_dispatch_message(message_type);
            return message_type;
        };

        static constexpr auto check_stream_closed = [](ConsumeFailure failure) -> final_failure_t {
            return failure == ConsumeFailure::BUFFER_CLOSED 
                ? std::unexpected{failure} 
                : final_failure_t{};
        };
        
        auto pipeline = consumer_queue_.try_pop()
                                       .transform(get_message_type)
                                       .and_then(handle_message_body)
                                       .or_else(check_stream_closed);
        return pipeline.has_value();
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

    void parse_and_dispatch_message(nasdaq::MessageType message_type) {
        switch (message_type) {
            case 'A': order_book_.add_order(parse_message<nasdaq::AddOrderMessage>()); break;
            case 'F': order_book_.add_order_mpid(parse_message<nasdaq::AddOrderMPIDMessage>()); break;
            case 'E': order_book_.execute_order(parse_message<nasdaq::OrderExecutedMessage>()); break;
            case 'C': order_book_.execute_order_with_price(parse_message<nasdaq::OrderExecutedWithPriceMessage>()); break;
            case 'X': order_book_.cancel_order(parse_message<nasdaq::OrderCancelMessage>()); break;
            case 'D': order_book_.delete_order(parse_message<nasdaq::OrderDeleteMessage>()); break;
            case 'U': order_book_.replace_order(parse_message<nasdaq::OrderReplaceMessage>()); break;
        }
    }

};
