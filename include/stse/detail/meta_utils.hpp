#pragma once

#include <meta>
#include <vector>
#include <ranges>
#include <algorithm>

namespace stse::detail {

template<typename Annotation>
consteval bool has_annotation(std::meta::info info) {
    auto annotations = std::meta::annotations_of(info) 
                     | std::views::transform(std::meta::type_of)
                     | std::views::transform(std::meta::remove_cv);

    auto match_annotation = [](auto a) { return a == ^^Annotation; };

    return std::ranges::any_of(annotations, match_annotation);
}

template<typename T>
consteval auto get_all_data_members_of() {
    auto all_data_members = std::vector<std::meta::info>{};

    static constexpr auto bases = std::define_static_array(
        std::meta::bases_of(^^T, std::meta::access_context::unchecked())
    );

    template for (constexpr auto base: bases) {
        auto parent_members = get_all_data_members_of<typename[:std::meta::type_of(base):]>();
        for (auto member: parent_members) all_data_members.push_back(member);
    }

    static constexpr auto data_members = std::define_static_array(
        std::meta::nonstatic_data_members_of(
            ^^T, 
            std::meta::access_context::unchecked()
        )
    );

    template for (constexpr auto member: data_members) all_data_members.push_back(member);

    return std::define_static_array(all_data_members);
}

template<typename T>
inline constexpr auto all_data_members_of = get_all_data_members_of<T>();

static constexpr auto skip_serialization = std::views::filter([](auto info) { 
    return !has_annotation<skipserialization>(info); 
});

static constexpr auto ignore_serialization = std::views::filter([](auto info) { 
    return !has_annotation<ignoreserialization>(info); 
});

template<typename T>
consteval auto get_serializable_members_of() {
    return std::define_static_array(
        all_data_members_of<T> 
        | skip_serialization 
        | ignore_serialization
    );
}

template<typename T>
consteval auto get_non_skipped_members_of() {
    return std::define_static_array(all_data_members_of<T> | skip_serialization);
}

template<typename T>
inline constexpr auto serializable_members_of = get_serializable_members_of<T>();

template<typename T>
inline constexpr auto non_skipped_members_of = get_non_skipped_members_of<T>();

}
