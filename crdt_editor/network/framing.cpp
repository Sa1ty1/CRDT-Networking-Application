#include <network/framing.hpp>

namespace framing {

    std::array<char, 4> encode_length(std::uint32_t length) {
        return {
            static_cast<char>((length >> 24) & 0xFF),
            static_cast<char>((length >> 16) & 0xFF),
            static_cast<char>((length >> 8) & 0xFF),
            static_cast<char>(length & 0xFF),
        };
    }

    std::uint32_t decode_length(const std::array<char, 4>& header) {
        return (static_cast<std::uint32_t>(static_cast<unsigned char>(header[0])) << 24) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(header[1])) << 16) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(header[2])) << 8) |
                static_cast<std::uint32_t>(static_cast<unsigned char>(header[3]));
    }

    std::string frame_message(const std::string& message) {
        if (message.size() > UINT32_MAX) {
            throw std::length_error("Message is to large to frame");
        }

        auto header = encode_length(static_cast<std::uint32_t>(message.size()));

        std::string framed;

        framed.reserve(HEADER_SIZE + message.size());

        framed.append(header.data(), header.size());

        framed.append(message);

        return framed;

    }
}