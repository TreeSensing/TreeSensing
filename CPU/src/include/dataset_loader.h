#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstring>
#include <string>
using namespace std;

#include "json.hh"
using json = nlohmann::json;
extern json global_cfg;

// An in-memory cache of dataset (packets) and ground truth (sorted flows). So
// there is no need to read the same file from disk multiple times.
struct {
    vector<pair<string, float>> dataset;
    vector<pair<int, string>> groundtruth;
} cache;

vector<pair<string, float>> loadCAIDA18(const char* filename, int read_num) {
    FILE* pf = fopen(filename, "rb");
    if (!pf) {
        fprintf(stderr, "Cannot open %s\n", filename);
        exit(-1);
    }

    vector<pair<string, float>> vec;
    double ftime = -1;
    char trace[30];
    int i = 0;
    while (fread(trace, 1, 21, pf)) {
        if (i++ == read_num) break;

        string tkey(trace, 4);  // SrcIP
        double ttime = *(double*)(trace + 13);
        if (ftime < 0) ftime = ttime;
        vec.push_back(pair<string, float>(tkey, ttime - ftime));
    }
    fclose(pf);
    return vec;
}

vector<pair<string, float>> loadCAIDA18_multi(const char* filepath,
                                              int read_num) {
    vector<pair<string, float>> vec;

    for (int j = 130000; j <= 135900; j += 100) {
        string filename = string(filepath) + "/" + to_string(j) + ".dat";
        FILE* pf = fopen(filename.c_str(), "rb");
        if (!pf) {
            fprintf(stderr, "Cannot open %s\n", filename.c_str());
            continue;
        }

        double ftime = -1;
        char trace[30];
        int i = 0;
        while (fread(trace, 1, 21, pf)) {
            if (i++ == read_num) break;

            string tkey(trace, 4);  // SrcIP
            double ttime = *(double*)(trace + 13);
            if (ftime < 0) ftime = ttime;
            vec.push_back(pair<string, float>(tkey, ttime - ftime));
        }
        fclose(pf);
    }
    return vec;
}

vector<pair<string, float>> loadCriteo(const char* filename, int read_num) {
    vector<pair<string, float>> vec;

    ifstream logFile(filename);
    if (logFile.fail()) {
        fprintf(stderr, "Cannot open %s\n", filename);
        exit(-1);
    }
    string str;

    while (getline(logFile, str)) {
        vec.push_back(pair<string, float>(str.substr(10), 1));
        str.clear();
    }

    logFile.close();
    return vec;
}

vector<pair<string, float>> loadZipf(const char* filename, int read_num) {
    int MAX_ITEM = INT32_MAX;
    ifstream inFile(filename, ios::binary);
    if (inFile.fail()) {
        fprintf(stderr, "Cannot open %s\n", filename);
        exit(-1);
    }
    ios::sync_with_stdio(false);

    char key[13];
    vector<pair<string, float>> vec;
    for (int i = 0; i < MAX_ITEM; ++i) {
        inFile.read(key, 4);
        if (inFile.gcount() < 4) break;

        string str = string(key, 4);
        vec.push_back(pair<string, float>(string(key, 4), 1));
    }
    inFile.close();
    return vec;
}

// Count the true size of flows from dataset and sort them in descending order.
// Return a vector of (flow count, key) pairs. If "to_file" specifies a file
// name, results will be output to that file, which can be later restored by
// groundtruth_from_file().
vector<pair<int, string>> groundtruth(
    const vector<pair<string, float>>& dataset, const char* to_file = nullptr) {
    if (!cache.groundtruth.empty()) return cache.groundtruth;

    unordered_map<string, int> key2cnt;
    for (auto pr : dataset) {
        key2cnt[pr.first]++;
    }

    vector<pair<int, string>> ans;  // (count, key) in descending order
    ans.reserve(dataset.size());
    for (auto pr : key2cnt) {
        ans.push_back(make_pair(pr.second, pr.first));
    };
    sort(ans.begin(), ans.end(), greater<pair<int, string>>());
    printf("Dataset: %lu packets, %lu flows\n", dataset.size(), ans.size());

    if (to_file) {
        FILE* fp = fopen(to_file, "wb");
        for (pair<int, string> pr : ans) {
            string key = pr.second;
            int flow_cnt = pr.first;
            fwrite(key.data(), 1, key.size(), fp);
            fwrite(&flow_cnt, sizeof(flow_cnt), 1, fp);
        }
        fclose(fp);
    }

    cache.groundtruth = ans;
    return ans;
}

// Read true flow size data from the file generated by groundtruth() with
// "to_file" specified.
vector<pair<int, string>> groundtruth_from_file(const char* from_file) {
    if (!cache.groundtruth.empty()) {
        return cache.groundtruth;
    }
    vector<pair<int, string>> ans;

    FILE* fp = fopen(from_file, "rb");
    char ckey[4];
    int flow_cnt;
    while (fread(&ckey, 1, 4, fp)) {
        if (!fread(&flow_cnt, sizeof(flow_cnt), 1, fp)) break;
        string key(ckey, 4);
        ans.push_back(make_pair(-flow_cnt, key));
    }
    fclose(fp);

    cache.groundtruth = ans;
    return ans;
}

// Call the specified loader function and read packets from the specified
// dataset according to the configuration in "global_cfg". Return a vector of
// (key, timestamp) pairs.
vector<pair<string, float>> load_dataset(int read_num = -1) {
    if (!cache.dataset.empty()) return cache.dataset;

    using loader_func_t = vector<pair<string, float>> (*)(const char*, int);
    static map<string, loader_func_t> loader2func = {
        {"loadCAIDA18", loadCAIDA18},
        {"loadCriteo", loadCriteo},
        {"loadZipf", loadZipf}};

    string loader = global_cfg["dataset"]["loader"];
    string path = global_cfg["dataset"]["path"];

    if (loader2func.find(loader) == loader2func.end()) {
        fprintf(stderr, "unknown loader %s\n", loader.c_str());
        exit(-1);
    }
    loader_func_t loader_func = loader2func[loader];
    auto dataset = loader2func[loader](path.c_str(), read_num);

    cache.dataset = dataset;
    return dataset;
}
