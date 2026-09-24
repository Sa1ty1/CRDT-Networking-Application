# include <util/hash.hpp>

std::string hash(const std::string& data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];

    SHA256(
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(), 
        digest
    );
    return to_hex(digest);
}

std::string to_hex(const unsigned char(&digest)[32]) {
    std::string result;
    result.reserve(64); // 32 bytes = 64 hex characters
    
    for (unsigned char byte : digest) {
        std::format_to(std::back_inserter(result), "{:02x}", byte);
    }
    
    return result;
}

// Toy hashing
// std::string hash(const std::string& data) {
//     uint64_t value = 7;

//     for (unsigned char byte: data) {
//         value = value * 31 + byte;
//     }
//     return std::format("{:x}", value);
// }