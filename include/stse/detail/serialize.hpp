#pragma once

#include "types.hpp"
#include "concepts.hpp"
#include "meta_utils.hpp"
#include "memory.hpp"
#include "layout.hpp"

#include <span>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace stse::detail {

template<Scalar T, EndianType Endian>
constexpr std::span<std::byte> serialize_scalar(std::span<std::byte>, const T&, Endian);
template<StaticContainer T, EndianType Endian>
constexpr std::span<std::byte> serialize_static_container(std::span<std::byte>, const T&, Endian);
template<Aggregate T, EndianType Endian>
constexpr std::span<std::byte> serialize_aggregate(std::span<std::byte>, const T&, Endian);

template<Serializable T, EndianType Endian>
constexpr std::span<std::byte> serialize(std::span<std::byte>, const T&, Endian);
template<NotSerializable T, EndianType Endian>
constexpr std::span<std::byte> serialize(std::span<std::byte>, const T&, Endian) 
    = delete("Type not supported for static serialization");

template<Scalar T, EndianType Endian>
constexpr std::span<std::byte> serialize_scalar(
    std::span<std::byte> destination, 
    const T& source,
    [[maybe_unused]] Endian endianness
) {
    static constexpr std::size_t value_byte_count = raw_size<T>;
    using value_buffer_t = std::array<std::byte, value_byte_count>;
    value_buffer_t bytes;

    if constexpr (Endian::endian == NativeEndian::endian) {
        bytes = std::bit_cast<value_buffer_t>(source);
    } else if constexpr (std::is_integral_v<T>) {
        bytes = std::bit_cast<value_buffer_t>(std::byteswap(source));
    } else {
        using U = uint_of_size_t<T>;
        auto byte_buffer = std::bit_cast<U>(source);
        bytes = std::bit_cast<value_buffer_t>(std::byteswap(byte_buffer));
    }

    constexpr_memcpy(destination.data(), bytes.data(), value_byte_count);

    return destination.subspan(value_byte_count);
}

template<StaticContainer T, EndianType Endian>
constexpr std::span<std::byte> serialize_static_container(
    std::span<std::byte> destination, 
    const T& source,
    Endian endianness
) {
    auto& [...items] = source;
    ((destination = serialize(destination, items, endianness)), ...);
    return destination;
}

template<Aggregate T, EndianType Endian>
constexpr std::span<std::byte> serialize_aggregate(
    std::span<std::byte> destination,
    const T& source,
    Endian endianness
) {
    template for (constexpr auto member : serializable_members_of<T>) {
        destination = serialize(destination, source.[:member:], endianness);
    }

    return destination;
}

template<Serializable T>
constexpr std::span<std::byte> serialize_flat(std::span<std::byte> destination, const T& source) {
    const auto write_bytes = [destination](const std::byte* memory_layout) {
        std::size_t write_offset = 0;
        template for (constexpr auto [offset, count]: byte_layout_of<T>) {
            constexpr_memcpy(destination.data() + write_offset, memory_layout + offset, count);
            write_offset += count;
        }
    };

    if consteval {
        using value_buffer_t = std::array<const std::byte, sizeof(T)>;
        const auto raw_bytes = std::bit_cast<value_buffer_t>(source);
        write_bytes(raw_bytes.data());
    } else {
        write_bytes(reinterpret_cast<const std::byte*>(&source));
    }

    return destination.subspan(raw_size<T>);
}

template<Serializable T, EndianType Endian> 
constexpr std::span<std::byte> serialize_fields(
    std::span<std::byte> destination, 
    const T& source,
    Endian endianness
) {
    if constexpr (Scalar<T>) {
        return serialize_scalar(destination, source, endianness); 
    } else if constexpr (StaticContainer<T>) {
        return serialize_static_container(destination, source, endianness); 
    } else if constexpr (Aggregate<T>) {
        return serialize_aggregate(destination, source, endianness); 
    } else {
        std::unreachable();
    }
}


template<Serializable T, EndianType Endian> 
constexpr std::span<std::byte> serialize(
    std::span<std::byte> destination, 
    const T& source,
    Endian endianness
) { 
    if constexpr (Endian::endian == NativeEndian::endian) {
        if consteval {
            if constexpr (BitCastSafe<T>) {
                return serialize_flat(destination, source);
            } else {
                return serialize_fields(destination, source, endianness);
            }
        } else {
            return serialize_flat(destination, source);
        }
    } else {
        return serialize_fields(destination, source, endianness);
    }
}

}
