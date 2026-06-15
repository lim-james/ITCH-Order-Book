#pragma once

#include "rdtsc.h"
#include "quartile_buffer.h"

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
        ~recorder() { parent_->buffer_.record(rdtsc() - t0_); }
    };
    
public:

    recorder record_scope() noexcept {
        return recorder{this};
    }

    void report(std::string_view label, double ghz = tsc_ghz()) const {
        std::size_t count = buffer_.count();
        if (count == 0) return;

        auto [p50, p99, p999] = buffer_.report<0.5, 0.99, 0.999>();

        std::println(
            "[{}] n={} p50={:.1f}ns p99={:.1f}ns p99.9={:.1f}ns",
            label, count, 
            (double)p50 / ghz, (double)p99 / ghz, (double)p999 / ghz
        );
    }

private:

    static constexpr std::size_t MAX_SAMPLES = 1 << 24;
    QuartileBuffer<time_t, MAX_SAMPLES> buffer_;

};
