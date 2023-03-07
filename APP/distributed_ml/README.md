# TreeSensing in Distributed Machine Learning

## Overview 

We apply TreeSensing to a simulated distributed machine learning system and compare it with Cluster-Reduce (CR) [1] and SketchML [2] on two tasks: classification (using logistic regression) and regression (using linear regression). In our system, there are eight workers and one parameter server. We fix the available transmission bandwidth of the three algorithms to be about 16 of the exact gradients. As described in § 5.2, for TreeSensing and Cluster-Reduce, we build larger MinMax sketches to encode gradients, and use SketchSensing to compress it into small memory before transmission. 

## File strucutres

* `classification.cpp` and `regression.cpp`: Codes to run the classification task and the regression task, respectively. 

* `classification_params.h` and `regression_params.h`: Parameter settings in the classification task and the regression task, respectively. You can modify these files before compiling. 

* `sensing_params.h`: Parameter settings of our SketchSensing. You can modify it before compiling. 

* `kll`: Codes of the quantile sketch (KLL) in SketchML.

* `min_max_sketch.h`: Codes of the MinMax sketch in SketchML. 

* `omp.h`: Codes for decoding a fragment in our SketchSensing. 

* `cluster.h`: Codes of Cluster-Reduce [1]. 


## Dataset

The dataset used in DML experiments is the **Twin gas sensor arrays dataset** download from [UCI Machine Learning Repository](http://archive.ics.uci.edu/ml/datasets/Twin+gas+sensor+arrays),
which contains 640 instances and $10^5$ features. 


These data files are only used for testing the performance of TreeSensing and the related algorithms in this project. Commercial purposes are fully excluded. If you want to use this dataset, please conform to the citation request in [UCI Machine Learning Repository](http://archive.ics.uci.edu/ml/datasets/Twin+gas+sensor+arrays). 




## How to run 

First, download the dataset from [UCI Machine Learning Repository](http://archive.ics.uci.edu/ml/datasets/Twin+gas+sensor+arrays) and put them into the folder `Twin_gas`.

You can simply build all the programs for DML with the following commands:

```bash
make clean; make
```

You can modify the parameters in `classification_params.h` `regression_params.h`, and `sensing_params.h`. 

After that, run `classification.out` or `regression.out` to execute the experiments.


## References

[1] Yikai Zhao, Zheng Zhong, Yuanpeng Li, Yi Zhou, Yifan Zhu, Li Chen, YiWang, and Tong Yang. Cluster-reduce: Compressing sketches for distributed data streams. In Proceedings of the 27th ACM SIGKDD Conference on Knowledge Discovery \& Data Mining, pages 2316–2326, 2021.


[2] Jiawei Jiang, Fangcheng Fu, Tong Yang, and Bin Cui. Sketchml: Accelerating distributed machine learning with data sketches. In Proceedings of the 2018 International Conference on Management of Data (SIGMOD), pages 1269–1284, 2018. 


[3] Twin gas sensor arrays Data Set. https://archive.ics.uci.edu/ml/datasets/Twin+gas+sensor+arrays.