#ifndef COMPRESSIVE_SENSING_H
#define COMPRESSIVE_SENSING_H

#include "limits.h"
#include "util/omp.h"
#include "utils.h"
#include <bits/stdc++.h>
#include <random>

// Inner product of matrix A and vector x
template <typename CounterType>
void matvec_dot(int *Mat, uint32_t *Vec, CounterType *Out, int n, int m) {
  for (int i = 0, s = 0; i < n; i++, s += m) {
    CounterType tmp = 0;
    for (int j = 0; j < m; j++)
      tmp += Mat[s + j] * Vec[j];
    Out[i] = tmp;
  }
}

void generate_matrix(int *&A, int n, int m, int seed) {
  std::mt19937 generator(seed);
  std::binomial_distribution<int> distribution(1, 0.5);

  if (A)
    delete[] A;
  A = new int[n * m];

  for (int i = 0, c = 0; i < n; i++)
    for (int j = 0; j < m; j++)
      A[c++] = distribution(generator) & 1;
}

template <typename CounterType> class CompressiveSensing {
public:
  int w;
  int ratio;
  int num_frags;
  int nw;
  int frag_n;
  int frag_m;
  CounterType zero_thld;
  int *A;

  CompressiveSensing(int w_, int ratio_, int num_frags_, CounterType zero_thld_,
                     int seed = 1) {
    A = NULL;
    init(w_, ratio_, num_frags_, zero_thld_, seed);
  }

  void init(int w_, int ratio_, int num_frags_, CounterType zero_thld_,
            int seed = 1) {
    w = w_;
    ratio = ratio_;
    num_frags = std::max(1, num_frags_);
    zero_thld = zero_thld_;
    nw = (w + num_frags - 1) / num_frags * num_frags;
    frag_n = nw / num_frags;
    frag_m = (frag_n + ratio - 1) / ratio;

    generate_matrix(A, frag_m, frag_n, seed);
  }

  // CounterType as int
  std::pair<size_t, int *> *compress(uint32_t *counters,
                                     float sparse_level = 0.95) {

    int *bigcounters = new int[nw];
    memset(bigcounters, 0, sizeof(int) * nw);

    for (int i = 0; i < w; i++)
      if (counters[i] >= zero_thld)
        bigcounters[i] = counters[i];

    std::pair<size_t, int *> *result = new std::pair<size_t, int *>[num_frags];

    for (int k = 0, p = 0; k < nw; k += frag_n, p++) {

      int cnts0 = 0;

      for (int j = k; j - k < frag_n; j++)
        if (counters[j] == 0)
          ++cnts0;

      if ((float)cnts0 / frag_n < sparse_level) {
        result[p].first = frag_n;
        result[p].second = new int[frag_n];
        memcpy(result[p].second, counters + k, sizeof(int) * frag_n);
      } else {
        result[p].first = frag_m;
        result[p].second = new int[frag_m];
        matvec_dot(A, counters + k, result[p].second, frag_m, frag_n);
      }
    }

    return result;
  }

  void decompress(uint32_t *counters, std::pair<size_t, CounterType *> *result,
                  float sparse_level = 0.95) {

    int pos = 0;

    float fl_A[frag_m][frag_n];

    for (int i = 0; i < frag_m; i++)
      for (int j = 0; j < frag_n; j++)
        fl_A[i][j] = A[i * frag_m + j];

    for (int i = 0; i < num_frags; i++) {
      int sz = result[i].first;
      CounterType *fragcounters = result[i].second;
      if (sz == frag_n) {
        for (int j = 0; j < frag_n; j++) {
          if (pos >= w)
            break;
          counters[pos++] = fragcounters[j];
        }
      } else {
        float fl_y[frag_m];
        float x_hat[frag_n];

        for (int i = 0; i < frag_m; i++)
          fl_y[i] = fragcounters[i];

        omp::decompress((float *)fl_A, fl_y, x_hat, frag_m, frag_n);

        int cnts0 = 0;

        for (int i = 0; i < frag_n; i++)
          if (std::abs(x_hat[i]) < 1e-5)
            ++cnts0;

        if ((float)cnts0 / frag_n < sparse_level) {
          for (int i = 0; i < frag_n && pos < w; i++)
            counters[pos++] = UINT32_MAX;
        } else {
          for (int i = 0; i < frag_n && pos < w; i++)
            counters[pos++] = x_hat[i];
        }
      }
    }
  }
};

#endif
