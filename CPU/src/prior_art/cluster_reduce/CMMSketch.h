#ifndef _CR_CMMSKETCH_H_
#define _CR_CMMSKETCH_H_

#include "BOBHash32.h"
#include "CMSketch.h"
#include <unordered_map>

class CMMSketch : public CMSketch {
  public:
    int query(const string &key, int key_len = 13, bool o = true) {
        int result[d];
        for (int i = 0; i < d; ++i) {
            int index = (hash[i].run(key.c_str(), key_len)) % w;
            for (int t = 0; t < compress_level; t++) {
                index = index / k[t] + signals[t][index];
            }
            int value = counters[index];
            int noise = (stream_size - value) / (w/d - 1);
            result[i] = value - noise;
        }

        sort(result, result + d);

        int mid = d / 2;
        int ret;
        if (d % 2 == 0)
            ret = (result[mid] + result[mid - 1]) / 2;
        else
            ret = result[mid];

        return ret;
    }
};

#endif
