#pragma once

#include "spsc_queue.h"

template<std::copyable T, std::size_t Capacity>
struct SPSCPack {
    SPSCProducer<T, Capacity> producer;
    SPSCConsumer<T, Capacity> consumer;
};

template<std::copyable T, std::size_t Capacity>
[[nodiscard]] auto make_spsc() -> SPSCPack<T, Capacity> {
    auto shared_queue = std::make_shared<SPSCQueue<T, Capacity>>();
    return {.producer = SPSCProducer{shared_queue}, .consumer = SPSCConsumer{shared_queue}};
}
