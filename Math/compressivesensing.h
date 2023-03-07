#ifndef COMPRESSIVE_SENSING_H
#define COMPRESSIVE_SENSING_H

#include "utils.h"
#include "omp.h"

// Inner product of matrix A and vector x
template<typename CounterType>
std::vector<CounterType> matvec_dot(const std::vector<std::vector<int> > &A, const std::vector<CounterType> &x) {
    assert(A[0].size() == x.size());
    std::vector<int> res;
    for (const std::vector<int> &row : A) {
        int e = inner_product(row.begin(), row.end(), x.begin(), 0);
        res.push_back(e);
    }
    return res;
}

// generate a matrix using the seed
void generate_matrix(std::vector<std::vector<int> > &A, int n, int m, int seed) {
    std::mt19937 generator(seed);
    std::binomial_distribution<int> distribution(1, 0.5);

    A.assign(n, std::vector<int>(m));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            A[i][j] = distribution(generator) & 1;
}

template<typename CounterType>
class CompressiveSensing {
public:
    int w;
    int ratio;
    int num_frags;
    int nw;
    int frag_n;
    int frag_m;
    CounterType zero_thld;
    std::vector<std::vector<int> > A;

    CompressiveSensing(int w_, int ratio_, int num_frags_, CounterType zero_thld_, int seed = 1) {
        init(w_, ratio_, num_frags_, zero_thld_, seed);
    }

    // construct a CompressiveSensing structor using the params
    void init(int w_, int ratio_, int num_frags_, CounterType zero_thld_, int seed = 1) {
        w = w_;
        ratio = ratio_;
        num_frags = std::max(num_frags_, 1);
        zero_thld = zero_thld_;
        nw = (w + num_frags - 1) / num_frags * num_frags;
        frag_n = nw / num_frags;
        frag_m = (frag_n + ratio - 1) / ratio;
        
        generate_matrix(A, frag_m, frag_n, seed);
    }

    // compress a Sketch into some std::vector
    std::vector<std::vector<CounterType> > compress(CounterType *counters, float sparse_level = 0.95) {

        CounterType *bigcounters = new CounterType[nw];
        memset(bigcounters, 0, sizeof(CounterType) * nw);

        for (int i = 0; i < w; i++)
            if (counters[i] >= zero_thld) // encoding for counters with big values
                bigcounters[i] = counters[i];

        std::vector<std::vector<CounterType> > result;
        
        for (int k = 0; k < nw; k += frag_n) {

            int cnts0 = 0;
            std::vector<CounterType> fragcounters(counters + k, counters + k + frag_n);

            for (auto x : fragcounters)
                if (x == 0)
                    ++cnts0;

            if ((float)cnts0 / frag_n < sparse_level) // only compress when the fragment is sparse
                result.push_back(fragcounters);
            else {
                std::vector<CounterType> tmp = matvec_dot(A, fragcounters);
                result.push_back(tmp);
            }
        }

        return result;
    }

    // decompress the std::vector into a Sketch array
    void decompress(CounterType *counters, const std::vector<std::vector<CounterType> > &result, float sparse_level = 0.95) {

        int pos = 0;

        float fl_A[frag_m][frag_n];

        for (int i = 0; i < frag_m; i++)
            for (int j = 0; j < frag_n; j++)
                fl_A[i][j] = A[i][j]; // get the matrix

        for (auto fragcounters : result) {
            if (fragcounters.size() == frag_n) {
                for (auto x : fragcounters) { // fragment is not compressed
                    if (pos >= w) break;
                    counters[pos++] = x;
                }
            }else { // fragment is compressed
                assert(frag_m == fragcounters.size());
                float fl_y[frag_m];
                float x_hat[frag_n];
                
                for (int i = 0; i < frag_m; i++)
                    fl_y[i] = fragcounters[i];

                omp::decompress((float *)fl_A, fl_y, x_hat, frag_m, frag_n); // compressedsensing

                int cnts0 = 0;

                for (int i = 0; i < frag_n; i++)
                    if (std::abs(x_hat[i]) < 1e-5)
                        ++cnts0;

                if ((float)cnts0 / frag_n < sparse_level) { // only decompress when the fragment is sparse
                    for (int i = 0; i < frag_n && pos < w; i++)
                        counters[pos++] = INT_MAX;
                }else {
                    for (int i = 0; i < frag_n && pos < w; i++)
                        counters[pos++] = x_hat[i];
                }
            }
        }
    }
};

#endif
