#ifndef __SENSING_PARAMS_H__
#define __SENSING_PARAMS_H__

// parameters recommended for classfication task

//int sensing_ratio = 3; //2
//int nfrags = 256 * sensing_ratio * 4; //*2
//float sparse_level = 0.975; //0.93
//int zero_thld = 950; //910


// parameters recommended for regression task

int sensing_ratio = 2;
int nfrags = 256 * sensing_ratio * 4;
float sparse_level = 0.91; //regression:0.935
int zero_thld = 910; //regression:920


#endif
