#pragma once

#include <new>
#include <atomic>
#include <array>
#include <optional>
#include <concepts>

template<std::copyable T, std::size_t Capacity>
class SPSCQueue {

    static_assert(Capacity > 0 && "Capacity cannot be empty");
    static_assert((Capacity & (Capacity - 1) == 0 && "Capacity must be power of 2");

    using value_t = T;
    using index_t = decltype(Capacity);

public:

    SPSCQueue() = default;
    ~SPSCQueue() = default;

    SPSCQueue(const SPSCQueue&) = delete;
    void operator=(const SPSCQueue&) = delete;

    SPSCQueue(SPSCQueue&&) = delete;
    void operator=(SPSCQueue&&) = delete;

    bool try_push(const T& item) {
        auto write_idx = write_idx_.load(std::memory_order_relaxed);
        
        if (write_idx - cached_read_idx_ == Capacity) {
            cached_read_idx_ = read_idx_.load(std::memory_order_acquire);
            if (write_idx - cached_read_idx_ == Capacity) return false;
        }

        buffer_[write_idx & MASK] = item;
        write_idx_.store(write_idx + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> try_pop() {
        auto read_idx = read_idx_.load(std::memory_order_relaxed);
        
        if (read_idx == cached_write_idx_) {
            cached_write_idx_ = write_idx_.load(std::memory_order_acquire);
            if (read_idx == cached_write_idx_) return std::nullopt;
        }

        T item = buffer_[read_idx & MASK];
        read_idx_.store(read_idx + 1, std::memory_order_release);
        return item;
    }

private:

    static constexpr index_t MASK = Capacity - 1;

    std::array<T, Capacity> buffer_;
    
    alignas(std::hardware_destructive_interference_size)
    std::atomic<index_t> write_idx_ = 0;
    index_t cached_read_idx_ = 0;

    alignas(std::hardware_destructive_interference_size)
    std::atomic<index_t> read_idx_ = 0;
    index_t cached_write_idx_ = 0;

};
