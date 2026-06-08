#pragma once 

#include <concepts>
#include <bit>
#include <format>

template <std::integral T>
struct BigEndian {
    T raw_value;

    operator T() const noexcept {
        if constexpr (std::endian::native == std::endian::big) 
            return raw_value; 
        else
            return std::byteswap(raw_value);
    }
};

template <std::integral T>
struct std::formatter<BigEndian<T>> : std::formatter<T> {
    auto format(const BigEndian<T>& val, std::format_context& ctx) const {
        return std::formatter<T>::format(static_cast<T>(val), ctx);
    }
};
