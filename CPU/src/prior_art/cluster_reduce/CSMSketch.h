#ifndef _CR_CSMSKETCH_H_
#define _CR_CSMSKETCH_H_

#include "BOBHash32.h"
#include "CMSketch.h"
#include <unordered_map>
#include <random>

class CSMSketch : public CMSketch {
  public:
    void insert(const string &key, int key_len = 13) {
        int i = rand() % d;
        int idx = hash[i].run(key.c_str(), key_len) % w;
        ++counters[idx];
    }

    int query(const string &key, int key_len = 13) {
        int sum = 0;
        for (int i = 0; i < d; ++i) {
            int idx = hash[i].run(key.c_str(), key_len) % w;
            for (int t = 0; t < compress_level; t++) {
                idx = idx / k[t] + signals[t][idx];
            }
            sum += counters[idx];
        }
        int ret = sum - d * ((double)stream_size / w);
        return ret;
    }
};

#endif
