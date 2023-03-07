/*
 * Count-Min-Log Sketch (CML)
 *
 * Pitel G, Fouquier G. Count-min-log sketch: Approximately counting with
 * approximate counters[J]. arXiv preprint arXiv:1502.04885, 2015.
 */

#pragma once

#include <random>
#include "common.h"

class CMLArray : public CounterArray<uint32_t> {
  public:
    void insert(const pair<string, float>& pkt, uint32_t f = 1) {
        assert(false);
    }

    uint32_t query(string flow) {
        int i = hash_value(flow);
        return counters[i];
    }
};

class CMLSketch : public CounterMatrix<CMLArray> {
  private:
    constexpr static double log_base = 1.00026;

    static bool increase_decision(int c) {
        return rand() < pow(log_base, -c) * RAND_MAX;
    }

    static double point_value(double c) {
        if (c == 0)
            return 0;
        else
            return pow(log_base, c - 1);
    }

    static double value(int c) {
        if (c <= 1) {
            return point_value(c);
        } else {
            double v = point_value(c + 1);
            return (1 - v) / (double)(1 - log_base);
        }
    }

  public:
    CMLSketch(int read_num, int d_) : CounterMatrix<CMLArray>(read_num, d_) {
        sketch_name = "CML Sketch";
    }

    void insert(const pair<string, float>& pkt) {
        int index[d];
        uint32_t value[d];
        uint32_t min_val = UINT32_MAX;

        for (int i = 0; i < d; ++i) {
            index[i] = arr[i].hash_value(pkt.first);
            value[i] = arr[i].counters[index[i]];
            min_val = min(min_val, value[i]);  // c
        }

        if (increase_decision(min_val)) {
            for (int i = 0; i < d; ++i) {
                if (arr[i].counters[index[i]] == min_val) {
                    arr[i].counters[index[i]] = min_val + 1;
                }
            }
        }
    }

    int query(string flow) {
        uint32_t min_val = UINT32_MAX;
        for (int i = 0; i < d; ++i) {
            min_val = min(min_val, arr[i].query(flow));
        }
        return (int)value(min_val);
    }
};
