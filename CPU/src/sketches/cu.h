/*
 * Conservative Update Sketch (CU)
 *
 * Estan C, Varghese G. New directions in traffic measurement and
 * accounting[C]//Proceedings of the 2002 conference on Applications,
 * technologies, architectures, and protocols for computer communications. 2002:
 * 323-336.
 */

#pragma once

#include "common.h"

class CUArray : public CounterArray<uint32_t> {
  public:
    void insert(const pair<string, float>& pkt, uint32_t f = 1) {
        assert(false);
    };

    uint32_t query(string flow) {
        int i = hash_value(flow);
        return counters[i];
    }
};

class CUSketch : public CounterMatrix<CUArray> {
  public:
    CUSketch(int read_num, int d_) : CounterMatrix<CUArray>(read_num, d_) {
        sketch_name = "CU Sketch";
    }

    void insert(const pair<string, float>& pkt) {
        int index[d];
        uint32_t value[d];
        uint32_t min_val = 1 << 30;

        for (int i = 0; i < d; ++i) {
            index[i] = arr[i].hash_value(pkt.first);
            value[i] = arr[i].counters[index[i]];
            min_val = min(min_val, value[i]);
        }

        uint32_t temp = min_val + 1;
        for (int i = 0; i < d; ++i) {
            arr[i].counters[index[i]] = max(arr[i].counters[index[i]], temp);
        }
    }

    int query(string flow) {
        uint32_t min_val = UINT32_MAX;
        for (int i = 0; i < d; ++i) {
            min_val = min(min_val, arr[i].query(flow));
        }
        return min_val;
    }
};
