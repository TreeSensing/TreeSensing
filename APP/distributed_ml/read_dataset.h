#ifndef __READ_DATA_H__
#define __READ_DATA_H__

#include <fstream>
#include "vector.h"

char G[][3] = { "CO", "Ea", "Ey", "Me" };

bool readFile(vector<Vector>& X, const char* filename, int D = INT32_MAX)
{
	ifstream inFile(filename);
	if (!inFile.is_open()) return false;
	Vector in_X; double in_x;
	int d_cnt = 0;
	while (inFile >> in_x)
	{
		in_X.push_back(in_x);
		if (++d_cnt == D)
			break;
	}
	X.push_back(in_X);
	return true;
}

void readClassification(vector<Vector>& X, vector<double>& y, const char* dir, int D = INT32_MAX)
{
	ios::sync_with_stdio(false);
	char filename[80];
	for (int B = 1; B <= 5; ++B)
		for (int g_i = 0; g_i < 2; ++g_i)
			for (int F = 10; F <= 100; F += 10)
				for (int R = 1; R <= 4; ++R)
				{
					sprintf(filename, "%s/B%d_G%s_F%03d_R%d.txt", dir, B, G[g_i], F, R);
					if (readFile(X, filename, D))
						y.push_back(g_i);
				}
}

void readRegression(vector<Vector>& X, vector<double>& y, const char* dir, int D = INT32_MAX)
{
	ios::sync_with_stdio(false);
	char filename[80];
	for (int B = 1; B <= 5; ++B)
		for (int g_i = 0; g_i < 4; ++g_i)
			for (int F = 10; F <= 100; F += 10)
				for (int R = 1; R <= 4; ++R)
				{
					sprintf(filename, "%s/B%d_G%s_F%03d_R%d.txt", dir, B, G[g_i], F, R);
					if (readFile(X, filename, D))
						y.push_back(F / 100.);
				}
}

void normalization(vector<Vector>& X)
{
	for (int j = 0; j < X[0].size(); ++j)
	{
		double min_val = 1e9, max_val = -1e9;
		for (int i = 0; i < X.size(); ++i)
			min_val = min(min_val, X[i][j]), max_val = max(max_val, X[i][j]);
		if (max_val == min_val)
		{
			for (int i = 0; i < X.size(); ++i)
				X[i][j] = 0;
			continue;
		}
		for (int i = 0; i < X.size(); ++i)
			X[i][j] = (X[i][j] - min_val) / (max_val - min_val);
	}
}

void separateTestData(vector<Vector>& train_X, vector<double>& train_y, vector<Vector>& test_X, vector<double>& test_y)
{
	for (int i = train_X.size() - 1; i > 0; --i)
	{
		int rand_i = rand() % (i + 1);
		swap(train_X[i], train_X[rand_i]);
		swap(train_y[i], train_y[rand_i]);
	}

	int test_size = train_X.size() * .1;
	int train_size = train_X.size() - test_size;
	test_X.insert(test_X.begin(), train_X.begin() + train_size, train_X.end());
	test_y.insert(test_y.begin(), train_y.begin() + train_size, train_y.end());
	train_X.resize(train_size), train_y.resize(train_size);
}

#endif
