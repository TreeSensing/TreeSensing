#include <bits/stdc++.h>
#include "tower.h"
#include "compressivesensing.h"
#include "CM_Sketch.h"

using namespace std;

vector<string> items;
std::vector<std::string> items_unique;
unordered_map<string, int>freq0;
int item_num = 0, flow_num = 0;

// Read the dataset and print some information
void readFile_zipf(const char* filename,int length=4, int MAX_ITEM = INT32_MAX){
	ifstream inFile(filename, ios::binary);

	if (!inFile.is_open())
		cout << "File fail." << endl;

	int max_freq = 0;
	char key[length];
	for (int i = 0; i < MAX_ITEM; ++i)
	{
		inFile.read(key, length);
		if (inFile.gcount() < length)
			break;
		items.push_back(string(key, KEY_LEN));
	}
	freq0.clear();
	for (int i=0; i<items.size();i++) {
		freq0[items[i]]++;
		if (freq0[items[i]] == 1)
			items_unique.push_back(items[i]);
	}
	inFile.close();

	for (auto pr : freq0)
		max_freq = max(max_freq, pr.second);

	item_num = items.size();
	flow_num = freq0.size();
	cout << "dataset name: " << filename << endl;
	cout << flow_num << " flows, " << items.size() << " items read" << endl;;
	cout << "max freq = " << max_freq << endl;
}

#include <boost/program_options.hpp>
using namespace boost::program_options;

// Get the command line arguments
void ParseArg(int argc, char *argv[],char*filename,int&len,int&sz,int&CHOOSE,int&testid)
{
    options_description opts("Join Options");

    opts.add_options()
        ("memory,m", value<int>()->required(), "memory size")
        ("test,t", value<int>()->required(), "test")
        ("keylen,l", value<int>()->required(), "keylen")
        ("filename,f", value<string>()->required(), "trace file")
        ;
    boost::program_options::variables_map vm;
    store(parse_command_line(argc, argv, opts), vm);
	if (vm.count("keylen"))
    {
        len=vm["keylen"].as<int>();
    }
    else
    {
        printf("please use -l to specify the keylen.\n");
        exit(0);
    }
    if (vm.count("memory"))
    {
        sz=vm["memory"].as<int>();
    }
    else
    {
        printf("please use -m to specify the memory size.\n");
        exit(0);
    }
    if (vm.count("filename"))
    {
        strcpy(filename,vm["filename"].as<string>().c_str());
    }
    else
    {
        printf("please use -f to specify the trace file.\n");
        exit(0);
    }
	if (vm.count("test"))
    {
        testid=vm["test"].as<int>();
    }
    else
    {
        printf("please use -t to specify the algorithm you want to test.\n");
        exit(0);
    }
}

std::vector<Tower<int>* > tres;
std::vector<std::vector<std::vector<int> > > cres;

// Compress the Sketch *s and save the data into tres and cres
void Compress(CM_Sketch *s, int ratio, int zerothld) {
	tres.clear();
	cres.clear();

	for (int i = 0; i < s->d; i++) // generate the towers
		tres.push_back(new Tower<int>(s->counter[i], s->w, {{8, 1}}, false, false, zerothld, -1));

	for (int i = 0; i < s->d; i++) { // compressive sensing algorithm
		CompressiveSensing<int> cs(s->w, ratio, s->w / 50, zerothld, i * 123);
		cres.push_back(cs.compress(s->counter[i]));
	}
}

// Decompress the data saved in tres and cres
void Decompress(CM_Sketch *s, int ratio, int zerothld) {
	int *tmp1 = new int[s->w];
	int *tmp2 = new int[s->w];

	for (int i = 0; i < s->d; i++) {
		auto &t = tres[i];
		auto &c = cres[i];
		
		t->decompress(tmp1); // tower decompress
		
		CompressiveSensing<int> cs(s->w, ratio, s->w / 50, zerothld, i * 123);

		cs.decompress(tmp2, c); // compressive sensing algorithm decompress

		for (int j = 0; j < s->w; j++)
			if (tmp1[j])
				s->counter[i][j] = tmp1[j];
			else
				s->counter[i][j] = tmp2[j];
	}
}


// Theorem 4.1
// calculate the correct probability of CMSketch
void ProbabilityCMSketch(int sz) {
	int w = sz;

	CM_Sketch *cm1 = new CM_Sketch(w, 3, 123);

	for (int i = 0; i < items.size(); i++)
		cm1->Insert(items[i].c_str()); // insert the items into the CMSketch

	int V = items.size();

	int cnt = 0;		
	for (auto str : items_unique) {
		if (cm1->Query(str.c_str()) - freq0[str] == 0)
			++cnt; // calculate the count of the correct flows
	}

	double a = pow(1 - 1. / cm1->w, flow_num - 1);
	double b = 1 - pow(1 - a, cm1->d); // calculate the theoretical probability

	printf("empirical probability:%.9f,theoretical probability:%.9f.\n", (float)cnt / flow_num, b); // print
}

float theta[100][100];
float P[100][100];
float PP[100];

// Theorem 4.1
// calculate the correct probability of Tower Compressed CMSketch
void ProbabilityTower(int sz) {
	int w = sz * 4; // sz means the memory size of the Tower, so the memory size of CMSketch is bigger

	CM_Sketch *cm1 = new CM_Sketch(w, 3, 123);

	for (int i = 0; i < items.size(); i++)
		cm1->Insert(items[i].c_str()); // insert the items into the CMSketch

	Compress(cm1, 3, 4096); // compress the CMSketch to get the Tower

	int V = items.size();

	memset(theta, 0, sizeof(theta));
	for (auto str : items_unique) {
		auto v = cm1->gethash(str.c_str());
		for (int j = 0; j < cm1->d; j++)
			if (freq0[str] < 4096) // only compute the small value 
				++theta[tres[j]->querydepth(freq0[str])][j]; // calculate the formula mentioned in the article
	}

	for (int j = 0; j < cm1->d; j++)
		for (int i = 0; i < tres[j]->level_cnt; i++)
			theta[i][j] /= flow_num; // calculate the formula mentioned in the article

	for (int j = 0; j < cm1->d; j++) {
		double s = 0;
		for (int i = 0; i < tres[j]->level_cnt; i++) {
			for (int k = i; k < tres[j]->level_cnt; k++)
				s += theta[k][j];
			P[i][j] = pow(1 - 1. / tres[j]->tower_widths[i], flow_num * s - 1); // calculate the formula mentioned in the article
		}
	}

	for (int j = 0; j < cm1->d; j++) {
		PP[j] = 0;
		double prod = 1;
		for (int i = 0; i < tres[j]->level_cnt; i++) {
			prod *= P[i][j];
			PP[j] += theta[i][j] * prod; // calculate the formula mentioned in the article
		}
	}

	double C = 1;
	for (int j = 0; j < cm1->d; j++)
		C *= 1 - PP[j];
	C = 1 - C; // calculate the theoretical probability
	
	int cnt = 0;
	for (auto str : items_unique) {
		int k = INT_MAX;
		auto v = cm1->gethash(str.c_str());
		for (int j = 0; j < cm1->d; j++)
			k = std::min(k, tres[j]->query(v[j]));
		if (k == freq0[str])
			++cnt; // calculate the count of the correct flows
	}

	printf("empirical probability:%.9f,theoretical probability:%.9f.\n", (float)cnt / flow_num, C); // print
}

// Theorem 4.2
// calculate the error bound probability of CMSketch
void ErrorBoundCMSketch(int sz) {
	int w = sz;

	CM_Sketch *cm1 = new CM_Sketch(w, 3, 123);

	for (int i = 0; i < items.size(); i++)
		cm1->Insert(items[i].c_str()); // insert the items into the CMSketch

	int V = items.size(); // sum of the flow frequency

	for (int xt = 1; xt <= 200; xt++) {

		int cnt = 0;		
		for (auto str : items_unique) {
			if (cm1->Query(str.c_str()) - freq0[str] >= xt) // calculate the count of the flows whose result is out of the error bound
				++cnt;
		}

		// calculate the theoretical probability and print
		printf("error bound:%d,empirical probability:%.9f,theoretical probability:%.9f.\n", xt, 1. - (float)cnt / flow_num, std::max(0., 1. - pow((float)V / xt / cm1->w, cm1->d)));
	}
}

// Theorem 4.2
// calculate the error bound probability of Tower Compressed CMSketch
void ErrorBoundTower(int sz) {
	int w = sz * 4; // sz means the memory size of the Tower, so the memory size of CMSketch is bigger

	CM_Sketch *cm1 = new CM_Sketch(w, 3, 123);

	for (int i = 0; i < items.size(); i++)
		cm1->Insert(items[i].c_str()); // insert the items into the CMSketch

	Compress(cm1, 3, 4096); // compress the CMSketch to get the Tower

	int V = items.size(); // sum of the flow frequency

	memset(theta, 0, sizeof(theta));
	for (auto str : items_unique) {
		auto v = cm1->gethash(str.c_str());
		for (int j = 0; j < cm1->d; j++)
			if (freq0[str] < 4096)
				++theta[tres[j]->querydepth(freq0[str])][j]; // calculate the formula mentioned in the article
	}

	for (int j = 0; j < cm1->d; j++)
		for (int i = 0; i < tres[j]->level_cnt; i++)
			theta[i][j] /= flow_num; // calculate the formula mentioned in the article

	for (int xt = 1; xt <= 200; xt += 1) {
		float eps = 1. * xt / V;

		int cnt = 0;
		for (auto str : items_unique) {
			int k = INT_MAX;
			auto v = cm1->gethash(str.c_str());
			for (int j = 0; j < cm1->d; j++)
				k = std::min(k, tres[j]->query(v[j]));
			if (std::abs(k - freq0[str]) >= xt) // calculate the count of the flows whose result is out of the error bound
				++cnt;
		}

		float Pr = 1;

		for (int j = 0; j < cm1->d; j++) {
			float tmp = 0;
			for (int i = 0; i < tres[j]->level_cnt; i++)
				tmp += 1 / eps * theta[i][j] / tres[j]->tower_widths[i]; // calculate the formula mentioned in the article
			Pr *= tmp;
		}

		// print
		printf("error bound:%d,empirical probability:%.9f,theoretical probability:%.9f.\n", xt, 1. - (float)cnt / flow_num, std::max(0., 1. - Pr));
	}
}

// Theorem 4.3
// calculate the probability a result is valid
// Theorem 4.4
// calculate the error bound of the CMSketch when some counters is lost
void ErrorBoundSketchsensing(int sz) {
	int w = sz;

	CM_Sketch *cm1 = new CM_Sketch(w, 3, 123);

	for (int i = 0; i < items.size(); i++)
		cm1->Insert(items[i].c_str()); // insert the items into the CMSketch

	std::vector<int> p[cm1->d];
	for (int j = 0; j < cm1->d; j++) { // randomly set a sequence to remove the counters
		p[j].resize(cm1->w);
		for (int i = 0; i < cm1->w; i++)
			p[j][i] = i;
		std::random_device rd;
		std::mt19937 rng(rd());
		std::shuffle(p[j].begin(), p[j].end(), rng);
	}

	int V = items.size();

	for (int mu = 0, c = 0; mu < 100; mu += 20) { // How many percent of the CMSketch is lost, use a int instead of float
		std::cerr << mu << '\n';
		while (c * 100 / cm1->w < mu) { // remove the counters
			for (int j = 0; j < cm1->d; j++)
				cm1->counter[j][p[j][c]] = INT_MAX;
			++c;
		}

		int cnt = 0;
		for (auto str : items_unique) {
			int k = cm1->Query(str.c_str());
			if (k < INT_MAX)
				++cnt; // calculate the valid flow count
		}

		float fmu = (float)mu / 100;

		// Theorem 4.3
		// print
		printf("mu:%.2f,empirical probability:%.9f,theoretical probability:%.9f\n", 1. - fmu, (float)cnt / flow_num, 1. - pow(1. - fmu, cm1->d));
		
		for (int xt = 1; xt <= 400; xt++) { // error bound

			int cnt1 = 0, cnt2 = 0;

			for (auto str : items_unique) {
				int k = cm1->Query(str.c_str());
				if (k != INT_MAX) {
					++cnt1; // calculate the valid flow count
					if (k - freq0[str] >= xt)
						++cnt2; // calculate the count of the flows whose result is out of the error bound
				}
			}

			float tmp = pow(fmu + V / cm1->w * (1. - fmu) / xt, cm1->d) - pow(fmu, cm1->d); // calculate the formula mentioned in the article

			tmp /= 1 - pow(fmu, cm1->d); // calculate the formula mentioned in the article

			// Theorem 4.4
			// print
			printf("mu:%.2f,error bound:%d,empirical probability:%.9f,theoretical probability:%.9f\n", 1. - fmu, xt, 1. - (float)cnt2 / cnt1, std::max(0., 1. - tmp));
		}
	}
}

int main(int argc,char *argv[]){
	char filename[100];
	int sz,CHOOSE,len,testid;

	// get the command line arugments
    ParseArg(argc, argv, filename, len, sz, CHOOSE, testid);
	
	// read the dataset
	readFile_zipf(filename, len);

	// run the test
	switch (testid)	{
	case 1:
		ProbabilityCMSketch(sz);
		break;
	case 2:
		ProbabilityTower(sz);
		break;
	case 3:
		ErrorBoundCMSketch(sz);
		break;
	case 4:
		ErrorBoundTower(sz);
		break;
	case 5:
		ErrorBoundSketchsensing(sz);
		break;
	default:
		break;
	}
	return 0;
}
