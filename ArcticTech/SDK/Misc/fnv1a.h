#pragma once

#include <cstdint>

constexpr unsigned int FNV_32_PRIME = 0x01000193;

constexpr uint32_t FNV1a(const char* buf) {
    unsigned int hval = 0x811c9dc5;

    while (*buf)
    {
        hval ^= (unsigned int)*buf++;
        hval *= FNV_32_PRIME;
    }

    return hval;
}