#pragma once

#include <new>
#include <atomic>
#include <array>
#include <expected>
#include <concepts>
#include <span>
#include <memory>
#include <cassert>

template<std::copyable T, std::size_t Capacity>
class SPSCProducer;

template<std::copyable T, std::size_t Capacity>
class SPSCConsumer;

template<std::copyable T, std::size_t Capacity>
class SPSCQueue {

    static_assert(Capacity > 0 && "Capacity cannot be empty");
    static_assert((Capacity & (Capacity - 1)) == 0 && "Capacity must be power of 2");

    using value_t = T;
    using index_t = decltype(Capacity);

public:

    SPSCQueue() = default;
    ~SPSCQueue() = default;

    SPSCQueue(const SPSCQueue&) = delete;
    void operator=(const SPSCQueue&) = delete;

    SPSCQueue(SPSCQueue&&) = delete;
    void operator=(SPSCQueue&&) = delete;

private:

    std::atomic<bool> is_closed_ = false;
    std::array<T, Capacity> buffer_{};
    
    alignas(std::hardware_destructive_interference_size)
    std::atomic<index_t> write_idx_ = 0;
    index_t cached_read_idx_ = 0;

    alignas(std::hardware_destructive_interference_size)
    std::atomic<index_t> read_idx_ = 0;
    index_t cached_write_idx_ = 0;

    friend class SPSCProducer<T, Capacity>;
    friend class SPSCConsumer<T, Capacity>;

};


enum class PushResponse: std::uint8_t { 
    SUCCESS, 
    FAILED_BUFFER_FULL, FAILED_PAYLOAD_TOO_LARGE 
};

template<std::copyable T, std::size_t Capacity>
class SPSCProducer {
    
    using shared_queue_t = std::shared_ptr<SPSCQueue<T, Capacity>>;
    static constexpr auto MASK = Capacity - 1;

public:

    explicit SPSCProducer(shared_queue_t queue): queue_(queue) {}
    ~SPSCProducer() = default;

    SPSCProducer(const SPSCProducer&) = delete;
    void operator=(const SPSCProducer&) = delete;

    SPSCProducer(SPSCProducer&&) = default;
    SPSCProducer& operator=(SPSCProducer&&) = default;

    PushResponse try_push(const T& item) {
        auto write_idx = queue_->write_idx_.load(std::memory_order_relaxed);
        
        if (write_idx - queue_->cached_read_idx_ == Capacity) {
            queue_->cached_read_idx_ = queue_->read_idx_.load(std::memory_order_acquire);
            if (write_idx - queue_->cached_read_idx_ == Capacity) 
                return PushResponse::FAILED_BUFFER_FULL;
        }

        queue_->buffer_[write_idx & MASK] = item;
        queue_->write_idx_.store(write_idx + 1, std::memory_order_release);
        return PushResponse::SUCCESS;
    }

    PushResponse try_push_many(std::span<const T> items) {
        if (items.size() > Capacity) return PushResponse::FAILED_PAYLOAD_TOO_LARGE;

        auto write_idx = queue_->write_idx_.load(std::memory_order_relaxed);
        auto final_write_idx = items.size() - 1 + write_idx;
        
        if (final_write_idx - queue_->cached_read_idx_ >= Capacity) {
            queue_->cached_read_idx_ = queue_->read_idx_.load(std::memory_order_acquire);
            if (final_write_idx - queue_->cached_read_idx_ >= Capacity) 
                return PushResponse::FAILED_BUFFER_FULL;
        }

        for (std::size_t i = 0; i < items.size(); ++i) {
            queue_->buffer_[(write_idx + i) & MASK] = items[i];
        }

        queue_->write_idx_.store(final_write_idx + 1, std::memory_order_release);
        return PushResponse::SUCCESS;
    }

    void close() {
        queue_->is_closed_.store(true, std::memory_order_release);
    }

private:

    shared_queue_t queue_;

};

enum ConsumeFailure { NONE, BUFFER_INSUFFICIENT, BUFFER_CLOSED };

template<std::copyable T, std::size_t Capacity>
class SPSCConsumer {
    
    using shared_queue_t = std::shared_ptr<SPSCQueue<T, Capacity>>;
    static constexpr auto MASK = Capacity - 1;

public:

    using pop_result_t = std::expected<T, ConsumeFailure>;

    explicit SPSCConsumer(shared_queue_t queue): queue_(queue) {}
    ~SPSCConsumer() = default;

    SPSCConsumer(const SPSCConsumer&) = delete;
    void operator=(const SPSCConsumer&) = delete;

    SPSCConsumer(SPSCConsumer&&) = default;
    SPSCConsumer& operator=(SPSCConsumer&&) = default;

    pop_result_t try_pop() {
        auto read_idx = queue_->read_idx_.load(std::memory_order_relaxed);
        
        if (read_idx == queue_->cached_write_idx_) {
            queue_->cached_write_idx_ = queue_->write_idx_.load(std::memory_order_acquire);
            if (read_idx == queue_->cached_write_idx_) {
                return std::unexpected{
                    queue_->is_closed_.load(std::memory_order_acquire)
                    ? ConsumeFailure::BUFFER_CLOSED
                    : ConsumeFailure::BUFFER_INSUFFICIENT
                };
            }
        }

        T item = queue_->buffer_[read_idx & MASK];
        queue_->read_idx_.store(read_idx + 1, std::memory_order_release);
        return item;
    }

    ConsumeFailure try_pop_many(std::size_t count, std::span<T> read_ptr) {
        assert(read_ptr.size() >= count);

        auto read_idx = queue_->read_idx_.load(std::memory_order_relaxed);
        
        if (queue_->cached_write_idx_ - read_idx < count) {
            queue_->cached_write_idx_ = queue_->write_idx_.load(std::memory_order_acquire);
            if (queue_->cached_write_idx_ - read_idx < count) {
                return queue_->is_closed_.load(std::memory_order_acquire)
                    ? ConsumeFailure::BUFFER_CLOSED
                    : ConsumeFailure::BUFFER_INSUFFICIENT;
            }
        }

        for (std::size_t i = 0; i < count; ++i)
            read_ptr[i] = queue_->buffer_[(read_idx + i) & MASK];

        queue_->read_idx_.store(read_idx + count, std::memory_order_release);
        return ConsumeFailure::NONE;
    }

    ConsumeFailure try_skip_many(std::size_t count) {
        auto read_idx = queue_->read_idx_.load(std::memory_order_relaxed);
        
        if (queue_->cached_write_idx_ - read_idx < count) {
            queue_->cached_write_idx_ = queue_->write_idx_.load(std::memory_order_acquire);
            if (queue_->cached_write_idx_ - read_idx < count) {
                return queue_->is_closed_.load(std::memory_order_acquire)
                    ? ConsumeFailure::BUFFER_CLOSED
                    : ConsumeFailure::BUFFER_INSUFFICIENT;
            }
        }

        queue_->read_idx_.store(read_idx + count, std::memory_order_release);
        return ConsumeFailure::NONE;
    }

private:

    shared_queue_t queue_;

};

