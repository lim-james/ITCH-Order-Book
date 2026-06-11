#pragma once 

#include "types.h"

template<typename buffer_t>
class SPMCProducer {
    
    using T = buffer_t::value_t;
    using shared_buffer_t = std::shared_ptr<buffer_t>;

    static constexpr auto CAPACITY = buffer_t::capacity();
    static constexpr auto MASK     = CAPACITY - 1;

public:

    explicit SPMCProducer(shared_buffer_t queue): buffer_(queue) {}
    ~SPMCProducer() = default;

    SPMCProducer(const SPMCProducer&)   = delete;
    void operator=(const SPMCProducer&) = delete;

    SPMCProducer(SPMCProducer&&)            = default;
    SPMCProducer& operator=(SPMCProducer&&) = default;

    ProduceResponse try_push(const T& item) {
        auto write_idx = buffer_->write_idx_.load(std::memory_order_relaxed);
        if (!is_valid_write_idx(write_idx)) return ProduceResponse::FAILED_BUFFER_FULL;

        buffer_->buffer_[write_idx & MASK] = item;
        buffer_->write_idx_.store(write_idx + 1, std::memory_order_release);
        return ProduceResponse::SUCCESS;
    }

    ProduceResponse try_push_many(std::span<const T> items) {
        if (items.size() > CAPACITY) return ProduceResponse::FAILED_PAYLOAD_TOO_LARGE;

        auto write_idx = buffer_->write_idx_.load(std::memory_order_relaxed);
        auto final_write_idx = items.size() - 1 + write_idx;
        
        if (!is_valid_write_idx(final_write_idx)) return ProduceResponse::FAILED_BUFFER_FULL;

        for (std::size_t i = 0; i < items.size(); ++i) {
            buffer_->buffer_[(write_idx + i) & MASK] = items[i];
        }

        buffer_->write_idx_.store(final_write_idx + 1, std::memory_order_release);
        return ProduceResponse::SUCCESS;
    }

    void close() {
        buffer_->is_closed_.store(true, std::memory_order_release);
    }

private:

    std::size_t cached_read_idx_{};
    shared_buffer_t buffer_;

    bool is_valid_write_idx(std::size_t write_idx) {
        if (write_idx - cached_read_idx_ >= CAPACITY) {
            cached_read_idx_ = get_first_read_idx();
            return write_idx - cached_read_idx_ < CAPACITY;
        }
        return true;
    }

    std::size_t get_first_read_idx() {
        std::size_t first_read_idx = std::numeric_limits<std::size_t>::max();
        for (auto& consumer: buffer_->consumers_) {
            first_read_idx = std::min(
                first_read_idx,
                consumer.read_idx_.load(std::memory_order_acquire)
            );
        }
        return first_read_idx; 
    }

};
