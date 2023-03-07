#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <cstring>
#include "utils.h"
#include "packed_array.h"
#include "simple_timer.h"
#include "shiftbf.h"
using namespace std;

//  Allocate memory for a new tower
void tower_alloc(int w, const vector<pair<int, int>> &tower_shape,
                 vector<uint32_t *> &tower_counters,
                 vector<uint8_t *> &overflow_indicators,
                 vector<int> &tower_widths) {
    int level_cnt = tower_shape.size();
    tower_counters.resize(level_cnt);
    overflow_indicators.resize(level_cnt - 1);
    tower_widths.resize(level_cnt);

    int counter_num = w;
    for (int i = 0; i < level_cnt; i++) {
        tower_widths[i] = counter_num;
        int bits = tower_shape[i].first;
        int cnts = tower_shape[i].second;

        int num_per_slot = 8 * sizeof(uint32_t) / bits;
        int packed_width = (counter_num + num_per_slot - 1) / num_per_slot;
        tower_counters[i] = new uint32_t[packed_width];
        memset(tower_counters[i], 0, packed_width * sizeof(uint32_t));

        if (i < level_cnt - 1) {
            int o_packed_width = (counter_num + 7) / 8;
            overflow_indicators[i] = new uint8_t[o_packed_width];
            memset(overflow_indicators[i], 0, o_packed_width * sizeof(uint8_t));
        }

        counter_num = (counter_num + cnts - 1) / cnts;
    }
}

// Tower encoding for counters with values < sep_thld
template <typename CounterType>
double tower_encoding(CounterType *counters, int w, int sep_thld,
                      const vector<pair<int, int>> &tower_shape, int sbf_size,
                      vector<uint32_t *> &tower_counters,
                      vector<uint8_t *> &overflow_indicators, ShiftBF *sbf,
                      vector<int> &tower_widths, bool use_max = false) {
    if (sbf_size > 0) {
        delete sbf;
        sbf = new ShiftBF(sbf_size);
    }

    int level_cnt = tower_shape.size();
    tower_counters.resize(level_cnt);
    overflow_indicators.resize(level_cnt - 1);
    tower_widths.resize(level_cnt);

    //  Allocate memory for tower
    int counter_num = w;
    for (int i = 0; i < level_cnt; i++) {
        tower_widths[i] = counter_num;
        int bits = tower_shape[i].first;
        int cnts = tower_shape[i].second;

        int num_per_slot = 8 * sizeof(uint32_t) / bits;
        int packed_width = (counter_num + num_per_slot - 1) / num_per_slot;
        tower_counters[i] = new uint32_t[packed_width];
        memset(tower_counters[i], 0, packed_width * sizeof(uint32_t));

        if (i < level_cnt - 1) {
            int o_packed_width = (counter_num + 7) / 8;
            overflow_indicators[i] = new uint8_t[o_packed_width];
            memset(overflow_indicators[i], 0, o_packed_width * sizeof(uint8_t));
        }

        counter_num = (counter_num + cnts - 1) / cnts;
    }

    // Extract small counters
    CounterType *small_counters = new CounterType[w];
    memset(small_counters, 0, w * sizeof(CounterType));
    for (int j = 0; j < w; j++) {
        if (counters[j] < sep_thld) small_counters[j] = counters[j];
    }

    SimpleTimer tower_timer;
    tower_timer.start();

    // Encode counters into each level of the tower
#ifdef COUNTER_TYPE_UINT32
    for (int k = 0; k < w; k += 64) {
#ifdef UseShiftEncoder
        uint64_t bitmap = 0;  // Will be inserted into ShiftBfEncoder
#endif
        for (int j = k; j < k + 64 && j < w; j++) {
            uint32_t value = small_counters[j];
            int idx = j;
            for (int i = 0; i < level_cnt; i++) {
                int bits = tower_shape[i].first;
                int cnts = tower_shape[i].second;
                uint32_t curr_val = ACCESS32(tower_counters[i], bits, idx);

                if (i == level_cnt - 1) {  // Top level of tower
                    uint32_t counter_max = (1 << bits) - 1;
                    if (value > counter_max - curr_val) {  // Overflow
                        WRITE32(tower_counters[i], bits, idx, counter_max);
                    } else {
                        if (use_max) {
                            WRITE32(tower_counters[i], bits, idx,
                                    max(curr_val, value) % (1 << bits));
                        } else {  // Use sum
                            WRITE32(tower_counters[i], bits, idx,
                                    (curr_val + value) % (1 << bits));
                        }
                    }
                } else {  // Levels below top
                    if (use_max) {
                        WRITE32(tower_counters[i], bits, idx,
                                max(curr_val, value) % (1 << bits));
                        value = max(curr_val, value) / (1 << bits);
                    } else {  // Use sum
                        WRITE32(tower_counters[i], bits, idx,
                                (curr_val + value) % (1 << bits));
                        value = (curr_val + value) / (1 << bits);
                    }
                    if (value) {  // Overflow to the next level
#ifdef UseShiftEncoder
                        bitmap |= (1ull << (j - k));
#else
                        WRITE8(overflow_indicators[i], 1, idx, 1);
#endif
                    } else {
                        break;
                    }
                    idx /= cnts;
                }
            }
        }
#ifdef UseShiftEncoder
        sbf->insert(bitmap, k);
#endif
    }
#else  // COUNTER_TYPE_UINT32 undefined
    for (int j = 0; j < w; j++) {
        int32_t value = small_counters[j];
        int idx = j;
        for (int i = 0; i < level_cnt; i++) {
            int bits = tower_shape[i].first;
            int cnts = tower_shape[i].second;
            int32_t curr_val =
                U2INT(ACCESS32(tower_counters[i], bits, idx), bits);

            if (i == level_cnt - 1) {  // Top level of tower
                int32_t counter_max = (1 << (bits - 1)) - 1;
                if (value + curr_val > counter_max) {  // Overflow
                    WRITE32(tower_counters[i], bits, idx, counter_max);
                } else if (value + curr_val < -counter_max) {
                    WRITE32(tower_counters[i], bits, idx,
                            INT2U(-counter_max, bits));
                } else {
                    WRITE32(
                        tower_counters[i], bits, idx,
                        INT2U((curr_val + value) % (1 << (bits - 1)), bits));
                }
            } else {  // Levels below top
                WRITE32(tower_counters[i], bits, idx,
                        INT2U((curr_val + value) % (1 << (bits - 1)), bits));
                value = (curr_val + value) / (1 << (bits - 1));
                if (value) {  // Overflow to the next level
                    WRITE8(overflow_indicators[i], 1, idx, 1);
                } else {
                    break;
                }
                idx /= cnts;
            }
        }
    }
#endif
    tower_timer.stop();
    if (small_counters) delete small_counters;

    return tower_timer.elapsedSeconds();
}

// Recover counters from a encoded tower
template <typename CounterType>
double tower_decoding(const vector<pair<int, int>> &tower_shape,
                      const vector<uint32_t *> &tower_counters,
                      const vector<uint8_t *> &overflow_indicators,
                      ShiftBF *sbf, const vector<int> &tower_widths,
                      int ignore_levels,
                      vector<CounterType> &recovered_counters) {
    if (tower_shape.empty()) return -1;
    int level_cnt = tower_shape.size();

    vector<CounterType *> tower_recovered(level_cnt);
    for (int i = 0; i < level_cnt; i++) {
        tower_recovered[i] = new CounterType[tower_widths[i]];
        memset(tower_recovered[i], 0, tower_widths[i] * sizeof(CounterType));
    }

    SimpleTimer timer;
    timer.start();

#ifdef COUNTER_TYPE_UINT32
    for (int j = 0; j < tower_widths[level_cnt - 1]; j++) {
        int top_bits = tower_shape[level_cnt - 1].first;
        tower_recovered[level_cnt - 1][j] =
            ACCESS32(tower_counters[level_cnt - 1], top_bits, j);
    }

    for (int i = level_cnt - 2; i >= 0; i--) {
        int bits = tower_shape[i].first;
        int cnts = tower_shape[i].second;
        for (int k = 0; k < tower_widths[i]; k += 64) {
#ifdef UseShiftEncoder
            uint64_t bitmap = sbf->query(k);
#endif

            for (int j = k; j < k + 64 && j < tower_widths[i]; j++) {
                if (i >= ignore_levels)
                    tower_recovered[i][j] =
                        ACCESS32(tower_counters[i], bits, j);
                else
                    tower_recovered[i][j] = 0;

#ifdef UseShiftEncoder
                if ((bitmap >> (j - k)) & 1) {
#else
                if (ACCESS8(overflow_indicators[i], 1, j)) {
#endif
                    int upper_bits = tower_shape[i + 1].first;
                    uint32_t upper_val = tower_recovered[i + 1][j / cnts];
                    tower_recovered[i][j] += upper_val * (1 << bits);
                }
            }
        }
    }
#else  // COUNTER_TYPE_UINT32 undefined
    for (int j = 0; j < tower_widths[level_cnt - 1]; j++) {
        int top_bits = tower_shape[level_cnt - 1].first;
        tower_recovered[level_cnt - 1][j] = U2INT(
            ACCESS32(tower_counters[level_cnt - 1], top_bits, j), top_bits);
    }

    for (int i = level_cnt - 2; i >= 0; i--) {
        int bits = tower_shape[i].first;
        int cnts = tower_shape[i].second;
        for (int j = 0; j < tower_widths[i]; j++) {
            if (i >= ignore_levels)
                tower_recovered[i][j] =
                    U2INT(ACCESS32(tower_counters[i], bits, j), bits);
            else
                tower_recovered[i][j] = 0;

            if (ACCESS8(overflow_indicators[i], 1, j)) {
                int upper_bits = tower_shape[i + 1].first;
                int32_t upper_val = tower_recovered[i + 1][j / cnts];
                tower_recovered[i][j] += upper_val * (1 << (bits - 1));
            }
        }
    }
#endif
    timer.stop();

    recovered_counters.resize(tower_widths[0]);
    for (int j = 0; j < tower_widths[0]; j++) {
        recovered_counters[j] = tower_recovered[0][j];
    }

    return timer.elapsedSeconds();
}