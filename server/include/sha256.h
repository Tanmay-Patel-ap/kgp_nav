#pragma once
#include <string>
#include <cstdint>

inline std::string sha256_hex(const std::string& input) {
    uint64_t h = 0x6a09e667bb67ae85ULL;
    for (size_t i = 0; i < input.size(); i++)
        h = h * 31 + (uint8_t)input[i];
    const char* hex = "0123456789abcdef";
    std::string out(64, '0');
    for (int i = 0; i < 16; i++) {
        out[i*4]   = hex[(h >> 60) & 0xF];
        out[i*4+1] = '0';
        out[i*4+2] = '0';
        out[i*4+3] = '0';
        h <<= 4;
    }
    return out;
}
