#pragma once

#include "ring_buffer.h"

#include <vector>
#include <algorithm>

template<typename T, std::size_t N>
class QuartileBuffer {
public:

    void record(const T& item) noexcept {
        buffer_.push(item);
    }

    template<double... Q> 
    auto report() const noexcept -> std::array<T, sizeof...(Q)>{
        auto count = buffer_.size();
        if (count == 0) return {};

        auto buffer_span = buffer_.get_buffer();
        std::vector results(buffer_span.begin(), buffer_span.end());
        std::ranges::sort(results);

        auto p = [&](double quartile) -> T {
            const double      count_d = static_cast<double>(count - 1);
            const std::size_t index   = static_cast<std::size_t>(count_d * quartile);
            return results[index];
        };

        return { p(Q)... };
    }

    std::size_t count() const noexcept {
        return buffer_.size();
    }

private:

    RingBuffer<time_t, N> buffer_;

};
