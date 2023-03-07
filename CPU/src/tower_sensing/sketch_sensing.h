#pragma once

#include <vector>
#include <random>
#include <thread>
#include "simple_timer.h"
using namespace std;

// SketchSensing for counters with values >= sep_thld
template <typename CounterType>
double sketch_sensing(CounterType* counters, int w, int sep_thld,
                      int round_thld, int ratio, int num_frags, int num_threads,
                      vector<vector<vector<int>>>& A_frags,
                      vector<vector<int>>& y_frags) {
    A_frags.resize(num_frags);
    y_frags.resize(num_frags);

    int m = (w + ratio - 1) / ratio;           // ceil(w / ratio)
    int m1 = (m + num_frags - 1) / num_frags;  // ceil(m / num_frags)
    const vector<int> idxs = split_integer(w, num_frags);

    // Generate random matrices
    std::default_random_engine generator;
    std::binomial_distribution<int> distribution(1, 0.5);
    for (int i = 0; i < num_frags; i++) {
        int frag_len = idxs[i + 1] - idxs[i];
        A_frags[i].assign(m1, vector<int>(frag_len));
        for (int j = 0; j < m1; j++) {
            for (int k = 0; k < frag_len; k++) {
                A_frags[i][j][k] = distribution(generator);
            }
        }
    }

    thread threads[num_threads];
    const vector<int> frag_idxs = split_integer(num_frags, num_threads);

    SimpleTimer timer;
    timer.start();

    for (int i = 0; i < w; i++) {
        CounterType value = counters[i];
        if (abs((int)value) < sep_thld) {  // Discard data
            counters[i] = 0;
        } else {  // Round data
            counters[i] = value / round_thld * round_thld;
        }
    }

    // y=Ax
    auto comp_tfunc = [&idxs, counters, &A_frags, &y_frags](int start,
                                                            int end) {
        for (int i = start; i < end; i++) {
            vector<int> x1(counters + idxs[i], counters + idxs[i + 1]);
            y_frags[i] = matvec_dot(A_frags[i], x1);
        }
    };

    assert(num_threads > 0 && num_threads <= 128);
    switch (num_threads) {
        case 1:  // Serial
            for (int i = 0; i < num_frags; i++) {
                vector<int> x1(counters + idxs[i], counters + idxs[i + 1]);
                y_frags[i] = matvec_dot(A_frags[i], x1);
            }
            break;

        case 2:  // Serial using thread func
            comp_tfunc(0, num_frags);
            break;

        default:  // Multithreading
            for (int t = 0; t < num_threads; t++) {
                threads[t] = thread(comp_tfunc, frag_idxs[t], frag_idxs[t + 1]);
            }
            for (int t = 0; t < num_threads; t++) threads[t].join();
    }

    timer.stop();
    return timer.elapsedSeconds();
}