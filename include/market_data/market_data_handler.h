#pragma once

#include "messages.h"
#include "spsc/spsc_queue.h"
#include "stse/stse.hpp"

template<std::size_t N>
class MarketDataHandler {
    
    using consumer_t   = SPSCConsumer<std::byte, N>;

public:

    MarketDataHandler(consumer_t&& consumer)
        : consumer_queue_{std::forward<consumer_t&&>(consumer)} {}
    
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
            case 'A': parse_message<nasdaq::AddOrderMessage>(); break;
            case 'F': parse_message<nasdaq::AddOrderMPIDMessage>(); break;
            case 'E': parse_message<nasdaq::OrderExecutedMessage>(); break;
            case 'C': parse_message<nasdaq::OrderExecutedWithPriceMessage>(); break;
            case 'X': parse_message<nasdaq::OrderCancelMessage>(); break;
            case 'D': parse_message<nasdaq::OrderDeleteMessage>(); break;
            case 'U': parse_message<nasdaq::OrderReplaceMessage>(); break;
        }
    }

};
