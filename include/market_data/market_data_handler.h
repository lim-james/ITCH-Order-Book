#pragma once

#include "messages.h"
#include "stse/stse.hpp"
#include "spmc/consumer.h"
#include "book/order_book.h" 

#include <unordered_map>
#include <cassert>

#include <print>

template<nasdaq::Stock TargetStock, typename consumer_t>
class MarketDataHandler {
    
    enum class MessageStatus: std::uint8_t { SIMULATED, SKIPPED };

public:

    MarketDataHandler(consumer_t&& consumer, OrderBook&& book)
        : consumer_queue_{std::forward<consumer_t&&>(consumer)} 
        , order_book_{std::forward<OrderBook&&>(book)} {}
    
    ~MarketDataHandler() noexcept {
        std::println("{}: {}", TargetStock, message_count_);
    }

    bool poll() {
        auto packet_size_response = try_read_from_queue<nasdaq::PacketSize>();
        if (!packet_size_response.has_value()) {
            return packet_size_response.error() != ConsumeFailure::BUFFER_CLOSED;
        }

        auto packet_size = packet_size_response.value();
        auto packet_body_size = packet_size - stse::serial_size_v<nasdaq::HeaderView>;

        auto header = read_from_queue_unsafe<nasdaq::HeaderView>();

        if (header.message_type != nasdaq::AddOrderMessage::MESSAGE_TYPE &&
            header.stock_locate != cache_stock_locate_) {
            consumer_queue_.try_skip_many(packet_body_size);
            return true;
        }

        if (simulate_message(header.message_type, header.stock_locate) == MessageStatus::SKIPPED) {
            consumer_queue_.try_skip_many(packet_body_size);
        } else {
            message_count_++;
        }

        return true; 
    }

private:

    std::size_t message_count_{};

    consumer_t consumer_queue_;
    OrderBook order_book_;

    std::uint16_t cache_stock_locate_;

    using reference_num_t = std::uint64_t;
    struct OrderRecord {
        typename OrderBook::side_t   side; 
        typename OrderBook::price_t  price; 
        typename OrderBook::shares_t shares; 
    };

    std::unordered_map<reference_num_t, OrderRecord> order_records_;

    template<typename T>
    std::expected<T, ConsumeFailure> try_read_from_queue() {
        static constexpr std::size_t T_SIZE = stse::serial_size_v<T>;
        static std::array<std::byte, T_SIZE> buffer{};
        auto consume_failure = consumer_queue_.try_pop_many(T_SIZE, buffer); 
        if (consume_failure != ConsumeFailure::NONE) return std::unexpected{consume_failure};

        T object; 
        stse::deserialize_advance(buffer, object);
        return object;
    }

    template<typename T>
    T read_from_queue_unsafe() {
        static constexpr std::size_t T_SIZE = stse::serial_size_v<T>;
        static std::array<std::byte, T_SIZE> buffer{};
        [[maybe_unused]] auto consume_failure = consumer_queue_.try_pop_many(T_SIZE, buffer); 

        assert(consume_failure == ConsumeFailure::NONE);

        T object; 
        stse::deserialize_advance(buffer, object);
        return object;
    }

    template<typename AddOrderMessage>
    void add_order(
        std::uint16_t stock_locate,
        const AddOrderMessage& message
    ) {
        if (message.stock != TargetStock) return;
        cache_stock_locate_ = stock_locate;

        using side_t = typename OrderBook::side_t;
        const auto side = message.buy_sell_indicator == 'B' ? side_t::BUY : side_t::SELL;

        order_records_[message.order_reference_number] = {
            .side   = side,
            .price  = message.price, 
            .shares = message.shares
        };
        order_book_.add_order(side, message.price, message.shares);
    }

    template<typename OrderExecuteMessage>
    void execute_order(const OrderExecuteMessage& message) {
        auto order_entry = order_records_.find(message.order_reference_number);
        auto& [side, price, shares] = order_entry->second;

        shares -= message.executed_shares;
        order_book_.execute_order(side, price, message.executed_shares);

        if (shares == 0) order_records_.erase(order_entry);
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
        auto [side, price, shares] = order_entry->second;
        order_records_.erase(order_entry);
        order_book_.cancel_order(side, price, shares);
    }

    void replace_order(const nasdaq::OrderReplaceMessage& message) {
        auto original_order_entry = order_records_.find(message.original_order_reference_number);
        auto [side, price, shares] = original_order_entry->second;

        order_records_.erase(original_order_entry);

        order_records_[message.new_order_reference_number] = {
            .side   = side,
            .price  = message.price, 
            .shares = message.shares
        };

        order_book_.replace_order(side, price, shares, message.price, message.shares);
    }

    MessageStatus simulate_message(
        char message_type, 
        std::uint16_t stock_locate
    ) {
        switch (message_type) {
            case 'A': add_order(stock_locate, read_from_queue_unsafe<nasdaq::AddOrderMessage>()); break;
            case 'F': add_order(stock_locate, read_from_queue_unsafe<nasdaq::AddOrderMPIDMessage>()); break;
            case 'E': execute_order(read_from_queue_unsafe<nasdaq::OrderExecutedMessage>()); break;
            case 'C': execute_order(read_from_queue_unsafe<nasdaq::OrderExecutedWithPriceMessage>()); break;
            case 'X': cancel_order(read_from_queue_unsafe<nasdaq::OrderCancelMessage>()); break;
            case 'D': delete_order(read_from_queue_unsafe<nasdaq::OrderDeleteMessage>()); break;
            case 'U': replace_order(read_from_queue_unsafe<nasdaq::OrderReplaceMessage>()); break;
            default:  return MessageStatus::SKIPPED;
        }
        return MessageStatus::SIMULATED;
    }

};
