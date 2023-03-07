# TreeSensing on CPU

## Overview

We implement and test TreeSensing (including *TreeEncoding*, *SketchSensing* and *ShiftBfEncoder*) on a CPU platform (Intel i9-10980XE, 18-core 4.2 GHz CPU with 128GB 3200 MHz DDR4 memory and 24.75 MB L3 cache). We implement three existing sketch compression algorithms: Cluster-Reduce [[1]](#md-ref-1), Hokusai [[2]](#md-ref-2), and Elastic [[3]](#md-ref-3). We apply TreeSensing and existing algorithms on six sketches: CM [[4]](#md-ref-4), CU [[5]](#md-ref-5), Count [[6]](#md-ref-6), CMM [[7]](#md-ref-7), CML [[8]](#md-ref-8), and CSM [[9]](#md-ref-9).

Most of the key codes (sketch data structures, insertion and query procedures, procedures of *TreeEncoding*, *SketchSensing* and *ShiftEncoder*) are implemented with C++. We wrap the main C++ codes using Python for ease of testing. Experimental setup and test cases are written in Python for convenience.

## Dependencies

* g++ 7.5.0 (Ubuntu 7.5.0-6ubuntu2)

* [Murmur Hash](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp)

* Python 3.8.10
  * NumPy 1.22.0
  * Pandas 1.3.5
  * tenseal 0.3.11

* [Optional] AVX-256 [Intel Intrinsics](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html) (SIMD operations)

## File structures

* `include/`: Common header files.
  * `dataset_loader.h`: Dataset loader.
  * `utils.h`: Some important utility functions and macros for TreeSensing.
* `sketches/`: Data structures and algorithms of sketches.
  * `common.h`: Data structures and algorithms shared by different kinds of sketches. `CounterArray` is a template base class that defines a single array with inserting and querying methods. `CounterMatrix` is a template base class containing a pointer to an array of several `CounterArray`s, and the inserting, querying and compressing methods for them. `CounterMatrix` is a complete sketch.
  * `cm.h`, `count.h`, `cu.h`, `cmm.h`, `cml.h`, `csm.h`: Codes for CM Sketch, Count Sketch, CU Sketch, CMM Sketch, CML Sketch and CSM sketch, including the methods of sketch insertion and querying. Classes in these source files inherit from template specializations of `CounterArray` and `CounterMatrix`.
* `tower_sensing/`: Codes for TreeEncoding, SketchSensing and ShiftBfEncoder.
  * `shiftbf.h`: Codes for ShiftBfEncoder.
  * `tower_encoding.h`: Codes for TreeEncoding and its recovery process.
  * `sketch_sensing.h`: Codes for SketchSensing and its recovery process.
* `prior_art/`: Implementation of other sketch compression algorithms, including Cluster-Reduce [[1]](#md-ref-1), Hokusai [[2]](#md-ref-2), and Elastic [[3]](#md-ref-3).
* `sketch_api.cpp`: Codes for generating shared libraries for different sketches (see `Makefile`). It exposes C interfaces for Python [ctypes](https://docs.python.org/3/library/ctypes.html).
* `Makefile`: Commands to compile shared libraries for different sketches.
* `sketch_compress.py`: Encapsulates calls to functions in the shared libraries. It also includes some commonly-used procedures for test cases to call.
* `tests.py`: Test cases.
* `extra_tests/`:
  * `test_sensing_flex.cpp`: A test case to test the flexibility of SketchSensing.
  * `test_tower_shape.cpp`: A test case for impact of HierarchicalTree's shape.
  * `test_tower_error.cpp`: A test case to test compression error of TreeEncoding.

## Datasets

Datasets used in CPU experiments can be downloaded from the links below. After downloading, modify the corresponding configuration in `config.json`.

* CAIDA18: [CAIDA_1min.dat](https://drive.google.com/file/d/1qhINjRQrtpeQ-4lH68Jz80v-kI7tT4FC/view?usp=sharing)
* Zipf: [003.dat](https://drive.google.com/file/d/1BWVzXKc11mwvSSrHuYurXtsgPGbJBe_A/view?usp=sharing)


**Notification:** These data files are only used for testing the performance of TreeSensing and the related algorithms in this project. Commercial purposes are fully excluded. Please do not use these datasets for other purpose. 


## How to run

### Build

First, download the datasets from the above links. Modify `loader` and `path` in `config.json` to specify which dataset to be used. Supported loaders are specified in `include/dataset_loader.h`.

```jsonc
// config.json
{
    "dataset": {
        "loader": "loadCAIDA18",
        "path": "./datasets/CAIDA_1min.dat"
    }
}
```

Then, build the shared libraries with the following command. 

```bash
make all [USER_DEFINES=-DSIMD_COMPRESS]
```

Shared libraries (.so files) are generated in `build/`. Use `-j` to build in parallel. You can decide whether to use SIMD by whether or not defining the `SIMD_COMPRESS` macro when calling the make command.


### Test cases

Below we show some examples of running tests on CM Sketches. 

Each test case is defined with a function starting with "test_" in `tests.py`. The directory where test results are stored is specified by "test_result_dir" in `config.json`.

The generic command to run a test is shown below:

```bash
python3 tests.py --test {test name} --sketch {sketch type} --k {top-k} --d {number of arrays} --w {number of counters in each array} --sep_thld {separating threshold} --round_thld {rounding parameter}
```

You can run the follwoing command to see the specific definition of each parameter: 

```bash
python3 tests.py -h
```

Note that not all parameters are valid in every test. For details, please refer to each test case's definition.


* Top-k Accuracy vs. Compression Ratio

  ```bash
  python3 tests.py --test test_acc_mem --sketch CM --k 500
  ```

* Full Accuracy vs. Tree Shape

  ```bash
  python3 tests.py --test test_fullacc_towershape --sketch CM
  ```

* Top-k and Full Accuracy vs. Separating Threshold

  ```bash
  python3 tests.py --test test_acc_separating_k --sketch CM
  ```

* Top-k Accuracy vs. Rounding Parameter

  ```bash
  python3 tests.py --test test_acc_rounding --sketch CM
  ```

* Flexibility of TreeEncoding

  ```bash
  python3 tests.py --test test_acc_ignore_level --sketch CM
  ```

* Efficiency of TreeEncoding

  ```bash
  python3 tests.py --test test_time_towerencoding --sketch CM
  ```

* Efficiency of SketchSensing

  ```bash
  python3 tests.py --test test_time_sketchsensing --sketch CM
  ```

* Top-k Accuracy (Comparison with Prior Art)

  ```bash
  python3 tests.py --test test_acc_algos --sketch CM
  ```

* Top-k Compression Efficiency (Comparison with Prior Art)

  ```bash
  python3 tests.py --test test_time_algos --sketch CM
  ```

There are some extra tests besides `tests.py`.

* Approximate recovery

  ```bash
  make build/test_sensing_flex
  ./build/test_sensing_flex
  ```

* Impact of HierarchicalTree shape

  ```bash
  make build/test_tower_shape
  ./build/test_tower_shape
  ```

* Compression error of TreeEncoding

  ```bash
  make build/test_tower_error
  ./build/test_tower_error
  ```

## References

<span id="md-ref-1"></span>
[1] Yikai Zhao, Zheng Zhong, Yuanpeng Li, Yi Zhou, Yifan Zhu, Li Chen, YiWang, and Tong Yang. Cluster-reduce: Compressing sketches for distributed data streams. In Proceedings of the 27th ACM SIGKDD Conference on Knowledge Discovery & Data Mining, pages 2316–2326, 2021.

<span id="md-ref-2"></span>
[2] Sergiy Matusevych, Alex Smola, and Amr Ahmed. Hokusai-sketching streams in real time. arXiv preprint arXiv:1210.4891, 2012.

<span id="md-ref-3"></span>
[3] Tong Yang, Jie Jiang, Peng Liu, Qun Huang, Junzhi Gong, Yang Zhou, Rui Miao, Xiaoming Li, and Steve Uhlig. Elastic sketch: Adaptive and fast network-wide measurements. In Proceedings of the 2018 Conference of the ACM Special Interest Group on Data Communication (SIGCOMM), pages 561–575, 2018.

<span id="md-ref-4"></span>
[4] Graham Cormode and S Muthukrishnan. An improved data stream summary: the count-min sketch and its applications. Journal of Algorithms, 2005.

<span id="md-ref-5"></span>
[5] Cristian Estan and George Varghese. New directions in traffic measurement and accounting. ACM SIGMCOMM CCR, 2002.

<span id="md-ref-6"></span>
[6] Moses Charikar, Kevin Chen, and Martin Farach-Colton. Finding frequent items in data streams. In Automata, Languages and Programming. 2002.

<span id="md-ref-7"></span>
[7] Fan Deng and Davood Rafiei. New estimation algorithms for streaming data: Count-min can do more. Webdocs. Cs. Ualberta. Ca, 2007.

<span id="md-ref-8"></span>
[8] Guillaume Pitel and Geoffroy Fouquier. Count-min-log sketch: Approximately counting with approximate counters. arXiv preprint arXiv:1502.04885, 2015.

<span id="md-ref-9"></span>
[9] Tao Li, Shigang Chen, and Yibei Ling. Per-flow traffic measurement through randomized counter sharing. IEEE/ACM Transactions on Networking, 20(5):1622–1634, 2012.

