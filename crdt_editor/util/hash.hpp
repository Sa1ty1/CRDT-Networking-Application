#pragma once
#include <iostream>
#include <cstdint>
#include <format>
#include <openssl/sha.h>

std::string hash(const std::string& data);

std::string to_hex(const unsigned char(&digest)[32]);