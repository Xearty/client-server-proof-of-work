#pragma once

#include "picosha2.h"
#include <array>
#include <type_traits>
#include <string.h>

using Byte = unsigned char;
using Sha256 = std::array<Byte, picosha2::k_digest_size>;

template <typename... Args>
struct SizeSum;

// Partial specialization for an empty parameter pack (base case)
template <>
struct SizeSum<> {
    static constexpr size_t value = 0; // Sum is 0 when there are no arguments
};

// Recursive partial specialization for non-empty parameter pack
template <typename First, typename... Rest>
struct SizeSum<First, Rest...> {
    static constexpr size_t value = sizeof(First) + SizeSum<Rest...>::value;
};

template <typename T>
struct HasPadding : std::conditional_t<std::has_unique_object_representations_v<T>
    || std::is_same_v<T, float>
    || std::is_same_v<T, double>,
    std::false_type, std::true_type> {};

template <typename T, size_t N>
void to_bytes(const T& val, std::array<Byte, N>& buffer, size_t offset) {
    using PureType = std::remove_cv_t<std::remove_reference_t<T>>;
    static_assert(!std::is_pointer_v<PureType>, "The argument must not be a pointer.");
    static_assert(!HasPadding<PureType>::value, "The argument must not contain padding.");
    memcpy(&buffer[offset], (char*)&val, sizeof(T));
}

template <size_t NBytesToHash>
void sha256_recursive(std::array<Byte, NBytesToHash>& buffer, size_t& offset) {}

template <size_t NBytesToHash, typename First, typename... Rest>
void sha256_recursive(std::array<Byte, NBytesToHash>& buffer, size_t& offset, const First& first, const Rest&... rest) {
    to_bytes(first, buffer, offset);
    offset += sizeof(First);
    sha256_recursive(buffer, offset, rest...);
}

template <typename... Args>
Sha256 sha256(const Args&... args) {
    std::array<Byte, SizeSum<Args...>::value> buffer;
    size_t offset = 0;
    sha256_recursive(buffer, offset, args...);

    Sha256 hashed;
    picosha2::hash256(buffer.begin(), buffer.end(), hashed.begin(), hashed.end());
    return hashed;
}
