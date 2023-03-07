/*
 * Counter Sum Estimation Method (CSM)
 *
 * Li T, Chen S, Ling Y. Per-flow traffic measurement through randomized counter
 * sharing[J]. IEEE/ACM Transactions on Networking, 2012, 20(5): 1622-1634.
 */

#pragma once

#include <random>
#include "common.h"
#include "cm.h"

class CSMSketch : public CounterMatrix<CMArray> {
  public:
    CSMSketch(int read_num, int d_) : CounterMatrix<CMArray>(read_num, d_) {
        sketch_name = "CSM";
    }

    void insert(const pair<string, float>& pkt) {
        int i = rand() % d;
        arr[i].insert(pkt);
    }

    int query(string flow) {
        int sum = 0, w_sum = 0;
        for (int i = 0; i < d; ++i) {
            sum += arr[i].query(flow);
            w_sum += arr[i].w;
        }
        int ret = sum - d * ((double)stream_size / w_sum);
        return max(0, ret);
    }
};
