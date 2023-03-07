#ifndef __MIN_MAX_SKETCH_H__
#define __MIN_MAX_SKETCH_H__

#include <cstring>
#include <cassert>
#include <algorithm>
#include "BOBHash32.h"
#include "demo.h"
#include <map>
#include "sensing_params.h"
#include <fstream>

class MinMaxSketch
{
    private:
        int w, d, k, max_value;
        int* counters, * signals;
        BOBHash32* hash;

    public:
        void init(int in_w, int in_d, int in_max)
        {
            w = in_w, d = in_d, k = 1, max_value = in_max;
            counters = new int[w];
            signals = new int[w];
            for (int i = 0; i < w; ++i)
                counters[i] = max_value;
            memset(signals, 0, w * sizeof(int));
            hash = new BOBHash32[d];
            for (int i = 0; i < d; ++i)
                hash[i].initialize(750 + i);
        }
        /*
           void print()
           {
           for (int i = 0; i < w; ++i)
           {
           int idx = i / k + signals[i];
           int val = (counters[idx] == max_value ? -1 : counters[idx]);
           cout << val << " ";
           }
           cout << endl << endl;
           }
           */

        void print() {
            map<int,int> mp;
            int zero_counter=0;
            for (int i = 0; i < w; ++i)
            {
                int idx = i / k + signals[i];
                int val = (counters[idx] == max_value ? -1 : counters[idx]);
                if(val == -1)
                    zero_counter++;
                if(mp.count(val)!=0)
                    mp[val]++;
                else
                    mp[val]=1;
            }
            cout<<endl<<"___________fre_count___________"<<endl;
            for(auto it=mp.begin();it!=mp.end();it++){
                cout<<it->first<<'\t'<<it->second<<endl;
            }
            cout<<"zero_counters/w="<<zero_counter<<"/"<<w<<endl;
            cout<<endl<<"___________fre_count___________"<<endl;   
        }

        void clear()
        {
            delete[]counters;
            delete[]signals;
            delete[]hash;
        }

        void set_zero(){
            int zero_cnt = 0;
            for(int i = 0; i < w; ++i){
                if(counters[i] <= zero_thld || counters[i] == max_value)
                    counters[i] = 0;
                if(counters[i] == 0)
                    zero_cnt++;
            }
            cout << "zero_counters:" << zero_cnt << "/" << w << " " << (float)zero_cnt/w << endl;
        }

        void insert(int key, int bucket_id)
        {
            for (int i = 0; i < d; ++i)
            {
                int idx = hash[i].run((char*)&key, 4) % w;
                counters[idx] = min(counters[idx], bucket_id);
                if(counters[idx] <= zero_thld)
                    counters[idx] = 0;
            }
        }

        int query(int key)
        {
            int ret = 0;
            for (int i = 0; i < d; ++i)
            {
                int idx = hash[i].run((char*)&key, 4) % w;
                int val = counters[idx / k + signals[idx]];
                ret = max(ret, val);
            }
            return ret;
        }

        // Divide `total` into `pieces` as evenly as possible
        vector<int> split_integer(int total, int pieces) {
            assert(total > 0 && pieces > 0 && pieces <= total);
            vector<int> res;
            int q = total / pieces;
            int r = total % pieces;
            if (r > 0) {
                for (int i = 0; i < pieces - r; i++) res.push_back(q);
                for (int i = 0; i < r; i++) res.push_back(q + 1);
            } else {
                for (int i = 0; i < pieces; i++) res.push_back(q);
            }
            return res;
        }

        void CompressiveSensing(int ratio, int num_frags, float sparse_level){

            int *counters_copy = new int[w];

            //begin compress
//            cout<<"begin compress..."<<endl;
            vector<vector<vector<int>>> A_frags(num_frags);
            vector<vector<int>> y_frags(num_frags);

            int m = (w + ratio - 1) / ratio;           // ceil(w / ratio)
            int m1 = (m + num_frags - 1) / num_frags;  // ceil(m / num_frags)
            const vector<int> idxs = split__integer(w, num_frags);

            vector<int> issparse(num_frags,1);
            int zero_count = 0;
            int j = 1;

            int sparse_count = 0;
            int zero_counters = 0;
            for (int i = 0; i < w; i++) {
                if (counters[i] == max_value)
                    counters[i] = 0;
                if (counters[i] <= zero_thld)
                    counters[i] = 0;
                counters_copy[i] = counters[i];

                if(counters[i] == 0){
                    zero_count++;
                    zero_counters++;
                }
                if(i+1 == idxs[j]){
                    int frags_len = idxs[j] - idxs[j-1];
                    if(float(zero_count) / frags_len < sparse_level)
                        issparse[j-1] = false;
                    else {
                        issparse[j-1] = true;
                        sparse_count ++;
                    }
                    j++;
                    zero_count = 0;
                }
            }
//            cout << "sparse frags: " << sparse_count << "/" << num_frags << endl;
//            cout << "zero counters: " << zero_counters  << "/" << w << " " << (float)zero_counters / w << endl;

            // Generate random matrices
            std::default_random_engine generator;
            std::binomial_distribution<int> distribution(1, 0.5);

            for (int i = 0; i < num_frags; i++) {
                if(issparse[i] == false)
                    continue;
                int frag_len = idxs[i + 1] - idxs[i];

                //cout << m1 << " " << frag_len << endl; exit(-1);

                A_frags[i].assign(m1, vector<int>(frag_len));
                for (int j = 0; j < m1; j++) {
                    for (int k = 0; k < frag_len; k++) {
                        A_frags[i][j][k] = distribution(generator);
                    }
                }
            }
//            cout << "create matrix" << endl;


            for (int i = 0; i < num_frags; i++) {
                // cout<<i<<"/"<<num_frags<<endl;
                if(issparse[i] == false)
                    continue;
                vector<int> x1(counters + idxs[i], counters + idxs[i + 1]);
                y_frags[i] = matvec_dot(A_frags[i], x1);
            }
//            cout<<"compress end..."<<endl;

            //decompress
            // cout<<"decompress begin..."<<endl;
            assert(num_frags != 0 && y_frags.size() == num_frags);
            vector<vector<float>> xhat_frags(num_frags);

            for (int i = 0; i < num_frags; i++) {
                // cout<<i<<"/"<<num_frags<<endl;

//                if(i) cout << "\33[1A";
//                cout << "Decomp " << i+1 << "/" << num_frags << " frags." << endl;

                if(issparse[i] == false)
                    continue;

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
//                    cout << x_hat[j] << "/" << counters[j+i*n] << " ";
                    xhat_frags[i].push_back(x_hat[j]);
                }
//                cout << endl;
                
//                if(i == 10)
//                    exit(-1);
            }

            int k = 0;
            j = 0;
            for(const vector<float> &x_hat : xhat_frags){
                if(issparse[j++] == false){
                    k = idxs[j];
                }
                else{
                    for(float v : x_hat){
//                        if(v < 0 || v > 1000){
//                            k = idxs[j];
//                            break;
//                        }
                        counters[k++] = round(v);
                    }
                    assert(k == idxs[j]);
                }
            }



            // cout<<"decompress end..."<<endl;

            int same_counters = 0;
            for (int i = 0; i < w; i++) {
                if (abs(counters_copy[i]-counters[i]) < 1) {
                    same_counters++;
                } else {
                    //cout << i << " " << counters_copy[i] << " " << counters[i] << endl;
                    //   printf("i %d, old %d, new %d, diff %d\n", i, counters_copy[i], counters[i], counters[i]-counters_copy[i]);
                }
            }
//            printf("same_counters %d / %d (%f)\n", same_counters, w, (double)same_counters/w);
            delete [] counters_copy;

        }




        void compressIteration(int in_k, int sig_width, vector<double> quantiles){
            k = in_k; int rng = 1 << sig_width - 1;
            int* new_counters = new int[w / k + 1];
            for (int i = 0; i <= w / k; ++i)
                new_counters[i] = max_value;

            for (int t = 0; t < 10; ++t)
            {
                for (int i = 0; i < w / k; ++i)
                    for (int j = i * k; j < min((i + 1) * k, w); ++j)
                        if (counters[j] < max_value)
                        {
                            double min_d = 1e9; int min_r = 0;
                            for (int r = -rng + 1; r <= rng; ++r)
                            {
                                if (i + r < 0 || i + r >= w / k) continue;
                                double dist = abs(quantiles[new_counters[i + r]] - quantiles[counters[j]]);
                                if (dist < min_d)
                                    min_d = dist, min_r = r;
                            }
                            signals[j] = min_r, new_counters[i + signals[j]] = min(new_counters[i + signals[j]], counters[j]);
                        }

                for (int i = 0; i <= w / k; ++i)
                    new_counters[i] = max_value;
                for (int i = 0; i <= w / k; ++i)
                    for (int j = i * k; j < min((i + 1) * k, w); ++j)
                        if (counters[j] < max_value)
                            new_counters[i + signals[j]] = min(new_counters[i + signals[j]], counters[j]);
            }
            delete[]counters;
            counters = new_counters;
        }
};

#endif
