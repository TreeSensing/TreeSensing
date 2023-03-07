/*
 * ShiftBfEncoder
 */

#pragma once

#include "murmur3.h"

struct ShiftBF {
    uint8_t *array;
    int w;
    int seed;

    ShiftBF(int size, int _seed = 97) {
        seed = _seed;
        w = (size + 7) / 8;
        array = new uint8_t[w + 10];
        memset(array, 0, sizeof(uint8_t) * (w + 10));
    }

    ~ShiftBF() { delete array; }

    int hash_value(uint32_t key, int t = 0) {
        return MurmurHash3_x86_32(&key, sizeof(key), seed + t) % w;
    }

    void insert(uint64_t value, uint32_t key) {
        for (int i = 0; i < 3; ++i) {
            int pos = hash_value(key, i);
            *(uint64_t *)(array + pos) |= value;
        }
    }

    uint64_t query(uint32_t key) {
        uint64_t res = UINT64_MAX;
        for (int i = 0; i < 3; ++i) {
            int pos = hash_value(key, i);
            res &= *(uint64_t *)(array + pos);
        }
        return res;
    }
};
