#pragma once

#include <concepts>
#include <span>
#include <cstddef>
#include <optional>

using DataFrame = std::span<const std::byte>;

template<typename T>
concept data_source = requires (T t) {
    { t.next() } -> std::same_as<std::optional<DataFrame>>;
};
