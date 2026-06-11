#pragma once

#include "ring_buffer.h"
#include "producer.h"
#include "consumer.h"

#include <vector>

template<typename buffer_t>
struct SPMCPack {
    using producer_t = buffer_t::producer_t;
    using consumer_t = buffer_t::consumer_t;

    producer_t producer;
    std::vector<consumer_t> consumers;
};

template<std::copyable T, std::size_t Capacity>
[[nodiscard]] auto make_spmc(std::size_t consumer_count) {
    using buffer_t   = SPMCRingBuffer<T, Capacity>;
    using producer_t = buffer_t::producer_t;
    using consumer_t = buffer_t::consumer_t;

    auto shared_buffer = std::make_shared<buffer_t>();

    std::vector<consumer_t> consumers;
    consumers.reserve(consumer_count);

    for (std::size_t i = 0; i < consumer_count; ++i)
        consumers.emplace_back(shared_buffer);

    return SPMCPack<buffer_t>{
        .producer  = producer_t{shared_buffer}, 
        .consumers = std::move(consumers)
    };
}
