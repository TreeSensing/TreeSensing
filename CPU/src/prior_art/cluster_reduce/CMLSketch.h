#ifndef _CR_CMLSKETCH_H_
#define _CR_CMLSKETCH_H_

#include "BOBHash32.h"
#include "CMSketch.h"
#include <unordered_map>
#include <random>

class CMLSketch : public CMSketch {
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
    void insert(const string &key, int key_len = 13) {
        int index[d];
        int value[d];
        int min_val = INT32_MAX;

        for (int i = 0; i < d; ++i) {
            index[i] = hash[i].run(key.c_str(), key_len) % w;
            value[i] = counters[index[i]];
            min_val = min(min_val, value[i]);  // c
        }

        if (increase_decision(min_val)) {
            for (int i = 0; i < d; ++i) {
                if (counters[index[i]] == min_val) {
                    counters[index[i]] = min_val + 1;
                }
            }
        }
    }

    int query(const string &key, int key_len = 13) {
        int min_val = INT32_MAX;
        for (int i = 0; i < d; ++i) {
            int index = (hash[i].run(key.c_str(), key_len)) % w;
            for (int t = 0; t < compress_level; t++) {
                index = index / k[t] + signals[t][index];
            }
            min_val = min(min_val, counters[index]);
        }
        return (int)value(min_val);
    }
};

#endif
