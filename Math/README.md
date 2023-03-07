# Codes for Mathematical Analysis

This folder contains the codes for our matematical analysis. We conduct experiments to validate Theorem 4.1-4.1 in our paper both theoretically and empirically. The results show that our theoretical results are highly consistent with experimental results. 



## File structures

- `utils.h`: Some macro definitions of bitwise operation.

- `murmur3.h`: Murmur hash library, obtained [here](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp).

- `ShiftBF.h`: Codes of ShiftBfEncoder, which is used to optimize the 1-bit indicator.

- `omp.h`: OMP (Orthogonal Matching Persuits) recovery algorithm of compressive sensing

- `tower.h`: Codes of TreeEncoding, which is used to compress the small sketch.

- `compressivesensing.h`: Codes of SketchSensing, which is used to compress the large sketch.

- `CM_Sketch.h` & `Sketch.h`: Codes of the Count-Min (CM) sketch

- `params.h`: Some macro definitions of bitwise params.

- `main.cpp`: Codes to run the test cases.




## Dataset 

The dataset used in our mathematical section is a Zipf dataset with the skewness of 0, which is generated using Web Polygraph [[1]](#md-ref-1). You can download the dataset from the link below. 

- Zipf_0: [zipf_0.0.dat](https://drive.google.com/file/d/1zPn3XVHKF3O7qYpSXTyCG71yyc33J9LD/view?usp=sharing)


## How to run

You can use the following command to build the codes. 

```bash
$ make
```

You can use the following command to run our tests. 

```bash
$ ./main -f FILENAME -t {1-5} -m [memorysize] -l [keylen]
```

1. `-f`: Path of the dataset you want to run.

2. `-t`: An integer (1-5), specifying the test case you want to run. The corresponding relationship is as follows. 

   | 1 | 2 | 3 | 4 | 5 |
   | - | - | - | - | - |
   | CorrectRateCMSketch | CorrectRateTower | ErrorBoundCMSketch | ErrorBoundTower | ErrorBoundSketchsensing |

   - CorrectRateCMSketch : Theorem 4.1 on a non-compressed CMSketch

   - CorrectRateTower : Theorem 4.1 on a compressed HierarchicalTree

   - ErrorBoundCMSketch : Theorem 4.2 on a non-compressed CMSketch

   - ErrorBoundTower : Theorem 4.2 on a compressed HierarchicalTree

   - ErrorBoundSketchsensing : Theorem 4.3-4.4 on an approximately recovered CMSketch


3. `-m`: An integer, specifying the memory usage (bytes) of the Sketch. If you choose to run the test case of TreeEncoding, this parameter specifies the size of the compressed sketch. 


4. `-l`: An integer, specifying the length of each entry in the dataset. If you use the Zipf dataset provided by us, you should set it to 4.

For example, you can run the following command to calculate the correct probability of CM sketch on dataset `zipf_0.0.dat`, and set the memory usage to 65536 bytes.

```bash
./main -f ./datasets/zipf_0.0.dat -m 65536 -l 4 -t 1
```

### Output Format

Our program prints the information of the dataset, and the results of the test case, including theoretical and empirical correct rate or error bound (guaranteed probability). 



## Reference 

<span id="md-ref-1"></span>
[1] Alex Rousskov and Duane Wessels. High-performance benchmarking with web polygraph. Software: Practice and Experience, 34(2):187–211, 2004.