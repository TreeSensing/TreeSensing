#include <iostream>
#include <bits/stdc++.h>
#include "omp.h"
using namespace std;

// Divide `total` into `pieces` as evenly as possible
vector<int> split__integer(int total, int pieces) {
    assert(total > 0 && pieces > 0 && pieces <= total);
    vector<int> indexes;
    int q = total / pieces;
    int r = total % pieces;
    int curr = 0;
    for (int i = 0; i < pieces - r; i++) {
        indexes.push_back(curr);
        curr += q;
    }
    for (int i = 0; i < r; i++) {
        indexes.push_back(curr);
        curr += q + 1;
    }
    indexes.push_back(curr);  // end
    assert(indexes.size() == pieces + 1);
    return indexes;
}

// Inner product of matrix A and vector x
vector<int> matvec_dot(const vector<vector<int>> &A, const vector<int> &x) {
    assert(A[0].size() == x.size());
    vector<int> res;
    for (const vector<int> &row : A) {
        int e = inner_product(row.begin(), row.end(), x.begin(), 0);
        res.push_back(e);
    }
    return res;
}

pair<vector<vector<vector<int>>>, vector<vector<int>>> cs_compress(
        uint32_t *counters, int w, int ratio, int zero_thld, int round_thld,
        int num_frags) {
    vector<vector<vector<int>>> A_frags(num_frags);
    vector<vector<int>> y_frags(num_frags);

    int m = (w + ratio - 1) / ratio;           // ceil(w / ratio)
    int m1 = (m + num_frags - 1) / num_frags;  // ceil(m / num_frags)
    const vector<int> idxs = split__integer(w, num_frags);
    std::default_random_engine generator;
    std::binomial_distribution<int> distribution(1, 0.5);

    // Generate random matrices
    for (int i = 0; i < num_frags; i++) {
        int frag_len = idxs[i + 1] - idxs[i];
        A_frags[i].assign(m1, vector<int>(frag_len));
        for (int j = 0; j < m1; j++) {
            for (int k = 0; k < frag_len; k++) {
                A_frags[i][j][k] = distribution(generator);
            }
        }
    }

    for (int i = 0; i < w; i++) {
        uint32_t value = counters[i];
        if (abs((int)value) <= zero_thld) {  // Discard data
            counters[i] = 0;
        } else {  // Round data
            counters[i] = value / round_thld * round_thld;
        }
    }

    // cout<<"----------------issparse"<<endl;
    // for(auto i:issparse){
    //     cout<<i<<",";
    // }
    // cout<<endl;

    for (int i = 0; i < num_frags; i++) {
        vector<int> x1(counters + idxs[i], counters + idxs[i + 1]);
        y_frags[i] = matvec_dot(A_frags[i], x1);
    }

    return make_pair(A_frags, y_frags);
}

void cs_decompress(vector<vector<vector<int>>> A_frags,
        vector<vector<int>> y_frags, uint32_t *counters, int w) {
    int num_frags = A_frags.size();
    assert(num_frags != 0 && y_frags.size() == num_frags);
    vector<vector<float>> xhat_frags(num_frags);

    for (int i = 0; i < num_frags; i++) {
        // cout << "decomp " << i << " / " << num_frags << endl;
        int m = A_frags[i].size(), n = A_frags[i][0].size();
        assert(m == y_frags[i].size());
        float fl_A[m][n];
        float fl_y[m];
        float x_hat[n];

        for (int j = 0; j < m; j++) {
            for (int k = 0; k < n; k++) {
                fl_A[j][k] = A_frags[i][j][k];
            }
            fl_y[j] = y_frags[i][j];
        }
        omp::decompress((float *)fl_A, fl_y, x_hat, m, n);
        for (int j = 0; j < n; j++) {
            xhat_frags[i].push_back(x_hat[j]);
        }
    }

    int k = 0;
    for (const vector<float> &x_hat : xhat_frags) {
        for (float v : x_hat) {
            counters[k++] = round(v);
        }
    }
}
