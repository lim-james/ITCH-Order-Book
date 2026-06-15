#pragma once

#include "types.h"

#include <new>
#include <atomic>
#include <inplace_vector>
#include <expected>
#include <concepts>
#include <span>
#include <memory>
#include <cassert>
#include <limits>

template<std::copyable T, std::size_t Capacity, std::size_t MaxConsumers>
class SPMCRingBuffer {

    using value_t = T;

    static_assert(Capacity > 0 && "Capacity cannot be empty");
    static_assert((Capacity & (Capacity - 1)) == 0 && "Capacity must be power of 2");

public:

    using producer_t = SPMCProducer<SPMCRingBuffer>;
    using consumer_t = SPMCConsumer<SPMCRingBuffer>;

    SPMCRingBuffer() = default;
    ~SPMCRingBuffer() = default;

    SPMCRingBuffer(const SPMCRingBuffer&) = delete;
    void operator=(const SPMCRingBuffer&) = delete;

    SPMCRingBuffer(SPMCRingBuffer&&) = delete;
    void operator=(SPMCRingBuffer&&) = delete;

    static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    [[nodiscard]] auto register_new_consumer() -> SPMCConsumerControlBlock* {
        consumers_.emplace_back();
        return &consumers_.back();
    }

private:

    std::atomic<bool> is_closed_ = false;
    std::unique_ptr<T[]> buffer_ = std::make_unique<T[]>(Capacity);
    
    alignas(std::hardware_destructive_interference_size)
    std::atomic<std::size_t> write_idx_ = 0;

    std::inplace_vector<SPMCConsumerControlBlock, MaxConsumers> consumers_{};

    friend class SPMCProducer<SPMCRingBuffer>;
    friend class SPMCConsumer<SPMCRingBuffer>;

};

