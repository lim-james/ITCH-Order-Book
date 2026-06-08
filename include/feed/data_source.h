#pragma once

#include <concepts>
#include <span>
#include <cstddef>
#include <expected>

using DataFrame = std::span<const std::byte>;

enum class DataReadFailure {
    InvalidFormat,
    StreamClosed
};

using ReadResult = std::expected<DataFrame, DataReadFailure>;

template<typename T>
concept data_source = requires (T t) {
    { t.next() } -> std::same_as<ReadResult>;
};
