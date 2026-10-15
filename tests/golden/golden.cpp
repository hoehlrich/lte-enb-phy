#include "golden/golden.hpp"

#include <bit>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <string>

// The format is little-endian and complex samples are interleaved re/im, so
// the bytes can be copied straight into memory only if both hold here.
static_assert(std::endian::native == std::endian::little,
              "golden vectors are little-endian; this machine is not");
static_assert(sizeof(lte::cf32) == 2 * sizeof(float));

namespace golden::detail {
namespace {

[[noreturn]] void fail(const std::filesystem::path& base, const std::string& why) {
    throw std::runtime_error("golden vector " + base.string() + ": " + why);
}

}  // namespace

std::vector<std::byte> read(std::string_view name, std::string_view dtype,
                            std::size_t elem_size, nlohmann::json& meta) {
    // LTE_VECTOR_DIR is set by CMake to the repo's vectors/ directory.
    const std::filesystem::path base = std::filesystem::path(LTE_VECTOR_DIR) / name;
    std::filesystem::path json_path = base;
    json_path += ".json";
    std::filesystem::path bin_path = base;
    bin_path += ".bin";

    std::ifstream json_file(json_path);
    if (!json_file) {
        fail(base, "cannot open .json");
    }
    meta = nlohmann::json::parse(json_file);

    if (meta.at("format") != 1) {
        fail(base, "unsupported format version " + meta.at("format").dump());
    }
    const auto stored = meta.at("dtype").get<std::string>();
    if (stored != dtype) {
        fail(base, "stored as " + stored + " but loaded as " + std::string(dtype));
    }

    const auto shape = meta.at("shape").get<std::vector<std::size_t>>();
    const std::size_t count =
        std::accumulate(shape.begin(), shape.end(), std::size_t{1}, std::multiplies<>{});
    const std::size_t expected = count * elem_size;

    std::ifstream bin_file(bin_path, std::ios::binary);
    if (!bin_file) {
        fail(base, "cannot open .bin");
    }
    const auto actual = std::filesystem::file_size(bin_path);
    if (actual != expected) {
        fail(base, ".bin has " + std::to_string(actual) + " bytes, shape and dtype need " +
                       std::to_string(expected));
    }

    std::vector<std::byte> bytes(expected);
    bin_file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(expected));
    return bytes;
}

}  // namespace golden::detail
