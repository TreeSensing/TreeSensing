#include <cstdio>
#include <bits/stdc++.h>
#include "CMSketch.h"
#include "CSketch.h"
#include "CUSketch.h"
#include "CMMSketch.h"
#include "CMLSketch.h"
#include "CSMSketch.h"
#include "init_config.h"
#include "dataset_loader.h"
#include "simple_timer.h"
using namespace std;

enum SketchType { CM = 0, Count = 1, CU = 2, CMM = 3, CML = 4, CSM = 5 };
string type2name[] = {"CM", "Count", "CU", "CMM", "CML", "CSM"};

extern "C" {
double run_test(int read_num, int d, int w, int ratio, int k,
                SketchType sketch_type, double* p_secs_used, int method_id,
                bool debug = false);

double distributed_data_stream(int w, int k, int ratio, double* out_AAE,
                               int method_id);
}

template <class SketchClassT>
double calc_ARE(SketchClassT* p_sketch, const vector<pair<int, string>>& ans,
                int k, int* p_real_k, bool debug) {
    double ARE = 0;
    int i = 0;
    for (auto flow : ans) {
        string key = flow.second;
        int true_val = flow.first;
        int est_val = p_sketch->query(key, key.length());

        int error = est_val - true_val;
        ARE += abs(error) * 1.0 / true_val;

        if (debug) {
            printf("%d: est_val=%d, true_val=%d, error=%d, ARE+=%f\n", i,
                   est_val, true_val, error, (abs(error) * 1.0 / true_val));
        }

        if (++i == k) {
            // printf("top-%d flow real frequency: %d\n", k, true_val);
            break;
        }
    }
    ARE /= i;

    if (p_real_k) *p_real_k = i;
    return ARE;
}

template <class SketchClassT>
double test_sketch(SketchClassT* p_sketch, int read_num, int d, int w,
                   int ratio, int k, SketchType sketch_type,
                   double* p_secs_used, int method_id, bool debug) {
    // Insert dataset into sketch
    vector<pair<string, float>> input = load_dataset(read_num);
    vector<pair<int, string>> ans = groundtruth(input);
    p_sketch->init(w, d);
    for (auto pkt : input) {
        string key = pkt.first;
        p_sketch->insert(key, key.length());
    }

    // Perform Cluster Reduce compression
    SimpleTimer timer;
    timer.start();

    switch (method_id) {
        case 0:
            p_sketch->compressSum(ratio, 1);
            break;
        case 1:
            p_sketch->compressMax(ratio, 1);
            break;
        case 2:
            p_sketch->compressIteration(ratio, 1);
            break;
        case 3:
            p_sketch->compressIterationByAverage(ratio, 1);
            break;
        case 4:
            p_sketch->compressDP(ratio, 1);
            break;
        default:
            // no compress
            break;
    }

    timer.stop();
    if (p_secs_used) *p_secs_used = timer.elapsedSeconds();

    // Test ARE
    int real_k;
    double ARE = calc_ARE<SketchClassT>(p_sketch, ans, k, &real_k, debug);
    // printf(
    //     "\033[0m\033[1;31mCluster Reduce (%s %s) ARE for top-%d flows: "
    //     "%lf\033[0m\n",
    //     type2name[sketch_type].c_str(), p_sketch->description.c_str(),
    //     real_k, ARE);

    return ARE;
}

double run_test(int read_num, int d, int w, int ratio, int k,
                SketchType sketch_type, double* p_secs_used, int method_id,
                bool debug) {
    // Create sketch object
    switch (sketch_type) {
        case CM: {
            CMSketch sketch;
            return test_sketch<CMSketch>(&sketch, read_num, d, w, ratio, k,
                                         sketch_type, p_secs_used, method_id,
                                         debug);
        }
        case Count: {
            CountSketch sketch;
            return test_sketch<CountSketch>(&sketch, read_num, d, w, ratio, k,
                                            sketch_type, p_secs_used, method_id,
                                            debug);
        }
        case CU: {
            CUSketch sketch;
            return test_sketch<CUSketch>(&sketch, read_num, d, w, ratio, k,
                                         sketch_type, p_secs_used, method_id,
                                         debug);
        }
        case CMM: {
            CMMSketch sketch;
            return test_sketch<CMMSketch>(&sketch, read_num, d, w, ratio, k,
                                          sketch_type, p_secs_used, method_id,
                                          debug);
        }
        case CML: {
            CMLSketch sketch;
            return test_sketch<CMLSketch>(&sketch, read_num, d, w, ratio, k,
                                          sketch_type, p_secs_used, method_id,
                                          debug);
        }
        case CSM: {
            CSMSketch sketch;
            return test_sketch<CSMSketch>(&sketch, read_num, d, w, ratio, k,
                                          sketch_type, p_secs_used, method_id,
                                          debug);
        }
    }

    fprintf(stderr, "Unknown sketch type %d!\n", sketch_type);
    return -1;
}

double distributed_data_stream(int w, int k, int ratio, double* out_AAE,
                               int method_id) {
    auto input = load_dataset();
    auto ans = groundtruth(input);
    int NODE_NUM = 8;
    vector<int> slice_idxs = {0,        3390215,  6780430,
                              10170645, 13560860, 16951075,
                              20341290, 23731505, (int)input.size()};

    CMSketch* sketches[NODE_NUM];
    for (int sk_i = 0; sk_i < NODE_NUM; sk_i++) {
        sketches[sk_i] = new CMSketch();
        sketches[sk_i]->init(w, 3);

        for (int i = slice_idxs[sk_i]; i < slice_idxs[sk_i + 1]; i++) {
            string key = input[i].first;
            sketches[sk_i]->insert(key, key.length());
        }

        switch (method_id) {
            case 0:
                sketches[sk_i]->compressSum(ratio, 1);
                break;
            case 1:
                sketches[sk_i]->compressMax(ratio, 1);
                break;
            case 2:
                sketches[sk_i]->compressIteration(ratio, 1);
                break;
            case 3:
                sketches[sk_i]->compressIterationByAverage(ratio, 1);
                break;
            case 4:
                sketches[sk_i]->compressDP(ratio, 1);
                break;
            default:
                exit(-1);
        }
        printf("sketches[%d] %s\n", sk_i, sketches[sk_i]->description.c_str());
    }

    vector<int> query_result(k, 0);
    int i = 0;
    for (auto flow : ans) {
        string key = flow.second;
        int true_val = flow.first;
        for (int sk_i = 0; sk_i < NODE_NUM; sk_i++) {
            int est_val = sketches[sk_i]->query(key, key.length());
            if (est_val < 0) {
                fprintf(stderr, "est_val=%d, true_val=%d\n", est_val, true_val);
                exit(-1);
            }
            query_result[i] += est_val;
        }
        if (++i == k) break;
    }

    double ARE = 0.0;
    double AAE = 0.0;
    i = 0;
    for (auto flow : ans) {
        string flow_id = flow.second;
        int true_val = flow.first;
        int est_val = query_result[i];
        if (est_val < 0) {
            fprintf(stderr, "est_val=%d, true_val=%d\n", est_val, true_val);
            exit(-1);
        }
        int error = est_val - true_val;
        ARE += abs(error) * 1.0 / true_val;
        AAE += abs(error);

        if (++i == k) break;
    }
    ARE /= i;
    AAE /= i;

    if (k == -1) k = i;
    // printf("\033[1;31mONLY CALC top-%d, ARE %f, AAE %f\033[0m\n", k, ARE,
    // AAE);

    if (out_AAE) *out_AAE = AAE;
    return ARE;
}
