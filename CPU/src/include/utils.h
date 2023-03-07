#pragma once

#include <vector>
#include <cassert>
#include <numeric>
using namespace std;

// Divide "total" into "pieces" as evenly as possible
vector<int> split_integer(int total, int pieces) {
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
    vector<int> res;
    for (const vector<int> &row : A) {
        assert(row.size() == x.size());
        int e = inner_product(row.begin(), row.end(), x.begin(), 0);
        res.push_back(e);
    }
    return res;
}
