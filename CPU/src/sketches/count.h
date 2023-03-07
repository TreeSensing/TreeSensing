/*
 * Count Sketch
 *
 * Charikar M, Chen K, Farach-Colton M. Finding frequent items in data
 * streams[C]//International Colloquium on Automata, Languages, and Programming.
 * Springer, Berlin, Heidelberg, 2002: 693-703.
 */

#pragma once

#include "common.h"

class CountArray : public CounterArray<int32_t> {
  public:
    uint32_t sign_seed;  // Set by CSketch::init_arr()

    void insert(const pair<string, float> &pkt, uint32_t f = 1) {
        int i = hash_value(pkt.first);
        int s = hash_sign(pkt.first);
        counters[i] += s;  // todo: overflow
    }

    int32_t query(string flow) {
        int i = hash_value(flow);
        return counters[i];
    }

    int hash_sign(string flow) {
        int tmp = MurmurHash3_x86_32(flow.data(), flow.length(), sign_seed) % 2;
        return tmp * 2 - 1;
    }
};

class CountSketch : public CounterMatrix<CountArray, int32_t> {
  public:
    CountSketch(int read_num, int d_)
        : CounterMatrix<CountArray, int32_t>(read_num, d_) {
        sketch_name = "Count Sketch";
    }

    void insert(const pair<string, float> &pkt) {
        for (int i = 0; i < d; ++i) arr[i].insert(pkt);
    }

    virtual int query(string flow) {
        int32_t values[d];
        for (int i = 0; i < d; ++i) {
            values[i] = arr[i].query(flow) * arr[i].hash_sign(flow);
        }
        sort(values, values + d);
        int32_t ret = d % 2 == 0 ? ((values[d / 2 - 1] + values[d / 2]) / 2)
                                 : values[d / 2];
        return max(0, ret);
    }

    void init_arr(int dd, int32_t *data, int w, int w0, uint32_t seed) {
        CounterMatrix<CountArray, int32_t>::init_arr(dd, data, w, w0, seed);
        arr[dd].sign_seed = seed + d;
    }
};
