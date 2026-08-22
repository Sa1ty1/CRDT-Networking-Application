#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <limits>
#include <utility>
#include <stdexcept>

namespace framing {

    constexpr std::size_t HEADER_SIZE = 4;
    constexpr std::size_t MAX_MESSAGE_SIZE = 1024 * 1024;

    std::array<char, 4> encode_length(std::uint32_t length);

    std::uint32_t decode_length(const std::array<char, 4>& header);

    std::string frame_message(const std::string& message);
}