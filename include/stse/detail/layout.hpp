#pragma once

#include "concepts.hpp"
#include "meta_utils.hpp"

#include <vector>
#include <meta>

namespace stse::detail {

template<typename T>
consteval std::size_t size_of();

template<typename T>
inline constexpr std::size_t raw_size = size_of<T>();

template<typename T>
consteval std::size_t size_of() { 
    if constexpr (Scalar<T>) {
        return std::meta::size_of(^^T); 
    } else if constexpr (StaticContainer<T>) {
        return std::tuple_size_v<T> * raw_size<typename T::value_type>;
    } else if constexpr (Aggregate<T>) {
        std::size_t total = 0;
        template for (constexpr auto member : non_skipped_members_of<T>) {
            total += raw_size<typename[:std::meta::type_of(member):]>;
        }
        return total;
    } else {
        std::unreachable();
    }
}


struct ByteRange { std::size_t offset, count; };
using ByteSequence = std::vector<ByteRange>;

template<Scalar T>
consteval void enumerate_scalar(ByteSequence& sequence, std::size_t offset) {
    sequence.emplace_back(offset, raw_size<T>);
}

template<StaticContainer T>
consteval void enumerate_static_container(ByteSequence& sequence, std::size_t offset) {
    using value_t = typename T::value_type;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (enumerate_object<value_t>(sequence, offset + sizeof(value_t) * I), ...);
    }(std::make_index_sequence<std::tuple_size_v<T>>{});
}

template<Aggregate T>
consteval void enumerate_aggregate(ByteSequence& sequence, std::size_t offset) {
    template for (constexpr auto member : serializable_members_of<T>) {
        using member_t = typename[:std::meta::type_of(member):];
        constexpr std::size_t internal_offset = std::meta::offset_of(member).bytes;
        enumerate_object<member_t>(sequence, offset + internal_offset);
    }
}

template<Serializable T> 
consteval void enumerate_object(ByteSequence& sequence, std::size_t offset = 0) { 
    if constexpr (Scalar<T>) {
        enumerate_scalar<T>(sequence, offset); 
    } else if constexpr (StaticContainer<T>) {
        enumerate_static_container<T>(sequence, offset);
    } else if constexpr (Aggregate<T>) {
        enumerate_aggregate<T>(sequence, offset); 
    } else {
        std::unreachable();
    }
}

consteval ByteSequence compress_layout(const ByteSequence& raw_sequence) { 
    ByteSequence compressed_sequence{};

    std::size_t next_offset = 0;

    for (auto [offset, count]: raw_sequence) {
        if (!compressed_sequence.empty() && offset == next_offset) {
            compressed_sequence.back().count += count;
        } else {
            compressed_sequence.emplace_back(offset, count);
        }
        next_offset = offset + count;
    }

    return compressed_sequence;
}


template<Serializable T>
consteval ByteSequence build_sequence() {
    ByteSequence sequence{};
    enumerate_object<T>(sequence);
    sequence = compress_layout(sequence);
    return sequence;
}

template<Serializable T>
consteval std::size_t count_byte_ranges() {
    return build_sequence<T>().size();
}

template<Serializable T> 
consteval auto compute_byte_layout() { 
    static constexpr std::size_t N = count_byte_ranges<T>();
    auto sequence = build_sequence<T>();

    std::array<ByteRange, N> arr{};
    for (std::size_t i = 0; i < N; ++i) arr[i] = sequence[i];

    return std::define_static_array(arr);
}

template<Serializable T>
inline constexpr auto byte_layout_of = compute_byte_layout<T>();

}
