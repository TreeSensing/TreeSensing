#include <random>
#include <map>
#include "read_dataset.h"
#include "cluster.h"
#include "classification_params.h"
#include "utils.h"

vector<LogisticCluster> clusters;
vector<Vector> train_X, test_X;
vector<double> train_y, test_y;
vector<Vector> random_W;

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
    random_W.resize(rep_num);
    for (int rep_cnt = 0; rep_cnt < rep_num; ++rep_cnt)
    {
        random_W[rep_cnt].resize(train_X[0].size());
        for (int i = 0; i < random_W.size(); ++i)
            random_W[rep_cnt][i] = distribution(generator);
    }
}

void initW(int rep_cnt)
{
    for (int i = 0; i < clusters.size(); ++i)
        clusters[i].W = random_W[rep_cnt];
}

/*
 * mode 2: compressive_sensing
 * mode 1: compress
 * mode 0: non-compress
 * mode -1: do not use sketchML
 */
void train(vector<LogisticCluster>& clusters, double bandwidth, int mode, double train_rate)
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

void classify(int mode)
{
    map<int, double> test_acc, test_loss, train_acc, train_loss;
    print_ARE = false;

    cout<<"_______compress begin_______"<<endl;
    for (int rep_cnt = 0; rep_cnt < rep_num; ++rep_cnt)
    {
        cerr << "Rep:" << rep_cnt << endl;
        initW(rep_cnt);
        // cout << "hi" << endl;
        double train_rate = init_train_rate;

        for (int epoch = 1; epoch <= epoch_num; ++epoch)
        {
            cerr << "Epoch:" << epoch << "/" << epoch_num << endl;
            // cout << "\n";
            //cout << setiosflags(ios::fixed) << setprecision(2);
            //refresh((float)epoch / epoch_num);
            train(clusters, bandwidth, mode, train_rate);

            if (epoch % tr_step == 0)
                train_rate *= 0.9;
            if (epoch == 1 || epoch % print_step == 0)
            {
                auto test_res = clusters[0].test(test_X, test_y);
                test_acc[epoch] += test_res.first;
                test_loss[epoch] += test_res.second;
                auto train_res = clusters[0].test(train_X, train_y);
                train_acc[epoch] += train_res.first;
                train_loss[epoch] += train_res.second;
            }

        }

        //cout << "hi" << endl;
        //    cout<<rep_cnt<<"/"<<rep_num<<endl;
    } 
    cerr<<"______compress end__________"<<endl;

    cout << "epoch\ttest acc\ttest loss\ttrain acc\ttrain loss" << endl;
    for (int epoch = 1; epoch <= epoch_num; ++epoch)
        if (epoch == 1 || epoch % print_step == 0)
            cout << epoch << "\t"
                << test_acc[epoch] / rep_num << "\t"
                << test_loss[epoch] / rep_num << "\t"
                << train_acc[epoch] / rep_num << "\t"
                << train_loss[epoch] / rep_num << endl;
}

int main()
{
    readClassification(train_X, train_y, "./Twin_gas", attribute_num);
    normalization(train_X);
    separateTestData(train_X, train_y, test_X, test_y);
    initClusters(cluster_num);


    cout << "classification with compressive sensing:" << endl;
    classify(2);

    cout << "classification with compress:" << endl;
    classify(1);

    cout << "classification without compress:" << endl;
    classify(0);

    cout << "classification without sketchML:" << endl;
    classify(-1);

    return 0;
}
