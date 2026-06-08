#pragma once 

#include "data_source.h"
#include "spsc/spsc_queue.h"

#include <cstddef>
#include <print>

template<data_source source_t, std::size_t N>
class FeedHandler {
    
    using producer_t = SPSCProducer<std::byte, N>;
    
public:

    FeedHandler(const source_t& source, producer_t&& producer)
        : source_{source}
        , producer_queue_{std::forward<producer_t&&>(producer)} {}

    void poll() {
        if (auto next_frame = source_.next()) {
            auto c = *reinterpret_cast<const char*>(next_frame->data());
            std::println("Message Type: {}", c);
            while (producer_queue_.try_push_many(*next_frame) != PushResponse::SUCCESS) {
                // std::println("Failed to push {} bytes", next_frame->size());
            }
            std::println("Read {} bytes", next_frame->size());
        }
    }

private:

    source_t source_;
    producer_t producer_queue_;

};
