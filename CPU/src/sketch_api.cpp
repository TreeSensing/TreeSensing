/* This source file is for generating shared libraries for different sketches.
 * Compile with `g++ -DSKETCH=CMSketch` to generate libraries for different
 * kinds of sketches by replacing "CMSketch" to other sketch names.
 */

#ifndef SKETCH
#pragma message("SKETCH undefined, using CMSketch")
#define SKETCH CMSketch
#endif

// Count Sketches use int32_t counters, while others use uint32_t
#ifndef COUNTER_TYPE
#define COUNTER_TYPE uint32_t
#define COUNTER_TYPE_UINT32
#endif

// e.g. `GLUE(CMSketch, _new)` generates `CMSketch_new`
#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)

// Use ShiftEncoder or 1-bit indicators
// #define UseShiftEncoder

#include <ctime>
#include "init_config.h"
#include "dataset_loader.h"
#include "murmur3.h"
#include "simple_timer.h"
#include "sketches/cm.h"
#include "sketches/count.h"
#include "sketches/cu.h"
#include "sketches/cmm.h"
#include "sketches/cml.h"
#include "sketches/csm.h"

extern "C" {

SKETCH *GLUE(SKETCH, _new)(int read_num, int d, bool is_compsen = false) {
    SKETCH *sketch = new SKETCH(read_num, d);
    if (is_compsen) sketch->compress_algo = "Compressive Sensing (restored)";
    return sketch;
}

void GLUE(SKETCH, _delete)(SKETCH *sketch) { delete sketch; }

void init_arr(SKETCH *sketch, int d_i, COUNTER_TYPE *in_data, int w, int w0,
              uint32_t seed) {
    sketch->init_arr(d_i, in_data, w, w0, seed);
}

void copy_array(SKETCH *sketch, int d_i, COUNTER_TYPE *out_data) {
    for (int i = 0; i < sketch->arr[d_i].w; i++) {
        out_data[i] = sketch->arr[d_i].counters[i];
    }
}

double insert_dataset(SKETCH *sketch, int start, int end) {
    SimpleTimer timer;
    timer.start();
    sketch->insert_dataset(start, end);
    timer.stop();
    return timer.elapsedSeconds();
}

double query_dataset(SKETCH *sketch, int k, double *out_AAE, int ignore_max) {
    return sketch->query_dataset(k, out_AAE, ignore_max);
}

double compress(SKETCH *sketch, int ratio, CompAlgoType algo, int sep_thld,
                int round_thld, int num_frags, int num_threads,
                int *tower_shape_ptr, int sbf_size, double *out_tower_secs) {
    SimpleTimer timer;
    timer.start();

    switch (algo) {
        case HOKUSAI:
            sketch->compress_hokusai(ratio);
            break;
        case ELASTIC:
            sketch->compress_elastic(ratio);
            break;
        case COMPSEN: {
            vector<pair<int, int>> tower_shape;
            while (tower_shape_ptr && *tower_shape_ptr != 0) {
                int bits = *tower_shape_ptr++;
                int cnts = *tower_shape_ptr++;
                tower_shape.push_back(make_pair(bits, cnts));
            }
            return sketch->compress_towrsen(ratio, sep_thld, round_thld,
                                            num_frags, num_threads, tower_shape,
                                            sbf_size, out_tower_secs);
        }
        default:
            fprintf(stderr, "Unknown compression algorithm type %d\n", algo);
            return 0;
    }

    timer.stop();
    return timer.elapsedSeconds();
}

#ifdef SIMD_COMPRESS
double compress_4096_simd(SKETCH *sketch, int ratio) {
    double total_comp_secs = 0;
    for (int d_i = 0; d_i < sketch->d; d_i++) {
        total_comp_secs += sketch->arr[d_i].compress_4096_simd(ratio);
    }
    return total_comp_secs;
}
#endif

double ts_decompress(SKETCH *sketch, int num_threads) {
    return sketch->decompress_towrsen(num_threads);
}

void copy_A_and_y_frags(SKETCH *sketch, int d_i, int *A_frags_data,
                        int *y_frags_data) {
    assert(sketch->compress_algo.substr(0, 19) == "Compressive Sensing");
    int i = 0;
    for (auto A : sketch->arr[d_i].A_frags) {
        for (auto row : A) {
            for (auto e : row) {
                A_frags_data[i++] = e;
            }
        }
    }

    i = 0;
    for (auto y : sketch->arr[d_i].y_frags) {
        for (auto e : y) {
            y_frags_data[i++] = e;
        }
    }
}

// Recover small counters from the tower
double copy_tower_small_counters(SKETCH *sketch, int d_i,
                                 COUNTER_TYPE *out_data, int ignore_levels) {
    auto &tower_shape = sketch->arr[d_i].tower_shape;
    if (tower_shape.empty()) return -1;

    vector<COUNTER_TYPE> recovered_counters;
    double tower_de_secs = tower_decoding<COUNTER_TYPE>(
        sketch->arr[d_i].tower_shape, sketch->arr[d_i].tower_counters,
        sketch->arr[d_i].overflow_indicators, sketch->arr[d_i].sbf,
        sketch->arr[d_i].tower_widths, ignore_levels, recovered_counters);

    // Write the recovered result
    int recovered_w = recovered_counters.size();
    memset(out_data, 0, recovered_w * sizeof(COUNTER_TYPE));
    for (int j = 0; j < recovered_w; j++) {
        out_data[j] = recovered_counters[j];
    }

    return tower_de_secs;
}

void query_and_copy(SKETCH *sketch, int k, int *out_data) {
    int i = 0;
    for (auto flow : sketch->ans) {
        string flow_id = flow.second;
        int true_val = flow.first;
        int est_val = (int)sketch->query(flow_id);
        if (est_val < 0) {
            fprintf(stderr, "est_val=%d, true_val=%d\n", est_val, true_val);
            exit(-1);
        }
        out_data[i] = est_val;
        if (++i == k) break;
    }
}

double calc_acc(int k, int *in_data, double *out_AAE) {
    auto input = load_dataset(-1);
    auto ans = groundtruth(input);

    double ARE = 0.0;
    double AAE = 0.0;

    int i = 0;
    for (auto flow : ans) {
        string flow_id = flow.second;
        int true_val = flow.first;
        int est_val = in_data[i];
        if (est_val < 0) {
            fprintf(stderr, "est_val=%d, true_val=%d\n", est_val, true_val);
            exit(-1);
        }
        int error = est_val - true_val;
        ARE += abs(error) * 1.0 / true_val;
        AAE += abs(error);

        if (debug) {
            printf("i=%d, est_val=%d, true_val=%d, error=%d, ARE+=%f\n", i,
                   est_val, true_val, error, (abs(error) * 1.0 / true_val));
        }

        if (++i == k) {
            if (debug) printf("Top-%d flow real frequency: %d \n", k, true_val);
            break;
        }
    }
    ARE /= i;
    AAE /= i;

    if (k == -1) k = i;
    // printf("\033[1;31mONLY CALC top-%d, ARE %f, AAE %f\033[0m\n", k, ARE,
    // AAE);

    if (out_AAE) *out_AAE = AAE;
    return ARE;
}
}
