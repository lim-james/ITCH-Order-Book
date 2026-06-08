#pragma once

#include "types.hpp"
#include "concepts.hpp"
#include "meta_utils.hpp"
#include "memory.hpp"
#include "layout.hpp"

#include <span>
#include <cstddef>
#include <utility>

namespace stse::detail {

template<Scalar T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_scalar(T&, std::span<const std::byte>, Endian);
template<StaticContainer T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_static_container(T&, std::span<const std::byte>, Endian);
template<Aggregate T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_aggregate(T&, std::span<const std::byte>, Endian);

template<Serializable T, EndianType Endian>
constexpr std::span<const std::byte> deserialize(T&, std::span<const std::byte>, Endian);
template<NotSerializable T, EndianType Endian>
constexpr std::span<const std::byte> deserialize(T&, std::span<const std::byte>, Endian)
    = delete("Type not supported for static serialization");


template<Scalar T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_scalar(
    T& destination, 
    std::span<const std::byte> source,
    [[maybe_unused]] Endian endianness
) {
    static constexpr std::size_t value_byte_count = raw_size<T>;
    using value_buffer_t = std::array<std::byte, value_byte_count>;
    value_buffer_t buffer;

    const auto bytes = source.first(value_byte_count);
    constexpr_memcpy(buffer.data(), bytes.data(), value_byte_count);

    destination = [&buffer]() {
        if constexpr (Endian::endian == NativeEndian::endian) {
            return std::bit_cast<T>(buffer);
        } else if constexpr (std::is_integral_v<T>) {
            return std::byteswap(std::bit_cast<T>(buffer));
        } else {
            using U = uint_of_size_t<T>;
            auto byte_buffer = std::bit_cast<U>(buffer);
            return std::bit_cast<T>(std::byteswap(byte_buffer));
        }
    }();

    return source.subspan(value_byte_count);
}

template<StaticContainer T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_static_container(
    T& destination, 
    std::span<const std::byte> source,
    Endian endianness
) {
    auto& [...items] = destination;
    ((source = deserialize(items, source, endianness)), ...);
    return source;
}

template<Aggregate T, EndianType Endian>
constexpr std::span<const std::byte> deserialize_aggregate(
    T& destination, 
    std::span<const std::byte> source,
    Endian endianness
) {
    template for (constexpr auto member: serializable_members_of<T>) {
        source = deserialize(destination.[:member:], source, endianness);
    }

    return source;
}

template<Serializable T> 
constexpr std::span<const std::byte> deserialize_flat(
    T& destination, 
    std::span<const std::byte> source
) { 
    const auto read_bytes = [source](std::byte* memory_layout) {
        std::size_t read_offset = 0;
        template for (constexpr auto [offset, count]: byte_layout_of<T>) {
            constexpr_memcpy(memory_layout + offset, source.data() + read_offset, count);
            read_offset += count;
        }
    };

    if consteval {
        using value_buffer_t = std::array<std::byte, sizeof(T)>;
        auto raw_bytes = value_buffer_t{}; 
        read_bytes(raw_bytes.data());
        destination = std::bit_cast<T>(raw_bytes);
    } else {
        read_bytes(reinterpret_cast<std::byte*>(&destination));
    }
    return source.subspan(raw_size<T>);
}


template<Serializable T, EndianType Endian> 
constexpr std::span<const std::byte> deserialize_fields(
    T& destination, 
    std::span<const std::byte> source,
    Endian endianness
) {
    if constexpr (Scalar<T>) {
        return deserialize_scalar(destination, source, endianness); 
    } else if constexpr (StaticContainer<T>) {
        return deserialize_static_container(destination, source, endianness); 
    } else if constexpr (Aggregate<T>) {
        return deserialize_aggregate(destination, source, endianness); 
    } else {
        std::unreachable();
    }
}

template<Serializable T, EndianType Endian> 
constexpr std::span<const std::byte> deserialize(
    T& destination, 
    std::span<const std::byte> source,
    Endian endianness
) {
    if constexpr (Endian::endian == NativeEndian::endian) {
        if consteval {
            if constexpr (BitCastSafe<T>) {
                return deserialize_flat(destination, source);
            } else {
                return deserialize_fields(destination, source, endianness);
            }
        } else {
            return deserialize_flat(destination, source);
        }
    } else {
        return deserialize_fields(destination, source, endianness);
    }
}

}
