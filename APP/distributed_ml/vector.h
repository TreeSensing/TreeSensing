#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "quantile_bucket.h"
#include "min_max_sketch.h"
#include "sensing_params.h"

using namespace std;

typedef vector<double> Vector;
bool print_ARE = false;

Vector add(const Vector& vec1, const Vector& vec2)
{
    Vector ret(vec1.size());
    for (int i = 0; i < vec1.size(); ++i)
        ret[i] = vec1[i] + vec2[i];
    return ret;
}

Vector sub(const Vector& vec1, const Vector& vec2)
{
    Vector ret(vec1.size());
    for (int i = 0; i < vec1.size(); ++i)
        ret[i] = vec1[i] - vec2[i];
    return ret;
}

Vector mul(double dbl, const Vector& vec)
{
    Vector ret(vec.size());
    for (int i = 0; i < vec.size(); ++i)
        ret[i] = dbl * vec[i];
    return ret;
}

double dot(const Vector& vec1, const Vector& vec2)
{
    double ret = 0;
    for (int i = 0; i < vec1.size(); ++i)
        ret += vec1[i] * vec2[i];
    return ret;
}

class encodedVector
{
    public:
        int size;
        vector<int> keys, signs;
        Vector quantiles;
        MinMaxSketch min_max_sketch;

        encodedVector(const Vector& vec, int bandwidth, int compress)
        {
            size = vec.size();
            vector<double> vals;
            for (int i = 0; i < vec.size(); ++i)
                if (vec[i] != 0)
                {
                    keys.push_back(i);
                    vals.push_back(abs(vec[i]));
                    signs.push_back(vec[i] >= 0 ? 1 : -1);
                }

            int counter_width = 16;
            int quantile_num = 1 << 10;
            quantiles = quantileSplit(vals, quantile_num);
            auto bucket_id = bucketSort(vals, quantiles);
            for (int i = 0; i < quantiles.size() - 1; ++i)
                quantiles[i] = (quantiles[i] + quantiles[i + 1]) / 2;

            int d = 2, k = 4, sig_width = 2;

            double compress_rate = 1;
            if (compress == 1)
                compress_rate = (double)(counter_width * k) / (counter_width + sig_width * (k - 1));
            else if (compress == 2)
                compress_rate = sensing_ratio;

            int w = bandwidth * compress_rate;
            min_max_sketch.init(w, d, quantile_num);
            for (int i = 0; i < keys.size(); ++i)
                min_max_sketch.insert(keys[i], bucket_id[i]);


            //    min_max_sketch.print();

            if(compress == 2){
//                cout << "_______compressive sensing begin_________" << endl;
                min_max_sketch.CompressiveSensing(sensing_ratio, nfrags, sparse_level);
//                cout << "_______compressive sensing end_________" << endl;
            }

            if (compress == 1)
                min_max_sketch.compressIteration(k, sig_width, quantiles);

            //if(compress == 0)
            //    min_max_sketch.set_zero();

            if (print_ARE)
            {
                double ARE = 0;
                for (int i = 0; i < keys.size(); ++i)
                {
                    int est_idx = min_max_sketch.query(keys[i]);
                    int real_idx = bucket_id[i];
                    double est_val = quantiles[est_idx];
                    double real_val = quantiles[real_idx];
                    double dist = abs(est_val - real_val);
                    ARE += dist / real_val;
                }
                ARE /= keys.size();
                cout << "ARE = " << ARE << endl;
                print_ARE = false;
                cout << keys.size() << " / " << w << endl;
            }
        }

        Vector decode()
        {
            Vector ret(size);
            for (int i = 0; i < keys.size(); ++i)
            {
                int idx = min_max_sketch.query(keys[i]);
                ret[keys[i]] = quantiles[idx] * signs[i];
            }
            return ret;
        }
};

#endif
