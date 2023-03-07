#include "include/Sketch.h"
#include "include/FAGMS.h"
#include "include/CM_Sketch.h"
#include "include/SkimSketch.h"
#include <bits/stdc++.h>

Sketch *Choose_Sketch(uint32_t w, uint32_t d, uint32_t hash_seed = 1000, int id = 0)
{
	switch (id)
	{
	case 0:
		return new CM_Sketch(w, d, hash_seed);
	case 1:
		return new C_Sketch(w, d, hash_seed);
	case 2:
		return new SkimSketch(w, d, hash_seed);
	}
	abort();
}

std::vector<std::string> items1, items0;
std::unordered_map<std::string, int> freq1, freq0;
long double Join_Ground_Truth;

int item_num = 0, flow_num = 0;
std::vector<double> MEM, ARE, AAE;

int max_freq = 0;

// read the caida & zipf dataset
// length -> the word's length
void readFile(const char *filename, int length, int MAX_ITEM = INT32_MAX)
{
	ifstream inFile(filename, ios::binary);

	if (!inFile.is_open())
		std::cout << "File fail." << endl;

	char key[length];
	for (int i = 0; i < MAX_ITEM; ++i)
	{
		inFile.read(key, length);
		if (inFile.gcount() < length)
			break;
		items1.push_back(std::string(key, KEY_LEN));
	}
	inFile.close();

	int data_delimiter = items1.size() / 2; // split the dataset into 2 pieces
	freq0.clear();
	freq1.clear();
	for (int i = 0; i < items1.size(); i++)
		if (i < data_delimiter)
			freq1[items1[i]]++;
		else
			freq0[items1[i]]++, items0.push_back(items1[i]);
	items1.resize(data_delimiter);

	for (auto pr : freq0) // calculate the max frequency
		max_freq = max(max_freq, pr.second);
	for (auto pr : freq1)
		max_freq = max(max_freq, pr.second);

	for (auto pr : freq0)
		if (freq1.count(pr.first))
			Join_Ground_Truth += 1ll * pr.second * freq1[pr.first]; // calculate the join ground truth

	// print the information of the dataset
	item_num = items1.size() + items0.size();
	flow_num = freq0.size() + freq1.size();
	std::cout << "dataset name: " << filename << endl;
	std::cout << flow_num << " flows, " << item_num << " items read" << endl;
	std::cout << "max freq = " << max_freq << endl;
	std::cout << "Join Ground Truth = " << Join_Ground_Truth << endl
			  << endl;
}

// d -> counts of hash function
// w -> memory size of the sketch,
void test_ske(int w, int d, int Choose)
{
	long double _ARE = 0, _AAE = 0;
	for (int i = 0; i < testcycles; ++i) // run many testcycles to reduce errors
	{
		// use the sketch type passed in the command line
		Sketch *ske0, *ske1;
		ske0 = Choose_Sketch(w, d, i * 233, Choose);
		ske1 = Choose_Sketch(w, d, i * 233, Choose);

		for (std::string str : items0)
			ske0->Insert(str.c_str());

		for (std::string str : items1)
			ske1->Insert(str.c_str());
		
		// join the sketch
		long double re = ske0->Join(ske1);

		// calculate some values need to print
		_AAE += abs(re - Join_Ground_Truth);
		_ARE += 1.0 * abs(re - Join_Ground_Truth) / Join_Ground_Truth;
	}
	_AAE /= testcycles;
	_ARE /= testcycles;
	MEM.push_back(w);
	AAE.push_back(_AAE);
	ARE.push_back(_ARE);
}

#include <boost/program_options.hpp>
using namespace boost::program_options;

// read the command line arguments
// filename -> the name of the dataset
// len -> the length of the key word
// sz -> the memory size of the sketch
// Choose -> the type of the sketch
void ParseArg(int argc, char *argv[], char *filename, int &len, int &sz, int &Choose)
{
	options_description opts("Join Options");

	opts.add_options()("memory,m", value<int>()->required(), "memory size")("version,v", value<int>()->required(), "version")("keylen,l", value<int>()->required(), "keylen")("filename,f", value<std::string>()->required(), "trace file")("help,h", "print help info");
	boost::program_options::variables_map vm;
	store(parse_command_line(argc, argv, opts), vm);

	if (vm.count("help"))
	{
		std::cout << opts << endl;
		exit(0);
	}
	if (vm.count("keylen"))
	{
		len = vm["keylen"].as<int>();
	}
	else
	{
		printf("please use -l to specify the keylen.\n");
		exit(0);
	}
	if (vm.count("version"))
	{
		Choose = vm["version"].as<int>();
	}
	else
	{
		printf("please use -v to specify the algorithm to use.\n");
		exit(0);
	}
	if (vm.count("memory"))
	{
		sz = vm["memory"].as<int>();
	}
	else
	{
		printf("please use -m to specify the memory size.\n");
		exit(0);
	}
	if (vm.count("filename"))
	{
		strcpy(filename, vm["filename"].as<std::string>().c_str());
	}
	else
	{
		printf("please use -f to specify the trace file.\n");
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	char filename[100];
	int sz, Choose, len;

	// read the arguments
	ParseArg(argc, argv, filename, len, sz, Choose);

	// read the dataset
	readFile(filename, len);

	// test the sketch and get all the values we need
	test_ske(sz, 3, Choose);

	// print the values
	std::ofstream oFile;
	char oFilename[50];

	// print the results into a .csv file
	sprintf(oFilename, "result%d.csv", Choose);
	puts(oFilename);

	oFile.open(oFilename, ios::app);
	if (!oFile)
		return 0;
	oFile << "Dataset:" << filename << endl;
	oFile << "Memory,"
		  << "AAE,"
		  << "ARE" << endl;
	for (size_t i = 0; i < ARE.size(); i++)
		oFile << fixed << setprecision(9) << MEM[i] << ',' << AAE[i] << ',' << ARE[i] << '\n';
	oFile.close();
	MEM.clear();
	AAE.clear();
	ARE.clear();
	return 0;
}
