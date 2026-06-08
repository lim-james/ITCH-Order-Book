#pragma once

#include <cstddef>
#include <cstring>

namespace stse::detail {

constexpr void constexpr_memcpy(std::byte* destination, const std::byte* source, std::size_t count) {
    if consteval {
        for (std::size_t i = 0; i < count; ++i) {
            destination[i] = source[i];
        }
    } else {
        std::memcpy(destination, source, count);
    }
}

}
