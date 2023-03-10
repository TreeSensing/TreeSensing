/*
 * ShiftEncoder
 */

#ifndef _SHIFTBF_H_
#define _SHIFTBF_H_

#include "util/murmur3.h"
using namespace std;

struct ShiftBF {
    uint8_t *array;
    int w;
    int seed;

    ShiftBF(int size, int _seed) {
        seed = _seed;
        w = (size + 7) / 8;
        array = new uint8_t[w + 10];
        memset(array, 0, sizeof(uint8_t) * (w + 10));
    }

    ~ShiftBF() { delete array; }

    void aux_setter(int size, int _seed)
    {
        seed = _seed;
        w = (size + 7) / 8;
        array = new uint8_t[w + 10];
        memset(array, 0, sizeof(uint8_t) * (w + 10));
    }
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
        uint64_t res = 0xffffffffffffffff;
        for (int i = 0; i < 3; ++i) {
            int pos = hash_value(key, i);
            res &= *(uint64_t *)(array + pos);
        }
        return res;
    }
};

#endif
