#ifndef __C_SKETCH_H__
#define __C_SKETCH_H__

#include "BOBHash32.h"
#include "CMSketch.h"
#include <unordered_map>

class CountSketch : public CMSketch {
  protected:
    // BOBHash32 *sign_hash;
    int qwq;

  public:
    void init(int w, int d, int start = 750) {
        CMSketch::init(w, d, start);
        sign_hash = new BOBHash32[d];
        for (int i = 0; i < d; i++)
            sign_hash[i].initialize(start + 3 + i);
    }

    void copy(const CountSketch &src) {
        CMSketch::copy(src);
        sign_hash = new BOBHash32[d];
        for (int i = 0; i < d; ++i)
            sign_hash[i].initialize(s + 3 + i);
    }
    void clear() {
        CMSketch::clear();
        delete[] sign_hash;
    }
    void insert(const string &key, int key_len = 13, int f = 1) {
        for (int i = 0; i < d; ++i) {
            int index = (hash[i].run(key.c_str(), key_len)) % w;
            int sign = (sign_hash[i].run(key.c_str(), key_len)) % 2;

            counters[index] += sign ? f : -f;
        }
    }
    int query(const string &key, int key_len = 13) {
        int result[d];
        for (int i = 0; i < d; ++i) {
            int index = (hash[i].run(key.c_str(), key_len)) % w;
            int sign = (sign_hash[i].run(key.c_str(), key_len)) % 2;
            for (int t = 0; t < compress_level; t++) {
                index = index / k[t] + signals[t][index];
            }
            result[i] = sign ? counters[index] : -counters[index];
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

    void compressMedian(int in_k, int sig_width = 1, bool ignore0 = true) {
        description = "compress-median";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        int *new_signals = new int[w];
        int *new_counters = new int[w / k];
        memset(new_signals, 0, w * sizeof(int));
        memset(new_counters, 0, (w / k) * sizeof(int));
        for (int i = 0; i < w / k; i++) {
            vector<int> reg;
            for (int j = i * k; j < (i + 1) * k; j++)
                if (counters[j] || !ignore0)
                    reg.push_back(counters[j]);
            new_counters[i] = calculateMedian(reg);
        }
        delete[] counters;
        counters = new_counters;
        signals[layer] = new_signals;
    }
};

#endif
