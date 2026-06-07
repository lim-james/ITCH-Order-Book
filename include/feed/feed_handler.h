#pragma once 

#include "data_source.h"

#include <print>

template<data_source source_t>
class FeedHandler {
public:

    FeedHandler(const source_t& source): source_(source) {}

    void poll() {
        if (auto next_frame = source_.next()) {
            const auto message_type = *reinterpret_cast<const char*>(next_frame->data());
            std::println("{}", message_type);
        }
    }

private:

    source_t source_;


};
