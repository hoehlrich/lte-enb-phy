#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "lte/common/types.hpp"

namespace golden {

// The element types a vector can be loaded as, and the dtype each must have
// been stored with. Any other T fails to compile.
template <typename T>
struct DType;
template <>
struct DType<std::uint8_t> {
    static constexpr std::string_view name = "u8";
};
template <>
struct DType<std::int32_t> {
    static constexpr std::string_view name = "i32";
};
template <>
struct DType<float> {
    static constexpr std::string_view name = "f32";
};
template <>
struct DType<lte::cf32> {
    static constexpr std::string_view name = "cf32";
};

template <typename T>
struct Vector {
    std::vector<T> data;             // column-major, as MATLAB stored it
    std::vector<std::size_t> shape;  // MATLAB's size(): always at least 2 dims
    nlohmann::json config;           // the parameters the generator used
};

namespace detail {
// Reads vectors/<name>.json and vectors/<name>.bin. Checks the format version,
// that the stored dtype matches, and that the byte count matches the shape.
// Throws std::runtime_error naming the vector if any check fails.
std::vector<std::byte> read(std::string_view name, std::string_view dtype, std::size_t elem_size,
                            nlohmann::json &meta);
}  // namespace detail

// golden::load<lte::cf32>("ofdm_mod/cell0_grid")
template <typename T>
Vector<T> load(std::string_view name) {
    nlohmann::json meta;
    const std::vector<std::byte> bytes = detail::read(name, DType<T>::name, sizeof(T), meta);

    Vector<T> v;
    v.data.resize(bytes.size() / sizeof(T));
    std::memcpy(v.data.data(), bytes.data(), bytes.size());
    v.shape = meta.at("shape").get<std::vector<std::size_t>>();
    v.config = meta.at("config");
    return v;
}

}  // namespace golden
