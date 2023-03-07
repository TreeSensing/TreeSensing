#ifndef __QUANTILE_BUCKET_H__
#define __QUANTILE_BUCKET_H__

#include <vector>
#include <algorithm>
#include "kll/kll_sketch.hpp"

using namespace std;

vector<double> quantileSplit(const vector<double>& vals, int q = 256)
{
	datasketches::kll_sketch<double> sketch;
	for (auto val : vals)
		sketch.update(val);

	vector<double> quantiles(q);
	for (int i = 0; i <= q; ++i){
		quantiles[i] = sketch.get_quantile((double)i / q);
  }

	return quantiles;
}

vector<int> bucketSort(const vector<double>& vals, const vector<double> quantiles)
{
	vector<int> bucket_id(vals.size());
	for (int i = 0; i < vals.size(); ++i)
		bucket_id[i] = upper_bound(quantiles.begin(), quantiles.end(), vals[i]) - quantiles.begin() - 1;
	return bucket_id;
}

#endif
