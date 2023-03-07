#ifndef __CLUSTER_H__
#define __CLUSTER_H__

#include "vector.h"

class LinearCluster
{
public:
	Vector W;
	vector<Vector> X;
	vector<double> y;

	Vector calcGrad()
	{
		Vector grad(W.size());
		for (int i = 0; i < X.size(); ++i)
		{
			double pred_y = dot(W, X[i]);
			double deltaY = pred_y - y[i];
			for (int j = 0; j < X[i].size(); ++j)
				grad[j] += deltaY * X[i][j];
		}
		grad = mul(1. / (X.size() * X[0].size()), grad);
		return grad;
	}

	void updateW(const Vector& deltaW)
	{
		W = sub(W, deltaW);
	}

	double test(const vector<Vector>& test_X, const vector<double>& test_y)
	{
		double loss = 0;
		for (int i = 0; i < test_X.size(); ++i)
		{
			double pred_y = dot(W, test_X[i]);
			double L = pow(pred_y - test_y[i], 2);
			loss += L;
		}
		loss /= test_X.size();
		return loss;
	}
};

class LogisticCluster
{
public:
	Vector W;
	vector<Vector> X;
	vector<double> y;

	double sigmoid(double x) { return 1 / (1 + exp(-x)); }

	Vector calcGrad()
	{
		Vector grad(W.size());
		for (int i = 0; i < X.size(); ++i)
		{
			double pred_y = sigmoid(dot(W, X[i]));
			double deltaY = pred_y - y[i];
			for (int j = 0; j < X[i].size(); ++j)
				grad[j] += deltaY * X[i][j];
		}
		grad = mul(1. / X.size(), grad);
		return grad;
	}

	void updateW(const Vector& deltaW)
	{
		W = sub(W, deltaW);
	}

	pair<double, double> test(const vector<Vector>& test_X, const vector<double>& test_y)
	{
		double acc = 0, loss = 0;
		for (int i = 0; i < test_X.size(); ++i)
		{
			double pred_y = sigmoid(dot(W, test_X[i]));
			double L = -(test_y[i] * log(pred_y) + (1 - test_y[i]) * log(1 - pred_y));
			loss += L;
			if (abs(pred_y - test_y[i]) < .5) ++acc;
		}
		acc /= test_X.size();
		loss /= test_X.size();
		return make_pair(acc, loss);
	}
};

#endif
