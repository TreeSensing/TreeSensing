#define COUNTER_TYPE_UINT32

#include <iostream>
#include <fstream>
#include <vector>
#include "sketches/cm.h"
#include "init_config.h"
using namespace std;

int bytes_of_tower(vector<pair<int, int>> tower_shape, int w) {
    int bytes = 0;

    int level_cnt = tower_shape.size();
    int counter_num = w;
    for (int i = 0; i < level_cnt; i++) {
        int bits = tower_shape[i].first;
        int cnts = tower_shape[i].second;

        int num_per_slot = 32 / bits;
        int packed_width = (counter_num + num_per_slot - 1) / num_per_slot;
        bytes += packed_width * sizeof(uint32_t);

        if (i < level_cnt - 1) {
            int o_packed_width = (counter_num + 7) / 8;
            bytes += o_packed_width * sizeof(uint8_t);
        }

        counter_num = (counter_num + cnts - 1) / cnts;
    }

    bytes += (w / 2 + 7) / 8;  // ShiftBfEncoder

    return bytes;
}

double calc_ratio(vector<pair<int, int>> tower_shape, int w = 1000000) {
    double bytes_before = w * 4;
    double bytes_after = (double)bytes_of_tower(tower_shape, w);
    return bytes_before / bytes_after;
}

int main() {
    init_config("config.json");

    int d = 3;
    int standard_w = 500000;
    double standard_ratio = calc_ratio({{4, 4}, {8, 1}});
    printf("standard_ratio=%f\n", standard_ratio);

    ofstream of("results/X_CE_kappa.csv");
    of << "kappa,2,3,4" << endl;

    for (int kappa = 2; kappa <= 8; kappa += 1) {
        of << kappa;
        for (int level_cnt = 2; level_cnt <= 4; level_cnt++) {
            vector<pair<int, int>> tower_shape;
            int total_bits = 0;
            for (int k = 0; k < level_cnt; k++) {
                if (k == level_cnt - 1)
                    tower_shape.push_back(make_pair(4, 1));
                else
                    tower_shape.push_back(make_pair(4, kappa));
                total_bits += 4;
            }

            double ratio = calc_ratio(tower_shape);
            int should_w = standard_w / standard_ratio * ratio;

            cout << "tower shape: ";
            for (auto pr : tower_shape) {
                printf("(%d, %d), ", pr.first, pr.second);
            }
            printf("ratio=%f -> %d, ", ratio, should_w);

            CMSketch cm(-1, d);
            for (int i = 0; i < d; i++) {
                cm.init_arr(i, nullptr, should_w, -1, 997 + i);
            }
            cm.insert_dataset();

            int sep_thld = 1 << total_bits;
            printf("sep_thld=%d\n", sep_thld);

            vector<vector<uint32_t>> origin_counters_ds;
            for (int i = 0; i < d; i++) {
                vector<uint32_t> origin_counters(should_w, 0);
                for (int j = 0; j < should_w; j++) {
                    uint32_t v = cm.arr[i].counters[j];
                    if (v < sep_thld) origin_counters[j] = v;
                }
                origin_counters_ds.push_back(origin_counters);
            }

            int error = 0;
            int total = 0;
            for (int i = 0; i < d; i++) {
                vector<uint32_t*> tower_counters;
                vector<uint8_t*> overflow_indicators;
                vector<int> tower_widths;
                tower_encoding(cm.arr[i].counters, should_w, sep_thld,
                               tower_shape, -1, tower_counters,
                               overflow_indicators, nullptr, tower_widths);

                vector<uint32_t> recovered_counters;
                tower_decoding(tower_shape, tower_counters, overflow_indicators,
                               nullptr, tower_widths, 0, recovered_counters);

                for (int j = 0; j < should_w; j++) {
                    total++;
                    if (recovered_counters[j] != origin_counters_ds[i][j]) {
                        error += abs((int)recovered_counters[j] -
                                     (int)origin_counters_ds[i][j]);
                    }
                }
            }
            double CE = (double)error / total;
            printf("error %d, total %d, CE %f\n", error, total, CE);
            of << "," << CE;
        }
        of << endl;
    }

    of.close();
    return 0;
}
