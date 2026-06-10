#pragma once 

#include "data_source.h"
#include "spsc/spsc_queue.h"

#include <cstddef>

template<data_source source_t, std::size_t N>
class FeedHandler {
    
    using producer_t = SPSCProducer<std::byte, N>;
    
public:

    FeedHandler(const source_t& source, producer_t&& producer)
        : source_{source}
        , producer_queue_{std::forward<producer_t&&>(producer)} {}

    ~FeedHandler() noexcept {
        producer_queue_.close();
    }

    bool poll() {
        static auto publish_queue = [this](DataFrame next_frame) -> ReadResult {
            while (producer_queue_.try_push_many(next_frame) != PushResponse::SUCCESS);
            // auto message_type = static_cast<char>(next_frame[2]);
            return next_frame;
        };

        static constexpr auto check_stream_closed = [](DataReadFailure failure) -> ReadResult {
            return failure == DataReadFailure::STREAM_CLOSED ? std::unexpected{failure} : ReadResult{};
        };

        auto next_frame = source_.next();
        auto pipeline = next_frame.and_then(publish_queue).or_else(check_stream_closed);
        return pipeline.has_value(); 
    }

private:

    source_t source_;
    producer_t producer_queue_;

};
