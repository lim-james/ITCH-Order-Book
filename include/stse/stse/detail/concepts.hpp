#pragma once

#include "meta_utils.hpp"

#include <type_traits>
#include <array>
#include <utility>

namespace stse::detail {

template<typename T>
concept EndianType = requires {
    { T::endian } -> std::convertible_to<std::endian>;
};

template<typename T>
concept NotSerializable = std::is_pointer_v<T> 
                       || std::is_null_pointer_v<T> 
                       || std::is_polymorphic_v<T>
                       || !std::is_trivially_copyable_v<T>
                       || EndianType<T>;

template<typename T>
concept Scalar = std::is_scalar_v<T>;

template<typename T>
concept StaticContainer = requires {
    typename T::value_type;
    typename std::tuple_size<T>::type;
} && std::is_trivially_copyable_v<typename T::value_type>;

template<typename T>
concept Aggregate = !Scalar<T> &&
                    !StaticContainer<T> &&
                    !NotSerializable<T>;

template<typename T>
[[nodiscard]] consteval bool is_serializable();

template<typename T>
inline constexpr bool is_serializable_v = is_serializable<T>();

template<typename T>
[[nodiscard]] consteval bool is_serializable() {
    using T_NORM = std::decay_t<T>;
    
    if constexpr (NotSerializable<T_NORM>) {
        return false;
    } else if constexpr (Scalar<T_NORM>) {
        return true;
    } else if constexpr (StaticContainer<T_NORM>) {
        return is_serializable_v<typename T_NORM::value_type>;
    } else if constexpr (Aggregate<T_NORM>) {
        template for (constexpr auto member: serializable_members_of<T_NORM>) {
            if (!is_serializable_v<typename[:std::meta::type_of(member):]>) {
                return false;
            }
        }
        return true;
    } else {
        std::unreachable(); 
    }   
}

template<typename T>
concept Serializable = is_serializable<T>();

template<typename T>
[[nodiscard]] consteval bool has_pointer_member();

template<typename T>
inline constexpr bool has_pointer_member_v = has_pointer_member<T>();

template<typename T>
[[nodiscard]] consteval bool has_pointer_member() {
    using T_NORM = std::decay_t<T>;
    
    if constexpr (Scalar<T_NORM>) {
        return std::is_pointer_v<T_NORM>;
    } else if constexpr (StaticContainer<T_NORM>) {
        return has_pointer_member_v<typename T_NORM::value_type>;
    } else if constexpr (Aggregate<T_NORM>) {
        template for (constexpr auto member: all_data_members_of<T_NORM>) {
            if (has_pointer_member_v<typename[:std::meta::type_of(member):]>) {
                return true;
            }
        }
        return false;
    } else {
        std::unreachable(); 
    }   
}

template<typename T>
concept BitCastSafe = !has_pointer_member<T>();

}
