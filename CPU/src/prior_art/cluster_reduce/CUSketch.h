#ifndef __CU_SKETCH_H__
#define __CU_SKETCH_H__

#include "BOBHash32.h"
#include "CMSketch.h"
#include <unordered_map>

class CUSketch : public CMSketch {
public:
  void insert(const string &key, int key_len = 13) {
    int mn_count = INT32_MAX;
    for (int i = 0; i < d; ++i) {
      int idx = hash[i].run(key.c_str(), key_len) % w;
      mn_count = min(mn_count, counters[idx]);
    }
    for (int i = 0; i < d; ++i) {
      int idx = hash[i].run(key.c_str(), key_len) % w;
      if (counters[idx] == mn_count)
        ++counters[idx];
    }
  }
};

#endif
