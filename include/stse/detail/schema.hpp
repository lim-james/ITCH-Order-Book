#pragma once

#include "concepts.hpp"
#include "meta_utils.hpp"

#include <cstdint>
#include <string_view>
#include <string>
#include <format>


namespace stse::detail {

template<Scalar T, std::uint8_t depth> std::string generate_schema();
template<StaticContainer T, std::uint8_t depth> std::string generate_schema();
template<Aggregate T, std::uint8_t depth> std::string generate_schema();

template<NotSerializable T, std::uint8_t depth>
std::string generate_schema() = delete("Type not supported for static serialization");


template<std::uint8_t depth>
constexpr std::string_view pad() {
    static_assert(depth <= 127, "Schema nesting depth exceeds maximum supported depth");

    if constexpr (depth == 0) {
        using namespace std::literals;
        return ""sv;
    } else {
        static constexpr auto padding = [] {
            std::array<char, depth * 2> padding{};
            padding.fill(' ');
            return padding;
        }();
        return {padding.data(), padding.size()};
    }
}

template<typename T>
std::string type_header() {
    constexpr auto type = std::meta::display_string_of(^^T);
    return std::format("[{} :: {} bytes]", type, raw_size<T>);
}

template<Scalar T, std::uint8_t depth>
std::string generate_schema() {
    return type_header<T>();
}

template<StaticContainer T, std::uint8_t depth>
std::string generate_schema() {
    auto schema_out = type_header<T>();

    using child_type = typename T::value_type;
    constexpr auto child_info  = std::meta::dealias(^^child_type);
    constexpr auto identifier  = std::meta::display_string_of(child_info);
    constexpr auto child_count = std::tuple_size_v<T>;
    auto meta_data  = generate_schema<child_type, depth + 1>();

    schema_out += std::format(
        "\n{}{}[{}]: {}",
        pad<depth + 1>(),
        identifier,
        child_count,
        meta_data 
    );

    return schema_out;
}

template<Aggregate T, std::uint8_t depth>
std::string generate_schema() {
    static constexpr auto data_members = serializable_members_of<T>;

    constexpr auto fields = data_members.size();
    std::string schema_out = std::format("{} [{} fields]", type_header<T>(), fields);

    template for (constexpr auto member: data_members) {
        constexpr auto identifier = std::meta::identifier_of(member);
        auto meta_data  = generate_schema<typename[:std::meta::type_of(member):], depth + 1>();
        schema_out += std::format(
            "\n{}{}: {}",
            pad<depth + 1>(),
            identifier,
            meta_data 
        );
    }

    return schema_out;
}
}
