#pragma once

#include <bit>
#include <cstdint>

namespace stse::detail {

struct BigEndian    { static constexpr std::endian endian = std::endian::big; };
struct LittleEndian { static constexpr std::endian endian = std::endian::little; };
struct NativeEndian { static constexpr std::endian endian = std::endian::native; };

struct skipserialization {};
struct ignoreserialization {};

template<std::size_t T> struct uint_of_size;
template<> struct uint_of_size<1> { using type = std::uint8_t; };
template<> struct uint_of_size<2> { using type = std::uint16_t; };
template<> struct uint_of_size<4> { using type = std::uint32_t; };
template<> struct uint_of_size<8> { using type = std::uint64_t; };

template<typename T>
using uint_of_size_t = typename uint_of_size<sizeof(T)>::type; 

}
