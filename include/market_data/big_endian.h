#pragma once 

#include <concepts>
#include <bit>

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
