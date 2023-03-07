# Codes for Join-aggregate Estimation

This folder contains the codes for the task of join-aggregate estimation. We apply our TreeSensing to the task of join-aggregate estimation, and compare it with Skimmed sketch [[1]](#md-ref-1) and FAGMS [[2]](#md-ref-2). 

## File Structures

- `src/include/Choose_Ske.h`: An interface function to choose a sketch type.

- `src/include/CM_Sketch.h`: Codes of the CM sketch.

- `src/include/FAGMS.h`: Codes of the Count sketch.

- `src/include/SkimSketch.h`: Codes of the Skimmed sketch.

- `src/include/MurmurHash.h`: Murmur hash library, obtained [here](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp).

- `src/include/params.h`: Some macro definitions of params used in our experiments.

- `src/include/Sketch.h`: Codes of the sketch basic class.

- `src/main.cpp`: Codes to run our tests.


## Datasets

The datasets used in tasks of join-aggregate estimation are CAIDA dataset and Zipf dataset with the skewness of 0, which is generated using Web Polygraph [[3]](#md-ref-3). We provide two small shards of CAIDA dataset in `datasets/CAIDA/`. You can directly run our test cases using them. 

You can download the Zipf dataset from the link below. 

- Zipf_0: [zipf_0.0.dat](https://drive.google.com/file/d/1zPn3XVHKF3O7qYpSXTyCG71yyc33J9LD/view?usp=sharing)

**Notification:** These data files are only used for testing the performance of TreeSensing and the related algorithms in this project. Commercial purposes are fully excluded. Please do not use these datasets for other purpose. 


## How to run

You can use the following command to build the codes. 

```bash
$ make
```

You can use the following command to run our tests. 

```bash
$ ./main -f FILENAME -v {0-2} -m [memorysize] -l [keylen]
```

1. `[-f filename]`: The path of the dataset you want to run. 

2. `[-v version]`: An integer, specifying the algorithm you want to run. The corresponding relationship is as follows. 


   | 0 | 1 | 2 |
   | - | - | - |
   | Count-Min Sketch | Count Sketch | Skimmed Sketch |

3. `[-l keylen]`: An integer, specifying the length of each entry in the dataset. In our test, if you use the CAIDA dataset provided by us, you should set it to 13 (4 srcIP, 4 dstIP, 2 srcPort, 2 dstPort, 1 protocolType). If you use the Zipf dataset provided by us, you should set it to 4.

4. `[-m memory]`: An integer, specifying the memory size (in bytes) used by the Sketch. 

*Notice*: When using the Skimmed sketch, you can modify the `Heavy_Thes` variable in `src/include/SkimSketch.h` to adapt to different datasets. This variable is the separating threshold of the Skimmed sketch algorithm. 

### Output Format

Our program will first print the basic information of the dataset, and then generate a `.csv` file containing the running results.


## References

<span id="md-ref-1"></span>
[1] Sumit Ganguly, Minos Garofalakis, and Rajeev Rastogi. Processing data-stream join aggregates using skimmed sketches. In International Conference on Extending Database Technology, pages 569–586. Springer, 2004. 

<span id="md-ref-2"></span>
[2] Graham Cormode and Minos Garofalakis. Sketching streams through the net: Distributed approximate query tracking. In Proceedings of the 31st international conference on Very large data bases, pages 13–24, 2005.

<span id="md-ref-3"></span>
[3] Alex Rousskov and Duane Wessels. High-performance benchmarking with web polygraph. Software: Practice and Experience, 34(2):187–211, 2004.