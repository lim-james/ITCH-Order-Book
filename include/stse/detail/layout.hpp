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


struct ByteRange { std::size_t struct_offset, wire_offset, count; };
using ByteSequence = std::vector<ByteRange>;

template<Scalar T>
consteval void enumerate_scalar(
    ByteSequence& sequence, 
    std::size_t struct_offset,
    std::size_t wire_offset
) {
    sequence.emplace_back(struct_offset, wire_offset, raw_size<T>);
}

template<StaticContainer T>
consteval void enumerate_static_container(
    ByteSequence& sequence, 
    std::size_t struct_offset,
    std::size_t wire_offset
) {
    using value_t = typename T::value_type;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (enumerate_object<value_t>(
            sequence, 
            struct_offset + sizeof(value_t) * I,
            wire_offset + sizeof(value_t) * I
        ), ...);
    }(std::make_index_sequence<std::tuple_size_v<T>>{});
}

template<Aggregate T>
consteval void enumerate_aggregate(
    [[maybe_unused]] ByteSequence& sequence, 
    [[maybe_unused]] std::size_t struct_offset,
    std::size_t wire_offset
) {
    std::size_t current_wire = wire_offset;
    template for (constexpr auto member : non_skipped_members_of<T>) {
        using member_t = typename[:std::meta::type_of(member):];
        constexpr std::size_t member_struct_offset = std::meta::offset_of(member).bytes;
        if constexpr (!has_annotation<ignoreserialization>(member)) {
            enumerate_object<member_t>(sequence, struct_offset + member_struct_offset, current_wire);
        }
        current_wire += raw_size<member_t>;
    }
}

template<Serializable T> 
consteval void enumerate_object(
    [[maybe_unused]] ByteSequence& sequence, 
    std::size_t struct_offset = 0,
    std::size_t wire_offset = 0
) { 
    if constexpr (Scalar<T>) {
        enumerate_scalar<T>(sequence, struct_offset, wire_offset); 
    } else if constexpr (StaticContainer<T>) {
        enumerate_static_container<T>(sequence, struct_offset, wire_offset);
    } else if constexpr (Aggregate<T>) {
        enumerate_aggregate<T>(sequence, struct_offset, wire_offset); 
    } else {
        std::unreachable();
    }
}

consteval ByteSequence compress_layout(const ByteSequence& raw_sequence) { 
    ByteSequence compressed{};

    for (auto [struct_offset, wire_offset, count]: raw_sequence) {
        if (!compressed.empty() && 
            struct_offset == compressed.back().struct_offset + compressed.back().count &&
            wire_offset == compressed.back().wire_offset + compressed.back().count) {
            compressed.back().count += count;
        } else {
            compressed.emplace_back(struct_offset, wire_offset, count);
        }
    }

    return compressed;
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
