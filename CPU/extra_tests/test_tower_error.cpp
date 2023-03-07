#define COUNTER_TYPE_UINT32
#include <bits/stdc++.h>
#include <cmath>
#include "sketches/cm.h"
#include "init_config.h"
using namespace std;

int main() {
    init_config("config.json");

    int d = 3;
    int w = 1000000;
    int sep_thld = 512;
    bool use_max = false;
    vector<pair<int, int>> tower_shape = {{4, 4}, {8, 1}};  // #1
    // vector<pair<int, int>> tower_shape = {{4, 2}, {4, 2}, {4, 1}};  // #2

    CMSketch cm(-1, d);
    for (int i = 0; i < d; i++) {
        cm.init_arr(i, nullptr, w, -1, 997 + i);
    }
    cm.insert_dataset();
    printf("Origin ARE %f\n", cm.query_dataset(-1));

    vector<vector<uint32_t*>> tower_counters_ds(d);
    vector<vector<uint8_t*>> overflow_indicators_ds(d);
    vector<vector<int>> tower_widths_ds(d);

    for (int i = 0; i < d; i++) {
        tower_encoding(cm.arr[i].counters, w, sep_thld, tower_shape, -1,
                       tower_counters_ds[i], overflow_indicators_ds[i], nullptr,
                       tower_widths_ds[i], use_max);
    }

    // ----- Non-pure ratio -----
    int upper_total_counters = 0;
    int upper_nonpure_counters = 0;
    int upper_pure_counters = 0;
    for (int d_i = 0; d_i < d; d_i++) {
        for (int i = 0; i < tower_shape.size() - 1; i++) {
            int bits = tower_shape[i].first;
            int cnts = tower_shape[i].second;
            printf("d_i %d, level %d, bits %d, cnts %d, tower_width %d; \n",
                   d_i, i, bits, cnts, tower_widths_ds[d_i][i]);
            for (int j = 0; j < tower_widths_ds[d_i][i]; j += cnts) {
                int overflows_in_group = 0;
                for (int k = j; k < j + cnts; k++) {
                    if (ACCESS8(overflow_indicators_ds[d_i][i], 1, k))
                        overflows_in_group++;
                }
                if (overflows_in_group > 1)
                    upper_nonpure_counters++;
                else
                    upper_pure_counters++;
            }
        }
    }

    printf(
        "upper_nonpure_counters %d, upper_pure_counters %d, nonpure ratio %f\n",
        upper_nonpure_counters, upper_pure_counters,
        (double)upper_nonpure_counters /
            (upper_nonpure_counters + upper_pure_counters));

    // ----- CE -----
    vector<vector<uint32_t>> recovered_counters_ds(d);

    int total = 0;
    int error = 0;
    for (int d_i = 0; d_i < d; d_i++) {
        vector<uint32_t> origin_counters(w, 0);
        for (int j = 0; j < w; j++) {
            uint32_t origin = cm.arr[d_i].counters[j];
            if (origin < sep_thld) origin_counters[j] = origin;
        }

        vector<uint32_t> recovered_counters;
        tower_decoding(tower_shape, tower_counters_ds[d_i],
                       overflow_indicators_ds[d_i], nullptr,
                       tower_widths_ds[d_i], -1, recovered_counters);
        recovered_counters_ds[d_i] = recovered_counters;
        assert(recovered_counters.size() == w);

        for (int j = 0; j < w; j++) {
            total++;
            if (recovered_counters[j] != origin_counters[j]) {
                // printf("i %d, j %d, recovered %u, old %u, error=%d\n", d_i,
                // j,
                //        recovered_counters[j], origin_counters[j],
                //        recovered_counters[j] - origin_counters[j]);
                error +=
                    abs(int(recovered_counters[j]) - int(origin_counters[j]));
            }
        }
    }
    printf("error %d, total %d, CE \033[1;32;40m%f\033[0m\n", error, total,
           (double)error / total);

    // ----- ARE -----
    CMSketch cm2(-1, d);
    for (int i = 0; i < d; i++) {
        uint32_t arr[w];
        for (int j = 0; j < w; j++) {
            arr[j] = recovered_counters_ds[i][j];
            uint32_t origin_large = cm.arr[i].counters[j];
            if (origin_large >= sep_thld) arr[j] += origin_large;
        }
        cm2.init_arr(i, arr, w, -1, 997 + i);
    }
    printf("Final ARE \033[1;32;40m%f\033[0m\n", cm2.query_dataset(-1));
    return 0;
}