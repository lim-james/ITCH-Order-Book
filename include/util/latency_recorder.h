#pragma once

#include "rdtsc.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <print>
#include <algorithm>
#include <string_view>

class LatencyRecorder {

    using time_t = std::uint64_t;

    struct recorder {
        time_t t0_;
        LatencyRecorder* parent_;
        recorder(LatencyRecorder* parent): t0_{rdtsc()}, parent_{parent} {}
        ~recorder() { parent_->record(rdtsc() - t0_); }
    };
    
public:

    recorder record_scope() noexcept {
        return recorder{this};
    }

    void report(std::string_view label, double ghz = tsc_ghz()) const {
        if (count_ == 0) return;
        
        auto count = std::min(MAX_SAMPLES, count_);

        std::vector times(buffer_.get(), buffer_.get() + count);
        std::ranges::sort(times);

        auto p = [&](double quartile) {
            std::size_t index = static_cast<std::size_t>((count - 1) * quartile);
            double time_ns = static_cast<double>(times[index]);
            return time_ns / ghz;
        };

        std::println(
            "[{}] n={} p50={:.1f}ns p99={:.1f}ns p99.9={:.1f}ns",
            label, count, 
            p(0.5), p(0.99), p(0.999)
        );
    }

private:

    static constexpr std::size_t MAX_SAMPLES = 1 << 24;
    static_assert((MAX_SAMPLES & (MAX_SAMPLES - 1)) == 0 && "Max Samples must be power of 2");

    static constexpr std::size_t MASK = MAX_SAMPLES - 1;

    std::unique_ptr<time_t[]> buffer_ = std::make_unique<time_t[]>(MAX_SAMPLES);
    std::size_t count_;

    void record(time_t time) noexcept {
        buffer_[count_++ & MASK] = time;
    }
};
