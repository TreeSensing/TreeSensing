#ifndef TOWER_H
#define TOWER_H

#include "util/ShiftBF.h"
#include "util/utils.h"
#include <utility>

template <typename CounterType> class Tower {
public:
  int w; // current width (since compression will cause width to change)
  int sbf_size;
  int level_cnt;
  int zero_thld;
  bool ShiftBF_on;
  bool count_sketch;

  std::pair<int, int> *tower_settings;
  uint32_t **tower_counters;
  uint8_t **overflow_indicators;
  ShiftBF **sbfs;
  int *tower_widths;

  Tower(CounterType *counters, int w_, int level_cnt_,
        std::pair<int, int> *tower_settings_, bool ShiftBF_on_ = true,
        bool count_sketch_ = true, int zero_thld_ = 4096, int sbf_size_ = -1) {
    init(w_, level_cnt_, tower_settings_, ShiftBF_on_, count_sketch_,
         zero_thld_, sbf_size_);
    compress(counters);
  }

  // the function that bypass the constuctor to do the initialization
  void aux_setter(CounterType *counters, int w_, int level_cnt_,
                  std::pair<int, int> *tower_settings_, bool ShiftBF_on_,
                  bool count_sketch_, int zero_thld_, int sbf_size_) {
    init(w_, level_cnt_, tower_settings_, ShiftBF_on_, count_sketch_,
         zero_thld_, sbf_size_);
    compress(counters);
  }

  ~Tower() {
    for (int i = 0; i < level_cnt; i++) {
      delete[] tower_counters[i];
      delete[] sbfs[i];
      if (i < level_cnt - 1)
        delete overflow_indicators[i];
    }
  }

  void init(int w_, int level_cnt_, std::pair<int, int> *tower_settings_,
            bool ShiftBF_on_ = true, bool count_sketch_ = true,
            int zero_thld_ = 4096, int sbf_size_ = -1) {
    w = w_;

    if (sbf_size_ == -1)
      sbf_size = w / 2;
    else
      sbf_size = sbf_size_;

    level_cnt = level_cnt_;

    zero_thld = zero_thld_;
    ShiftBF_on = ShiftBF_on_;
    count_sketch = count_sketch_;

    tower_settings =
        (std::pair<int, int> *)CALLOC(level_cnt, sizeof(std::pair<int, int>));

    for (int i = 0; i < level_cnt; i++)
      tower_settings[i] = tower_settings_[i];

    sbfs = (ShiftBF **)CALLOC(level_cnt,
                              sizeof(ShiftBF *)); 
    for (int i = 0; i < level_cnt; i++) {
      ShiftBF *temp = (ShiftBF *)CALLOC(1, sizeof(ShiftBF));
      temp->ShiftBF::aux_setter(
          sbf_size,
          97); 
      sbfs[i] = temp;

    
    }
    tower_counters = (unsigned int **)CALLOC(
        level_cnt, sizeof(unsigned int *)); 
    overflow_indicators = (unsigned char **)CALLOC(
        level_cnt - 1,
        sizeof(unsigned char *)); 
    tower_widths = (int *)CALLOC(level_cnt, sizeof(int)); 

    int counter_num = w;
    for (int i = 0; i < level_cnt; i++) {
      tower_widths[i] = counter_num;
      int bits = tower_settings[i].first;
      int cnts = tower_settings[i].second;

      int num_per_slot = 32 / bits;
      int packed_width = (counter_num + num_per_slot - 1) / num_per_slot;

      tower_counters[i] = (unsigned int *)CALLOC(
          packed_width, sizeof(unsigned int)); 
      memset(tower_counters[i], 0, packed_width * sizeof(uint32_t));

      if (ShiftBF_on == false && i < level_cnt - 1) {

        int o_packed_width = (counter_num + 7) / 8;
        overflow_indicators[i] = (unsigned char *)CALLOC(
            o_packed_width,
            sizeof(unsigned char)); 
        memset(overflow_indicators[i], 0, o_packed_width * sizeof(uint8_t));
      }

      counter_num = (counter_num + cnts - 1) / cnts;
    }
  }

  void compress(CounterType *counters) {
    // Tower encoding for counters with small values
    CounterType *small_counters =
        (CounterType *)CALLOC(w, sizeof(CounterType)); 
    memset(small_counters, 0, w * sizeof(CounterType));
    for (int j = 0; j < w; j++) {
      CounterType k = counters[j] > 0 ? counters[j] : -counters[j];
      if (k < zero_thld)
        small_counters[j] = counters[j];
    }

    // Encode counters into each level of the tower
    if (count_sketch == false) {
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
            uint32_t curr_val = ACCESS32(tower_counters[i], bits, idx);

            if (i == level_cnt - 1) { // Top level of tower
              uint32_t counter_max = (1 << bits) - 1;
              if (value > counter_max - curr_val) { // Overflow
                WRITE32(tower_counters[i], bits, idx, counter_max);
              } else {
                WRITE32(tower_counters[i], bits, idx,
                        (curr_val + value) % (1 << bits));
              }
            } else { // Lower levels of tower
              WRITE32(tower_counters[i], bits, idx,
                      (curr_val + value) % (1 << bits));
              value = (curr_val + value) / (1 << bits);
              if (value) { // Overflow to the next level
                if (ShiftBF_on) {
                  bitmap[i] |= (1ull << (j - k));
                } else {
                  WRITE8(overflow_indicators[i], 1, idx, 1);
                }
              } else {
                break;
              }
              idx /= cnts;
            }
          }
        }
        for (int i = 0; i < level_cnt; i++)
          sbfs[i]->insert(bitmap[i], k);
      }
    } else {
      for (int k = 0; k < w; k += 64) {
        uint64_t bitmap[level_cnt];
        for (int i = 0; i < level_cnt; i++)
          bitmap[i] = 0;

        for (int j = k; j < k + 64 && j < w; j++) {
          int32_t value = small_counters[j];
          int idx = j;
          for (int i = 0; i < level_cnt; i++) {
            int bits = tower_settings[i].first;
            int cnts = tower_settings[i].second;
            int32_t curr_val =
                U2INT(ACCESS32(tower_counters[i], bits, idx), bits);

            if (i == level_cnt - 1) { // Top level of tower
              int32_t counter_max = (1 << (bits - 1)) - 1;
              if (value + curr_val > counter_max) { // Overflow
                WRITE32(tower_counters[i], bits, idx, counter_max);
              } else if (value + curr_val < -counter_max) {
                WRITE32(tower_counters[i], bits, idx,
                        INT2U(-counter_max, bits));
              } else {
                WRITE32(tower_counters[i], bits, idx,
                        INT2U((curr_val + value) % (1 << (bits - 1)), bits));
              }
            } else { // Lower levels of tower
              WRITE32(tower_counters[i], bits, idx,
                      INT2U((curr_val + value) % (1 << (bits - 1)), bits));
              value = (curr_val + value) / (1 << (bits - 1));
              if (value) { // Overflow to the next level
                if (ShiftBF_on) {
                  bitmap[i] |= (1ull << (j - k));
                } else {
                  WRITE8(overflow_indicators[i], 1, idx, 1);
                }
              } else {
                break;
              }
              idx /= cnts;
            }
          }
        }
        for (int i = 0; i < level_cnt; i++)
          sbfs[i]->insert(bitmap[i], k);
      }
    }
    // Tower encoding ended

    delete[] small_counters;
  }

  CounterType query(int pos) {
    CounterType result = 0;

    int shift = 0;
    int idx = pos;
    for (int i = 0; i < level_cnt; i++) {
      int bits = tower_settings[i].first;
      int cnts = tower_settings[i].second;
      int32_t curr_val = ACCESS32(tower_counters[i], bits, idx);

      if (count_sketch == true)
        result += curr_val << shift;
      else
        result += U2INT(curr_val, bits) << shift;

      if (i == level_cnt - 1)
        break;

      if (ShiftBF_on == false && ACCESS8(overflow_indicators[i], 1, idx) == 0)
        break;

      if (ShiftBF_on == true &&
          (sbfs[i]->query(pos / 64 * 64) >> (pos % 64) & 1) == 0)
        break;

      shift += bits - (count_sketch == false);
      idx /= cnts;
    }

    return result;
  }

  void decompress(CounterType *counters) {
    for (int i = 0; i < w; i++)
      counters[i] = query(i);
  }
};

#endif
