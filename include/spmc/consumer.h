#pragma once

#include "types.h"

#include <memory>

template<typename buffer_t>
class SPMCConsumer {
    
    using T = buffer_t::value_t;
    using shared_buffer_t = std::shared_ptr<buffer_t>;

    static constexpr auto CAPACITY = buffer_t::capacity();
    static constexpr auto MASK     = CAPACITY - 1;

public:

    explicit SPMCConsumer(shared_buffer_t buffer)
        : buffer_(buffer) 
        , control_block_(buffer->register_new_consumer()) {}

    ~SPMCConsumer() noexcept = default;

    SPMCConsumer(const SPMCConsumer&) = delete;
    void operator=(const SPMCConsumer&) = delete;

    SPMCConsumer(SPMCConsumer&&) noexcept = default;
    SPMCConsumer& operator=(SPMCConsumer&&) noexcept = default;

    std::expected<T, ConsumeFailure> try_pop() {
        auto read_idx = control_block_->read_idx_.load(std::memory_order_relaxed);
        
        if (auto failure = check_pop_status(read_idx); failure != ConsumeFailure::NONE) {
            return std::unexpected{failure};
        }

        T item = buffer_->buffer_[read_idx & MASK];
        control_block_->read_idx_.store(read_idx + 1, std::memory_order_release);
        return item;
    }

    ConsumeFailure try_pop_many(std::size_t count, std::span<T> read_ptr) {
        assert(read_ptr.size() >= count);

        auto read_idx = control_block_->read_idx_.load(std::memory_order_relaxed);
        
        if (auto failure = check_pop_status(read_idx, count); failure != ConsumeFailure::NONE) {
            return failure;
        }

        for (std::size_t i = 0; i < count; ++i)
            read_ptr[i] = buffer_->buffer_[(read_idx + i) & MASK];

        control_block_->read_idx_.store(read_idx + count, std::memory_order_release);
        return ConsumeFailure::NONE;
    }

    ConsumeFailure try_skip_many(std::size_t count) {
        auto read_idx = control_block_->read_idx_.load(std::memory_order_relaxed);
        
        if (auto failure = check_pop_status(read_idx, count); failure != ConsumeFailure::NONE) {
            return failure;
        }

        control_block_->read_idx_.store(read_idx + count, std::memory_order_release);
        return ConsumeFailure::NONE;
    }

private:

    std::size_t cached_write_idx_{};

    shared_buffer_t buffer_;
    SPMCConsumerControlBlock* control_block_;

    ConsumeFailure check_pop_status(std::size_t read_idx, std::size_t count = 1) {
        if (cached_write_idx_ - read_idx < count) {
            cached_write_idx_ = buffer_->write_idx_.load(std::memory_order_acquire);
            if (cached_write_idx_ - read_idx < count) {
                return buffer_->is_closed_.load(std::memory_order_acquire)
                    ? ConsumeFailure::BUFFER_CLOSED
                    : ConsumeFailure::BUFFER_INSUFFICIENT;
            }
        }
        return ConsumeFailure::NONE;
    }

};
