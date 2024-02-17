#pragma once

template <typename T>
T reverse_bytes(const T& x) {
    constexpr T mask = 255;
    T result = 0;
    if constexpr (sizeof(T) == 2) {
        result |= ((x >> 0) & mask) << (8);
        result |= ((x >> 8) & mask) << (0);
    } else if constexpr (sizeof(T) == 4) {
        result |= ((x >> 0) & mask) << (24);
        result |= ((x >> 8) & mask) << (16);
        result |= ((x >> 16) & mask) << (8);
        result |= ((x >> 24) & mask) << (0);
    } else if constexpr (sizeof(T) == 8) {
        result |= ((x >> 0) & mask) << (56);
        result |= ((x >> 8) & mask) << (48);
        result |= ((x >> 16) & mask) << (40);
        result |= ((x >> 24) & mask) << (32);
        result |= ((x >> 32) & mask) << (24);
        result |= ((x >> 40) & mask) << (16);
        result |= ((x >> 48) & mask) << (8);
        result |= ((x >> 56) & mask) << (0);
    } else {
        return 0;
    }
    return result;
}