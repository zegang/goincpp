// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_REFLECT_TYPE_HPP
#define GOINCPP_REFLECT_TYPE_HPP

namespace goincpp {
namespace reflect {

#ifdef __cpp_lib_concepts
#include <concepts>

template <typename T>
consteval bool is_comparable() {
    return std::is_same_v<decltype(std::declval<T>() == std::declval<T>()), int>;
}

#else // __cpp_lib_concepts

#include <type_traits>

template <typename, typename = void>
struct is_comparable : std::false_type {};

template <typename T>
struct is_comparable<T,
                     std::void_t< decltype(std::declval<T>() == std::declval<T>()) >
                    > : std::true_type {};

#endif

}
}

#endif // GOINCPP_REFLECT_TYPE_HPP
