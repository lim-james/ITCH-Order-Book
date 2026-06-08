#pragma once 

#include <cstdint>

#include "big_endian.h"

namespace nasdaq {

template<std::size_t N>
using short_string   = std::array<char, N>;

using PacketSize     = BigEndian<std::uint16_t>;
using MessageType    = char;
using Stock          = short_string<8>;
using MPID           = short_string<4>;
using LocateCode     = BigEndian<std::uint16_t>;
using TrackingNum    = BigEndian<std::uint16_t>;
using ReferenceNum   = BigEndian<std::uint64_t>;
using NumShares4     = BigEndian<std::uint32_t>;
using NumShares8     = BigEndian<std::uint64_t>;
using Price4         = BigEndian<std::uint32_t>;
using Price8         = BigEndian<std::uint64_t>;
using Int4           = BigEndian<std::uint32_t>;
using Int8           = BigEndian<std::uint64_t>;
using ReleaseTime    = BigEndian<std::uint32_t>;

struct [[gnu::packed]] Timestamp {
    std::array<std::uint8_t, 6> raw_timestamp;

    operator std::uint64_t() const noexcept {
        return (static_cast<std::uint64_t>(raw_timestamp[0]) << 40) 
        |      (static_cast<std::uint64_t>(raw_timestamp[1]) << 32)
        |      (static_cast<std::uint64_t>(raw_timestamp[2]) << 24)
        |      (static_cast<std::uint64_t>(raw_timestamp[3]) << 16)
        |      (static_cast<std::uint64_t>(raw_timestamp[4]) << 8)
        |      (static_cast<std::uint64_t>(raw_timestamp[5]));
    }
};

}
