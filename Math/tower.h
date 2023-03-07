#ifndef TOWER_H
#define TOWER_H

#include "ShiftBF.h"
#include "utils.h"

template <typename CounterType>
class Tower {
  public:
    int w;  // current width (since compression will cause width to change)
    int sbf_size;
    int level_cnt;
    int zero_thld;
    int total_size;
    bool ShiftBF_on;
    bool count_sketch;

    std::vector<std::pair<int, int>> tower_settings;
    std::vector<uint32_t *> tower_counters;
    std::vector<uint8_t *> overflow_indicators;
    std::vector<ShiftBF *> sbfs; // Overflow indicators using Shift Bloom Filter
    std::vector<int> tower_widths;

    // construct a Tower using the params
    Tower(CounterType *counters, int w_, const std::vector<std::pair<int, int>> &tower_settings_ = {{4, 2}, {4, 2}, {4, 1}},
          bool ShiftBF_on_ = true, bool count_sketch_ = true, int zero_thld_ = 4096, int sbf_size_ = -1) {
        init(w_, tower_settings_, ShiftBF_on_, count_sketch_, zero_thld_, sbf_size_);
        compress(counters);
    }

    ~Tower() {
        for (auto arr : tower_counters) delete[] arr;
        for (auto arr : overflow_indicators) delete[] arr;
        for (auto sbf : sbfs) delete sbf;
    }

    void init(int w_, const std::vector<std::pair<int, int>> &tower_settings_ = {{4, 2}, {4, 2}, {4, 1}},
              bool ShiftBF_on_ = true, bool count_sketch_ = true, int zero_thld_ = 4096, int sbf_size_ = -1) { // initialize the Tower
        w = w_;
        
        if (sbf_size_ == -1)
            sbf_size = w / 2;
        else
            sbf_size = sbf_size_;
    
        zero_thld = zero_thld_;
        ShiftBF_on = ShiftBF_on_;
        count_sketch = count_sketch_;
        tower_settings = tower_settings_;

        level_cnt = tower_settings.size();

        int sum = 0;

        sbfs.resize(level_cnt);
        for (int i = 0; i < level_cnt; i++)
            sbfs[i] = new ShiftBF(sbf_size, 97);
        
        tower_counters.resize(level_cnt);
        overflow_indicators.resize(level_cnt - 1);
        tower_widths.resize(level_cnt);

        int counter_num = w;

        for (int i = 0; i < level_cnt; i++) { // initialize the std::vector
            tower_widths[i] = counter_num;
            int bits = tower_settings[i].first;
            int cnts = tower_settings[i].second;

            int num_per_slot = 32 / bits;
            int packed_width =
                (counter_num + num_per_slot - 1) / num_per_slot; // the real size of this level's array
            
            sum += packed_width * 4; // calculate the memory size Tower uses(byte)

            if (tower_counters[i])
                delete tower_counters[i];
            tower_counters[i] = new uint32_t[packed_width];
            memset(tower_counters[i], 0, packed_width * sizeof(uint32_t));

            if (ShiftBF_on == false && i < level_cnt - 1) { // initialize the overflow bitmap
                if (overflow_indicators[i])
                    delete overflow_indicators[i];

                int o_packed_width = (counter_num + 7) / 8;
                sum += o_packed_width;
                overflow_indicators[i] = new uint8_t[o_packed_width];
                memset(overflow_indicators[i], 0,
                        o_packed_width * sizeof(uint8_t));
            }

            counter_num = (counter_num + cnts - 1) / cnts;
        }

        total_size = sum;

        // std::cout << sum << '\n';
    }

    void compress(CounterType *counters) {
        // Tower encoding for counters with small values
        CounterType *small_counters = new CounterType[w];
        memset(small_counters, 0, w * sizeof(CounterType));
        for (int j = 0; j < w; j++) {
            CounterType k = counters[j] > 0 ? counters[j] : -counters[j];
            if (k < zero_thld) small_counters[j] = counters[j];
        }

        // Encode counters into each level of the tower
        for (int k = 0; k < w; k += 64) {
            uint64_t bitmap[level_cnt];
            for (int i = 0; i < level_cnt; i++)
                bitmap[i] = 0;

            for (int j = k; j < k + 64 && j < w; j++) {
                uint32_t value = small_counters[j];
                int idx = j;
                for (int i = 0; i < level_cnt; i++) {
                    int bits = tower_settings[i].first;
                    int cnts = tower_settings[i].second;
                    uint32_t curr_val =
                        ACCESS32(tower_counters[i], bits, idx);

                    if (i == level_cnt - 1) {  // Top level of tower
                        uint32_t counter_max = (1 << bits) - 1;
                        if (value > counter_max - curr_val) {  // Overflow
                            WRITE32(tower_counters[i], bits, idx,
                                    counter_max);
                        } else {
                            WRITE32(tower_counters[i], bits, idx,
                                    (curr_val + value) % (1 << bits));
                        }
                    } else {  // Lower levels of tower
                        WRITE32(tower_counters[i], bits, idx,
                                (curr_val + value) % (1 << bits));
                        value = (curr_val + value) / (1 << bits);
                        if (value) {  // Overflow to the next level
                            if (ShiftBF_on) {
                                bitmap[i] |= (1ull << (j - k));
                            }else {
                                WRITE8(overflow_indicators[i], 1, idx, 1);
                            }
                        }else {
                            break;
                        }
                        idx /= cnts;
                    }
                }
            }
            for (int i = 0; i < level_cnt; i++)
                sbfs[i]->insert(bitmap[i], k);
        }

        delete []small_counters;
    }

    // query the depth of the pos
    int querydepth(int pos) {
        int result = 0;

        int shift = 0;
        int idx = pos;
        for (int i = 0; i < level_cnt; i++) {
            int bits = tower_settings[i].first;
            int cnts = tower_settings[i].second;

            result = i;

            if (i == level_cnt - 1)
                break;

            if (ShiftBF_on == false && ACCESS8(overflow_indicators[i], 1, idx) == 0)
                break;
            
            if (ShiftBF_on == true && (sbfs[i]->query(pos / 64 * 64) >> (pos % 64) & 1) == 0)
                break;

            idx /= cnts;
        }

        return result;
    }

    // query the value
    CounterType query(int pos) {
        CounterType result = 0;

        int shift = 0;
        int idx = pos;
        for (int i = 0; i < level_cnt; i++) {
            int bits = tower_settings[i].first;
            int cnts = tower_settings[i].second;
            int32_t curr_val =
                ACCESS32(tower_counters[i], bits, idx);

            result += curr_val << shift; // add the value into the result
            
            if (i == level_cnt - 1)
                break;

            if (ShiftBF_on == false && ACCESS8(overflow_indicators[i], 1, idx) == 0)
                break;
            
            if (ShiftBF_on == true && (sbfs[i]->query(pos / 64 * 64) >> (pos % 64) & 1) == 0)
                break;

            shift += bits;
            idx /= cnts;
        }

        return result;
    }

    // decompress the Tower into a Sketch
    void decompress(CounterType *counters) {
        for (int i = 0; i < w; i++)
            counters[i] = query(i);
    }

};

#endif
