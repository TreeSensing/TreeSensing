# TreeSensing: Linearly Compressing Sketches with Flexibility

This repository contains all related code of our paper "TreeSensing: Linearly Compressing Sketches with Flexibility". 

## Introduction

A Sketch is an excellent probabilistic data structure, which records the approximate statistics of data streams by maintaining a summary. Linear additivity is an important property of sketches. This paper studies how to keep the linear property after sketch compression. Most existing compression methods do not keep the linear property.We propose TreeSensing, an accurate, efficient, and flexible framework to linearly compress sketches. In TreeSensing, we first separate a sketch into two partial sketches according to counter values. For the sketch with small counters,we propose a key technique called TreeEncoding to compress it into a hierarchical structure. For the sketch with large counters, we propose a key technique called SketchSensing to compress it using compressive sensing. We theoretically analyze the accuracy of TreeSensing. We use TreeSensing to compress 7 sketches and conduct two end-to-end experiments: distributed measurement and distributed machine learning. Experimental results show that TreeSensing outperforms prior art on both accuracy and efficiency, which achieves up to 100× smaller error and 5.1× higher speed than state-of-the-art Cluster-Reduce. All related codes are open-sourced anonymously. 

## About this repository

* `CPU` contains codes of TreeSensing and the related algorithms implemented on CPU platforms. 

* `APP` contains codes of TreeSensing and the related algorithms in our application experiments. 

    * `APP/distributed_measurement` contains codes of TreeSensing and the realted algorithm in a simulated distributed measurement system. 

    * `APP/distributed_ml` contains codes of TreeSensing and the related algorithm in a simulated distributed machine learning (DML) system. 

    * `APP/join_aggregate` contains codes of TreeSensing and the related algorithm in the task of join-aggregate estimation.

* `Math` contains codes related to our mathematical analysis. 

* `Redis` contains codes of TreeSensing implemented on top of [Redis](https://redis.io/). 
 
* `FPGA` contains codes of TreeSensing implemented on FPGA platforms.

* More details can be found in the folders.