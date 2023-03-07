# TreeSensing in Distributed Measurement


This folder contains codes of TreeSensing and the realted algorithm in a simulated distributed measurement system. We apply TreeSensing and other sketch compression algorithms to a simulated distributed system with $n$ measurement nodes and one central analyzer. In each measurement period, each node builds a sketch with its local data, compresses the sketch, and sends the compressed sketch to the central analyzer. The central analyzer recovers all received sketches, and uses them to perform global analysis tasks. 

In this scenerio, we compare the accuracy of TreeSensing against other sketch compression algorithms: Cluster-Reduce [[1]](#md-ref-1), Hokusai [[2]](#md-ref-2), and Elastic [[3]](#md-ref-3). We evaluate the efficiency of privacy-preserving aggregation. We also compare the aggregation time between linear aggregation modes and non-linear aggregation modes of TreeSensing. 

## File Structures

* `tower_aggregate.h`: Codes for merging multiple HierarchicalTrees into one aggregated HierarchicalTree.
* `Makefile`: Commands to compile each test case.
* `test_accuracy/`, `test_aggregation/`, `test_decryption_aggregation/`, `test_time_encryption`: Each of these directory contains a test case that evaluates the accuracy/speed of TreeSensing. The results will be placed in `results/` directory. 

Note that this experiment also depends on the codes in `../../CPU` folder. 

## Dependencies

See the dependencies of [TreeSensing on CPU](../../CPU/README.md#Dependencies).


## Datasets 

In this experiment, we also use the 1-minute CAIDA dataset used in our [CPU experiments](../../CPU/README.md#Datasets). The dataset can be downloaded at [CAIDA_1min.dat](https://drive.google.com/file/d/1qhINjRQrtpeQ-4lH68Jz80v-kI7tT4FC/view?usp=sharing). 

We split the dataset into $n$ shards, and distribute each measurement node one data shard. 



## How to Run

First, download the datasets from the above links. Modify `loader` and `path` in `CPU/config.json` to specify which dataset to be used.

### Build

```bash
make all
```

### Run

After building, you can use the following commands to run our test cases. 

- Accuracy in distributed measurement
  
  ```bash
  cd test_accuracy
  python3 test_accuracy.py
  ```
  
- Efficiency of privacy-preserving
  
  ```bash
  cd test_time_encryption
  python3 test_time_encryption
  ```
  
- Comparison between linear and non-linear aggregation
  
  - Aggregation time
    
    ```bash
    test_aggregation/test_aggregation
    ```
    
  - Decryption+aggregation time
    
    ```bash
    test_decryption_aggregation/compress_and_save && python3 test_decryption_aggregation/read_and_encrypt.py  
    ```
    


### Output Format

By default, our program generates an output file containing the results of test case in `./results/`. 


## References

<span id="md-ref-1"></span>
[1] Yikai Zhao, Zheng Zhong, Yuanpeng Li, Yi Zhou, Yifan Zhu, Li Chen, YiWang, and Tong Yang. Cluster-reduce: Compressing sketches for distributed data streams. In Proceedings of the 27th ACM SIGKDD Conference on Knowledge Discovery & Data Mining, pages 2316–2326, 2021.

<span id="md-ref-2"></span>
[2] Sergiy Matusevych, Alex Smola, and Amr Ahmed. Hokusai-sketching streams in real time. arXiv preprint arXiv:1210.4891, 2012.

<span id="md-ref-3"></span>
[3] Tong Yang, Jie Jiang, Peng Liu, Qun Huang, Junzhi Gong, Yang Zhou, Rui Miao, Xiaoming Li, and Steve Uhlig. Elastic sketch: Adaptive and fast network-wide measurements. In Proceedings of the 2018 Conference of the ACM Special Interest Group on Data Communication (SIGCOMM), pages 561–575, 2018.