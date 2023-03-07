/*
g++ -std=c++17 -pthread -O3 extra_tests/encrypt_linear_faster.cpp -o \
extra_tests/encrypt_linear_faster && extra_tests/encrypt_linear_faster
*/
#include <iostream>
#include <string>
#include "sketches/cm.h"
#include "init_config.h"
using namespace std;

const int d = 3;
const int w = 1000000;
const int TOTAL_ITEMS = 27121713;  // CAIDA
const int NUM_THREADS = 8;
const int FRAG_SIZE = 250;

void delete_sketches(const vector<CMSketch *> &sketches) {
    for (CMSketch *p : sketches) {
        if (p) delete p;
    }
}

void compress_and_save(int node_num, const char *filename) {
    vector<int> slice_idxs = split_integer(TOTAL_ITEMS, node_num);

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "failed to open %s\n", filename);
        exit(-1);
    }

    vector<CMSketch *> sketches(node_num);
    for (int sk_i = 0; sk_i < node_num; sk_i++) {
        sketches[sk_i] = new CMSketch(-1, d);
        for (int i = 0; i < d; i++) {
            sketches[sk_i]->init_arr(i, nullptr, w, -1, 997 + i);
        }
        sketches[sk_i]->insert_dataset(slice_idxs[sk_i], slice_idxs[sk_i + 1]);

        sketches[sk_i]->compress_towrsen(6, 4096, 1, w / FRAG_SIZE, NUM_THREADS,
                                         {}, -1);
        printf("Non-linear sk_i=%d compressed\n", sk_i);

        int num_to_encrypt = FRAG_SIZE * 0.05;
        for (int i = 0; i < d; i++) {
            const vector<vector<int>> &y_frags = sketches[sk_i]->arr[i].y_frags;
            for (const vector<int> &y_frag : y_frags) {
                for (int j = 0; j < num_to_encrypt; j++) {
                    fprintf(fp, "%d ", y_frag[j]);
                }
            }
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
}

int main() {
    init_config("../../CPU/config.json");

    vector<int> node_num_vec({2, 3, 4, 5, 6, 7, 8, 9, 10});
    for (int node_num : node_num_vec) {
        char filename[50];
        snprintf(filename, 50, "compressed_sketch_data/%dnodes.txt", node_num);
        compress_and_save(node_num, filename);
    }
}