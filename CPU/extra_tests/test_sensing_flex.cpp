#include <cstdio>
#include <random>
#include <vector>
#include <numeric>
#include "sketches/cm.h"
#include "init_config.h"
using namespace std;

const int READ_NUM = -1;
const int D = 3;
const int W = 65000;
const int K = 500;
const int SEED = 997;
const int NUM_FRAGS = 300;  // Fragments per array

const int ALL_COUNTERS = D * W;
const int ALL_FRAGS = NUM_FRAGS * D;

const int INVALID_VAL = 1000000;

int main() {
    init_config("config.json");

    FILE *fp_VQR = fopen("./results/recon_valid.csv", "w");
    FILE *fp_ARE = fopen("./results/recon_ARE.csv", "w");
    fprintf(fp_VQR, "rate,2,3,4,5\n");  // d=2..6
    fprintf(fp_ARE, "rate,2,3,4,5\n");

    for (int r = 50; r <= 100; r += 5) {
        double rate = r / 100.0;  // Reconstruction rate
        fprintf(fp_VQR, "%.2f", rate);
        fprintf(fp_ARE, "%.2f", rate);
        for (int d = 2; d < 6; d++) {
            vector<double> VQRs, AREs;
            for (int _ = 0; _ < 10; _++) {  // Reduce accidental results
                int w = ALL_COUNTERS / d;
                int invalid_frags = ALL_FRAGS * (1 - rate);

                CMSketch sketch(READ_NUM, d);
                for (int i = 0; i < d; i++) {
                    sketch.init_arr(i, nullptr, w, -1, SEED + i);
                }
                sketch.insert_dataset();

                const vector<int> idxs = split_integer(w, ALL_FRAGS / d);

                // Randomly select some fragments in which all counters will be
                // set to INVALID_VAL.
                for (int i = 0; i < invalid_frags; i++) {
                    int d_i = rand() % d;
                    int fr_i = rand() % (ALL_FRAGS / d);
                    if (sketch.arr[d_i].counters[idxs[fr_i]] == INVALID_VAL) {
                        i--;
                        continue;
                    }

                    for (int j = idxs[fr_i]; j < idxs[fr_i + 1]; j++) {
                        sketch.arr[d_i].counters[j] = INVALID_VAL;
                    }
                }

                int valid_flows = 0;
                for (int i = 0; i < K; i++) {
                    auto flow = sketch.ans[i];
                    for (int d_i = 0; d_i < d; d_i++) {
                        int est_val = (int)sketch.arr[d_i].query(flow.second);
                        if (est_val < INVALID_VAL) {
                            valid_flows++;
                            break;
                        }
                    }
                }
                double valid_query_rate = (double)valid_flows / K;
                VQRs.push_back(valid_query_rate);
                printf("valid %d / %d (%.3f)\n", valid_flows, K,
                       valid_query_rate);

                double ARE = sketch.query_dataset(K);
                AREs.push_back(ARE);
                printf(
                    "rate=%.2f, d=%d, w=%d, all_frags=%d, invalid_frags=%d, "
                    "ARE=%f\n",
                    rate, d, w, ALL_FRAGS, invalid_frags, ARE);
            }
            fprintf(fp_VQR, ",%f",
                    accumulate(VQRs.begin(), VQRs.end(), 0.0) / VQRs.size());
            fprintf(fp_ARE, ",%f",
                    accumulate(AREs.begin(), AREs.end(), 0.0) / AREs.size());
        }
        fprintf(fp_VQR, "\n");
        fprintf(fp_ARE, "\n");
    }

    fclose(fp_VQR);
    fclose(fp_ARE);
    return 0;
}
