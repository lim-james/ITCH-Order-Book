#pragma once 

#include <memory>
#include <span>

template<typename T, std::size_t N>
class RingBuffer {

    using value_t = T;

    static constexpr std::size_t CAPACITY = N;
    static_assert(CAPACITY > 0 && "Capacity must be non-empty");
    static_assert((CAPACITY & (CAPACITY-1)) == 0 && "Capacity must be power of 2");

    static constexpr std::size_t MASK = CAPACITY - 1;

public:

    RingBuffer() = default;
    ~RingBuffer() noexcept = default;

    RingBuffer(const RingBuffer&) = delete;
    void operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&&) = default;
    RingBuffer& operator=(RingBuffer&&) = default;

    void push(const T& item) noexcept {
        buffer_[size_++ & MASK] = item;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return std::min(CAPACITY, size_);
    }

    [[nodiscard]] std::span<const value_t> get_buffer() const noexcept {
        return std::span{buffer_.get(), size()};
    }

private:

    std::size_t size_{};
    std::unique_ptr<value_t[]> buffer_ = std::make_unique<value_t[]>(N);

};
