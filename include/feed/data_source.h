#pragma once

#include <concepts>
#include <span>
#include <cstddef>
#include <optional>

struct DataFrame {
    std::span<std::byte> bytes;
};

template<typename T>
concept data_source = requires (T t) {
    { t.next() } -> std::same_as<std::optional<DataFrame>>;
};
