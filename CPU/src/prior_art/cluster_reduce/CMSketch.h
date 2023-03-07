#ifndef __CM_SKETCH_H__
#define __CM_SKETCH_H__

#include "BOBHash32.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>

template <typename T> inline bool chkmin(T &lhs, const T &rhs) {
    return lhs > rhs ? lhs = rhs, true : false;
}

const int max_compress_level = 5;

class CMSketch {
  public:
    int w, d, *k, s, rng;
    int compress_level;
    int *counters, **signals;
    BOBHash32 *hash, *sign_hash;
    int stream_size = 0;  // CMM will use this field

  public:
    string description;

    void init(int in_w, int in_d, int in_s = 750) {
        w = in_w, d = in_d, s = in_s, compress_level = 0, rng = 0;
        k = new int[max_compress_level];
        counters = new int[w];
        signals = new int *[max_compress_level];
        memset(counters, 0, w * sizeof(int));
        memset(k, 0, max_compress_level * sizeof(int));
        k[0] = 1;
        signals[0] = new int[w];
        memset(signals[0], 0, w * sizeof(int));
        hash = new BOBHash32[d];
        for (int i = 0; i < d; ++i)
            hash[i].initialize(s + i);
        description = "non-compress";
    }

    int size() {
        int u = w;
        for (int i = 0; i < compress_level; i++)
            u /= k[i];
        return u;
    }
    void clear() {
        delete[] counters;
        delete[] signals;
        delete[] hash;
    }

    void copy(const CMSketch &src) {
        w = src.w, d = src.d, s = src.s, compress_level = src.compress_level,
        rng = src.rng;
        counters = new int[w];
        signals = new int *[max_compress_level];
        k = new int[max_compress_level];
        memcpy(k, src.k, max_compress_level * sizeof(int));
        memcpy(counters, src.counters, w * sizeof(int));
        for (int i = 0, cur_w = w; i < max(compress_level, 1); i++) {
            cur_w /= k[i];
            signals[i] = new int[cur_w];
            memcpy(signals[i], src.signals[i], cur_w * sizeof(int));
        }
        hash = new BOBHash32[d];
        for (int i = 0; i < d; ++i)
            hash[i].initialize(s + i);
        description = src.description;
    }

    void writeCounters(const char *filename) {
        ofstream outFile(filename, ios::binary);
        for (int i = 0; i < w; ++i) {
            short est_cnt = counters[i];
            // outFile.write((char *)(&est_cnt), sizeof(short));
            outFile << est_cnt << ",";
        }
    }

    double count0() {
        int cnt = 0;
        for (int i = 0; i < w; ++i)
            cnt += (counters[i] == 0);
        return (double)cnt / w;
    }

    void insert(const string &key, int key_len = 13) {
        for (int i = 0; i < d; ++i) {
            int idx = hash[i].run(key.c_str(), key_len) % w;
            ++counters[idx];
        }
        ++stream_size;
    }

    int query(const string &key, int key_len = 13, bool o = true) {
        int ret = INT32_MAX;
        for (int i = 0; i < d; ++i) {
            int idx = hash[i].run(key.c_str(), key_len) % w;
            for (int t = 0; t < compress_level; t++) {
                idx = idx / k[t] + signals[t][idx];
            }
            ret = min(ret, counters[idx]);
        }
        return ret;
    }

    void compressSum(int in_k, int sig_width = 1) {
        description = "compress-sum";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        int *new_signals = new int[w];
        int *new_counters = new int[w / k];
        memset(new_signals, 0, w * sizeof(int));
        memset(new_counters, 0, (w / k) * sizeof(int));
        for (int i = 0; i < w / k; i++)
            for (int j = i * k; j < (i + 1) * k; j++)
                new_counters[i] += counters[j];
        delete[] counters;
        counters = new_counters;
        signals[layer] = new_signals;
    }

    void compressMax(int in_k, int sig_width = 1) {
        description = "compress-max";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        int *new_signals = new int[w];
        int *new_counters = new int[w / k];
        memset(new_signals, 0, w * sizeof(int));
        memset(new_counters, 0, (w / k) * sizeof(int));
        for (int i = 0; i < w / k; i++)
            for (int j = i * k; j < (i + 1) * k; j++)
                new_counters[i] = max(new_counters[i], counters[j]);
        delete[] counters;
        counters = new_counters;
        signals[layer] = new_signals;
    }

    int calculateMedian(vector<int> reg) {
        int m = reg.size(), n = m + 1 >> 1;
        if (!m)
            return 0;
        int res = 0;
        nth_element(reg.begin(), reg.begin() + n - 1, reg.end());
        if (~m & 1) {
            int t = reg[n - 1];
            nth_element(reg.begin(), reg.begin() + n, reg.end());
            res = (t + reg[n]) >> 1;
        } else
            res = reg[n - 1];
        return res;
    }

    double calculateAve(const vector<int> &reg) {
        if (reg.empty())
            return 0;
        long long sum = 0;
        for (auto x : reg)
            sum += x;
        return (double)sum / (int)reg.size();
    }
    void compressIterationByAverage(int in_k, int sig_width = 1,
                                    bool ignore0 = true) {
        description = "compress-iteration-average";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        int *new_signals = new int[w];
        double *new_counters = new double[w / k];
        int *cnt = new int[w / k];
        memset(new_signals, 0, w * sizeof(int));
        memset(new_counters, 0, (w / k) * sizeof(double));
        memset(cnt, 0, (w / k) * sizeof(int));
        vector<int> *reg;
        reg = new vector<int>[w / k];
        for (int i = 0; i < w / k; ++i)
            for (int j = i * k; j < (i + 1) * k; ++j) {
                if (counters[j] || !ignore0) {
                    reg[i].push_back(counters[j]);
                    ++cnt[i + new_signals[j]];
                }
            }
        for (int i = 0; i < w / k; i++) {
            new_counters[i] = calculateAve(reg[i]);
            reg[i].clear();
        }
        for (int t = 0; t < 10; ++t) {
            for (int i = 0; i < w / k; ++i)
                for (int j = i * k; j < (i + 1) * k; ++j)
                    if (counters[j] || !ignore0) {
                        double min_d = INT32_MAX;
                        int min_r = 0;
                        for (int r = -rng + 1; r <= rng; ++r) {
                            if (i + r < 0 || i + r >= w / k)
                                continue;
                            double dist = (new_counters[i + r] > counters[j])
                                          ? (new_counters[i + r] - counters[j])
                                          : (counters[j] - new_counters[i + r]);
                            if (chkmin(min_d, dist))
                                min_r = r;
                        }
                        new_signals[j] = min_r;
                        reg[i + new_signals[j]].push_back(counters[j]);
                    }

            for (int i = 0; i < w / k; ++i) {
                new_counters[i] = calculateAve(reg[i]);
                reg[i].clear();
            }
        }
        signals[layer] = new_signals;
        for (int i = 0; i < w / k; i++)
            counters[i] = new_counters[i];
        delete[] new_counters;
    }

    void compressIteration(int in_k, int sig_width = 1, bool ignore0 = true
                           /* std::unordered_map<string, int> freq*/) {
        description = "compress-iteration";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        int *new_signals = new int[w];
        int *new_counters = new int[w / k];
        int *cnt = new int[w / k];
        memset(new_signals, 0, w * sizeof(int));
        memset(new_counters, 0, (w / k) * sizeof(int));
        memset(cnt, 0, (w / k) * sizeof(int));
        for (int i = 0; i < w / k; ++i)
            for (int j = i * k; j < (i + 1) * k; ++j)
                if (counters[j] || !ignore0) {
                    new_counters[i] = max(new_counters[i], counters[j]);
                    ++cnt[i + new_signals[j]];
                }

        for (int t = 0; t < 10; ++t) {
            for (int i = 0; i < w / k; ++i)
                for (int j = i * k; j < (i + 1) * k; ++j)
                    if (counters[j] || !ignore0) {
                        int min_d = INT32_MAX, min_r = 0;
                        for (int r = -rng + 1; r <= rng; ++r) {
                            if (i + r < 0 || i + r >= w / k)
                                continue;
                            int dist =
                                (new_counters[i + r] > counters[j])
                                ? (new_counters[i + r] - counters[j])
                                : (counters[j] - new_counters[i + r]); // * cnt[i + r];
                            if (dist < min_d)
                                min_d = dist, min_r = r;
                        }
                        new_signals[j] = min_r;
                    }

            for (int i = 0; i < w / k; ++i)
                new_counters[i] = 0, cnt[i] = 0;
            for (int i = 0; i < w / k; ++i)
                for (int j = i * k; j < (i + 1) * k; ++j)
                    if (counters[j] || !ignore0) {
                        new_counters[i + new_signals[j]] =
                            max(new_counters[i + new_signals[j]], counters[j]);
                    }
        }
        signals[layer] = new_signals;
        delete[] counters;
        counters = new_counters;
    }

    struct group {
        int *arr, len;
        int *prefixSum, *zero_num;

        void init(int *in_arr, int in_l) {
            len = 0;
            arr = new int[in_l];
            zero_num = new int[in_l + 1];
            zero_num[0] = 0;
            for (int i = 0; i < in_l; ++i) {
                arr[len++] = in_arr[i];
            }
            sort(arr, arr + len);
            for (int i = 0; i < in_l; i++)
                zero_num[i + 1] = zero_num[i] + (!arr[i]);
            prefixSum = new int[len + 1];
            prefixSum[0] = 0;
            for (int i = 0; i < len; ++i)
                prefixSum[i + 1] = prefixSum[i] + arr[i];
        }

        int val(int p) {
            return (p >= 0) ? arr[p] : INT32_MIN;
        }

        int max_val() {
            if (!len)
                return INT32_MIN;
            return arr[len - 1];
        }

        int sum(int l, int r) { // [l, r)
            return prefixSum[r] - prefixSum[l];
        }
        int count_zero(int l, int r, bool ignore0) {
            return ignore0 ? zero_num[r] - zero_num[l] : 0;
        }
        int lower_loss(int represent, int p, bool ignore0) {
            int t = represent * (p - count_zero(0, p, ignore0)) - sum(0, p);
            return t;
        }

        int upper_loss(int represent, int p, bool ignore0) {
            int t = represent * (len - p - count_zero(p, len, ignore0)) - sum(p, len);
            return t;
        }
    };
    void compressDP(int in_k, int sig_width = 1, bool ignore0 = true) {
        description = "compress-dp";
        int layer = compress_level++;
        int k = this->k[layer] = in_k, rng = 1 << sig_width - 1;
        int w = this->w;
        for (int t = 0; t < layer; t++)
            w /= this->k[t];
        k = in_k;
        group *g = new group[w / k];
        for (int i = 0; i < w / k; ++i)
            g[i].init(counters + i * k, k);

        int *represent = new int[w / k + 1];
        memset(represent, 0, (w / k + 1) * sizeof(int));

        int **f = new int *[w / k + 1];
        for (int i = 0; i < w / k; ++i) {
            f[i] = new int[2 * k + 1];
            memset(f[i], 10, (2 * k + 1) * sizeof(int));
        }
        int **trans = new int *[w / k + 1];
        for (int i = 1; i < w / k; ++i)
            trans[i] = new int[2 * k + 1];

        for (int j = 1; j < g[0].len; ++j) {
            f[0][j] = g[0].lower_loss(g[0].val(j - 1), j, ignore0);
        }
        for (int j = 0; j < g[0].len; ++j) {
            f[0][j + k] = g[0].upper_loss(g[0].max_val(), j, ignore0);
        }

        for (int i = 1; i < w / k; ++i) {
            for (int j = 0; j < g[i].len; ++j) {
                for (int p = 0; p < g[i - 1].len; ++p) {
                    int represent = max(g[i].val(j - 1), g[i - 1].max_val());
                    int tmp_loss = f[i - 1][p] + g[i].lower_loss(represent, j, ignore0) +
                                   g[i - 1].upper_loss(represent, p, ignore0);
                    if (chkmin(f[i][j], tmp_loss))
                        trans[i][j] = p;
                }
                for (int p = 0; p < g[i - 1].len; ++p) {
                    int represent = max(g[i].val(j - 1), g[i - 1].val(p - 1));
                    int tmp_loss = f[i - 1][p + k] + g[i].lower_loss(represent, j, ignore0) +
                                   g[i - 1].lower_loss(represent, p, ignore0);
                    if (chkmin(f[i][j], tmp_loss))
                        trans[i][j] = p + k;
                }
            }
            // cerr << "!!\n";
            for (int j = 0; j < g[i].len; ++j) {
                for (int p = 0; p < g[i - 1].len; ++p) {
                    int represent = max(g[i].max_val(), g[i - 1].max_val());
                    int tmp_loss = f[i - 1][p] + g[i].upper_loss(represent, j, ignore0) +
                                   g[i - 1].upper_loss(represent, p, ignore0);
                    if (chkmin(f[i][j + k], tmp_loss))
                        trans[i][j + k] = p;
                }
                for (int p = 0; p < g[i - 1].len; ++p) {
                    int represent = max(g[i].max_val(), g[i - 1].val(p - 1));
                    int tmp_loss = f[i - 1][p + k] + g[i].upper_loss(represent, j, ignore0) +
                                   g[i - 1].lower_loss(represent, p, ignore0);
                    if (chkmin(f[i][j + k], tmp_loss))
                        trans[i][j + k] = p + k;
                }
            }
        }

        int cur = k;
        // printf("%d\n", f[w / k - 1][cur]);
        int *new_signals = new int[w];
        memset(new_signals, 0, w * sizeof(int));
        for (int i = w / k - 1; i >= 0; --i) {
            if (cur < k) {
                int bound = g[i].val(cur - 1);
                for (int j = i * k; j < (i + 1) * k; ++j)
                    new_signals[j] = (counters[j] > bound);
            } else {
                int bound = g[i].val(cur - k - 1);
                for (int j = i * k; j < (i + 1) * k; ++j)
                    new_signals[j] = (counters[j] <= bound);
            }

            if (i > 0)
                cur = trans[i][cur];
        }
        int *new_counters = new int[w / k + 1];
        memset(new_counters, 0, (w / k + 1) * sizeof(int));
        for (int i = 0; i < w; ++i) {
            new_counters[i / k + new_signals[i]] =
                max(new_counters[i / k + new_signals[i]], counters[i]);
        }
        signals[layer] = new_signals;
        delete[] counters;
        counters = new_counters;
        delete[] * f;
        delete[] f;
    }
};

#endif
