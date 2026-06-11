#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

template<typename buffer_t> class SPMCProducer;
template<typename buffer_t> class SPMCConsumer;

struct SPMCConsumerControlBlock {
    alignas(std::hardware_destructive_interference_size)
    std::atomic<std::size_t> read_idx_ = 0;
};

template<std::copyable T, std::size_t Capacity, std::size_t MaxConsumers = 8>
class SPMCRingBuffer;

enum class ProduceResponse: std::uint8_t { 
    SUCCESS, 
    FAILED_BUFFER_FULL, FAILED_PAYLOAD_TOO_LARGE 
};

enum class ConsumeFailure: std::uint8_t { NONE, BUFFER_INSUFFICIENT, BUFFER_CLOSED };
