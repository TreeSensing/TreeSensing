#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include "tower_encoding.h"
using namespace std;

// Aggregate the rhs tower into lhs tower
void tower_aggregate_2l(const vector<pair<int, int>> &tower_shape,
                        vector<uint32_t *> &tower_counters_lhs,
                        vector<uint8_t *> &overflow_indicators_lhs,
                        vector<int> &tower_widths_lhs,
                        const vector<uint32_t *> &tower_counters_rhs,
                        const vector<uint8_t *> &overflow_indicators_rhs,
                        const vector<int> &tower_widths_rhs) {
    int level_cnt = tower_shape.size();
    assert(level_cnt == 2);

    // Aggregate layer 0
    for (int j = 0; j < tower_widths_lhs[0]; j++) {
        // Counter
        uint32_t curr_val_lhs = ACCESS32(tower_counters_lhs[0], 4, j);
        uint32_t curr_val_rhs = ACCESS32(tower_counters_rhs[0], 4, j);
        uint32_t new_val = curr_val_lhs + curr_val_rhs;
        WRITE32(tower_counters_lhs[0], 4, j, new_val % (1 << 4));

        // Overflow indicator
        uint8_t curr_ovf_lhs = ACCESS8(overflow_indicators_lhs[0], 1, j);
        uint8_t curr_ovf_rhs = ACCESS8(overflow_indicators_rhs[0], 1, j);
        WRITE8(overflow_indicators_lhs[0], 1, j, curr_ovf_lhs | curr_ovf_rhs);

        // Overflow to the next layer
        if (new_val >= (1 << 4)) {
            WRITE8(overflow_indicators_lhs[0], 1, j, 1);
            uint32_t next_curr_val = ACCESS32(tower_counters_lhs[1], 8, j / 4);
            uint32_t next_new_val = next_curr_val + (new_val) / (1 << 4);
            uint32_t counter_max = (1 << 8) - 1;
            if (next_new_val > counter_max) {
                WRITE32(tower_counters_lhs[1], 8, j / 4, counter_max);
            } else {
                WRITE32(tower_counters_lhs[1], 8, j / 4, next_new_val);
            }
        }
    }

    // Aggregate layer 1
    for (int j = 0; j < tower_widths_lhs[1]; j++) {
        uint32_t curr_val_lhs = ACCESS32(tower_counters_lhs[1], 8, j);
        uint32_t curr_val_rhs = ACCESS32(tower_counters_rhs[1], 8, j);
        uint32_t new_val = curr_val_lhs + curr_val_rhs;
        uint32_t counter_max = (1 << 8) - 1;
        if (new_val > counter_max) {  // Overflow
            WRITE32(tower_counters_lhs[1], 8, j, counter_max);
        } else {
            WRITE32(tower_counters_lhs[1], 8, j, new_val);
        }
    }
}
