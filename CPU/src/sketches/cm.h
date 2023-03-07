/*
 * Count-Min Sketch (CM)
 *
 * Cormode G, Muthukrishnan S. An improved data stream summary: the count-min
 * sketch and its applications[J]. Journal of Algorithms, 2005, 55(1): 58-75.
 */

#pragma once

#include "common.h"

class CMArray : public CounterArray<uint32_t> {
  public:
    void insert(const pair<string, float>& pkt, uint32_t f = 1) {
        int i = hash_value(pkt.first);
        if (f <= UINT32_MAX - counters[i]) counters[i] += f;
    }

    uint32_t query(string flow) {
        int i = hash_value(flow);
        return counters[i];
    }
};

class CMSketch : public CounterMatrix<CMArray> {
  public:
    CMSketch(int read_num, int d_) : CounterMatrix<CMArray>(read_num, d_) {
        sketch_name = "CM Sketch";
    }

    void insert(const pair<string, float>& pkt) {
        for (int i = 0; i < d; ++i) arr[i].insert(pkt);
    }

    int query(string flow) {
        uint32_t ret = UINT32_MAX;
        for (int i = 0; i < d; ++i) {
            uint32_t tmp = arr[i].query(flow);
            ret = min(ret, tmp);
        }
        return ret;
    }
};
