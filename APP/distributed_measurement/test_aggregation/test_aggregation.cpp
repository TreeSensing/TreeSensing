#define COUNTER_TYPE_UINT32
#include <iostream>
#include "sketches/cm.h"
#include "init_config.h"
#include "../tower_aggregate.h"
using namespace std;

#define NONLINEAR_CSV_PATH "results/X_SECS_distributed_nonlinear.csv"
#define LINEAR_CSV_PATH "results/X_SECS_distributed_linear.csv"
#define LINEAR_ONLYDECOMP_CSV_PATH \
    "results/X_SECS_distributed_linear_onlydecomp.csv"

const int d = 3;
const int w = 1000000;
const int TOTAL_ITEMS = 27121713;  // CAIDA_1min packet count
const int NUM_THREADS = 8;
const int FRAG_SIZE = 300;
const vector<pair<int, int>> TOWER_SHAPE = {{4, 4}, {8, 1}};

void delete_sketches(const vector<CMSketch *> &sketches) {
    for (CMSketch *p : sketches) {
        if (p) delete p;
    }
}

double do_nonlinear(int node_num) {
    vector<int> slice_idxs = split_integer(TOTAL_ITEMS, node_num);

    vector<CMSketch *> sketches(node_num);
    for (int sk_i = 0; sk_i < node_num; sk_i++) {
        sketches[sk_i] = new CMSketch(-1, d);
        for (int i = 0; i < d; i++) {
            sketches[sk_i]->init_arr(i, nullptr, w, -1, 997 + i);
        }
        sketches[sk_i]->insert_dataset(slice_idxs[sk_i], slice_idxs[sk_i + 1]);

        sketches[sk_i]->compress_towrsen(6, 4096, 1, w / 250, NUM_THREADS, {},
                                         -1);
        printf("Non-linear: sketch %d compressed\n", sk_i);
    }

    double time_nonlinear = 0.0;
    for (int sk_i = 0; sk_i < node_num; sk_i++) {
        double time_temp = sketches[sk_i]->decompress_towrsen(NUM_THREADS);
        printf("Sketch %d, decompression time: %f s\n", sk_i, time_temp);
        time_nonlinear += time_temp;
    }

    delete_sketches(sketches);
    return time_nonlinear;
}

double do_linear(int node_num, double *time_only_decomp = nullptr) {
    vector<int> slice_idxs = split_integer(TOTAL_ITEMS, node_num);

    vector<CMSketch *> sketches(node_num);
    for (int sk_i = 0; sk_i < node_num; sk_i++) {
        sketches[sk_i] = new CMSketch(-1, d);
        for (int i = 0; i < d; i++) {
            sketches[sk_i]->init_arr(i, nullptr, w, -1, 997 + i);
        }
        sketches[sk_i]->insert_dataset(slice_idxs[sk_i], slice_idxs[sk_i + 1]);

        sketches[sk_i]->compress_towrsen(6, 4096, 1, w / FRAG_SIZE, NUM_THREADS,
                                         TOWER_SHAPE, -1);
        printf("Linear: sketch %d compressed\n", sk_i);
    }

    CMSketch combined_sketch(-1, d);
    double time_linear = 0.0;
    for (int i = 0; i < d; i++) {
        combined_sketch.init_arr(i, nullptr, w, -1, 997 + i);

        SimpleTimer timer;
        timer.start();

        // SketchSensing aggregation
        combined_sketch.arr[i].A_frags = sketches[0]->arr[i].A_frags;
        combined_sketch.arr[i].y_frags = sketches[0]->arr[i].y_frags;
        for (int sk_i = 1; sk_i < node_num; sk_i++) {
            for (auto fr_i = 0; fr_i < sketches[sk_i]->arr[i].y_frags.size();
                 fr_i++) {
                for (int j = 0; j < sketches[sk_i]->arr[i].y_frags[fr_i].size();
                     j++) {
                    combined_sketch.arr[i].y_frags[fr_i][j] +=
                        sketches[sk_i]->arr[i].y_frags[fr_i][j];
                }
            }
        }

        // TowerEncoding aggregation
        auto tower_shape = sketches[0]->arr[0].tower_shape;
        vector<uint32_t *> tower_counters_res;
        vector<uint8_t *> overflow_indicators_res;
        vector<int> tower_widths_res;
        tower_alloc(w, tower_shape, tower_counters_res, overflow_indicators_res,
                    tower_widths_res);
        for (int sk_i = 0; sk_i < node_num; sk_i++) {
            tower_aggregate_2l(tower_shape, tower_counters_res,
                               overflow_indicators_res, tower_widths_res,
                               sketches[sk_i]->arr[i].tower_counters,
                               sketches[sk_i]->arr[i].overflow_indicators,
                               sketches[sk_i]->arr[i].tower_widths);
        }

        timer.stop();
        time_linear += timer.elapsedSeconds();
    }
    combined_sketch.compress_algo = "Compressive Sensing (compact)";

    double temp = combined_sketch.decompress_towrsen(NUM_THREADS);
    printf("Aggregated sketch decompression time: %f s\n", temp);
    time_linear += temp;
    if (time_only_decomp) *time_only_decomp = temp;
    delete_sketches(sketches);

    return time_linear;
}

int main() {
    init_config("../../CPU/config.json");

    vector<int> node_num_vec({2, 3, 4, 5, 6, 7, 8, 9, 10});
    FILE *fp_nonlinear = fopen(NONLINEAR_CSV_PATH, "w");
    FILE *fp_linear = fopen(LINEAR_CSV_PATH, "w");
    FILE *fp_linear_od = fopen(LINEAR_ONLYDECOMP_CSV_PATH, "w");
    if (!fp_nonlinear || !fp_linear || !fp_linear_od) {
        fprintf(stderr, "failed to open some files\n");
        exit(-1);
    }
    fprintf(fp_nonlinear, "node_num,nonlinear\n");
    fprintf(fp_linear, "node_num,linear\n");
    fprintf(fp_linear_od, "node_num,linear_onlydecomp\n");

    for (int node_num : node_num_vec) {
        fprintf(fp_nonlinear, "%d", node_num);
        fprintf(fp_linear, "%d", node_num);
        fprintf(fp_linear_od, "%d", node_num);

        double time_nonlinear = do_nonlinear(node_num);
        fprintf(fp_nonlinear, ",%f", time_nonlinear);
        printf("time_nonlinear=%f s\n", time_nonlinear);

        double time_only_decomp;
        double time_linear = do_linear(node_num, &time_only_decomp);
        fprintf(fp_linear, ",%f", time_linear);
        fprintf(fp_linear_od, ",%f", time_only_decomp);
        printf("time_linear=%f s, time_only_decomp=%f\n", time_linear,
               time_only_decomp);

        printf("node_num=%d, %fx faster\n\n", node_num,
               time_nonlinear / time_linear);

        fprintf(fp_nonlinear, "\n");
        fprintf(fp_linear, "\n");
        fprintf(fp_linear_od, "\n");
        fflush(fp_nonlinear);
        fflush(fp_linear);
        fflush(fp_linear_od);
    }

    fclose(fp_nonlinear);
    fclose(fp_linear);
    fclose(fp_linear_od);

    return 0;
}
