#include <random>
#include "read_dataset.h"
#include "cluster.h"
#include "regression_params.h"

vector<LinearCluster> clusters;
vector<Vector> train_X, test_X;
vector<double> train_y, test_y;
Vector random_W;

void initClusters(int cluster_num)
{
    clusters.resize(cluster_num);
    for (int i = 0; i < train_X.size(); ++i)
    {
        clusters[i % cluster_num].X.push_back(train_X[i]);
        clusters[i % cluster_num].y.push_back(train_y[i]);
    }
    default_random_engine generator;
    uniform_real_distribution<double> distribution(-1, 1);
    random_W.resize(train_X[0].size());
    for (int i = 0; i < random_W.size(); ++i)
        random_W[i] = distribution(generator);
}

void initW()
{
    for (int i = 0; i < clusters.size(); ++i)
        clusters[i].W = random_W;
}

/*
 * mode 1: compress
 * mode 0: non-compress
 * mode -1: do not use sketchML
 */
void train(vector<LinearCluster>& clusters, double bandwidth, int mode, double train_rate)
{
    Vector grad;
    for (int i = 0; i < clusters.size(); ++i)
    {
        auto cluster_grad = clusters[i].calcGrad();
        if (mode >= 0)
            cluster_grad = encodedVector(cluster_grad, bandwidth, mode).decode();
        if (i == 0) grad = cluster_grad;
        else grad = add(grad, cluster_grad);
    }
    grad = mul(train_rate / clusters.size(), grad);
    for (int i = 0; i < clusters.size(); ++i)
        clusters[i].updateW(grad);
}

void regress(int mode)
{
    initW();
    double train_rate = init_train_rate;
    print_ARE = false;

    cout << "epoch\ttest loss\ttrain loss" << endl;
    for (int epoch = 1; epoch <= epoch_num; ++epoch)
    {
        train(clusters, bandwidth, mode, train_rate);
        if (epoch % tr_step == 0)
            train_rate *= 0.9;
        if (epoch == 1 || epoch % print_step == 0)
        {
            cout << epoch << "\t"
                << clusters[0].test(test_X, test_y) << "\t"
                << clusters[0].test(train_X, train_y) << endl;
        }
    }
}

int main()
{
    readRegression(train_X, train_y, "./Twin_gas", attribute_num);
    normalization(train_X);
    separateTestData(train_X, train_y, test_X, test_y);
    initClusters(cluster_num);

    cout << "regression with compressive sensing" << endl;
    regress(2);

    cout << "regression with compress:" << endl;
    regress(1);

    cout << "regression without compress:" << endl;
    regress(0);

    cout << "regression without sketchML:" << endl;
    regress(-1);

    return 0;
}
