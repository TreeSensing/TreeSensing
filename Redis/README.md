# TreeSensing in Redis

This folder contains the codes for implementing TreeSensing on top of [Redis](https://redis.io/), a widely used in-memory data structure store. We integrate TreeSensing into Redis as a persistence tool to reduce the storage overhead of sketches. Specifically, we first implement a CM sketch using Redis Module, which supports basic insertion and query commands. Then we implement TreeSensing by adding four commands to the CM sketch: TreeSensing compression/recovery, and SketchSensing compression/recovery. Experiments are conducted under Redis 5.0.7.


## File Structures
- `./dataset`: an empty folder, you should download the dataset (see details below) and put the dataset into this folder. 
 
- `./src`: the codes of TreeSensing and SketchSensing on CM sketch, and the related codes to support the execution on Redis.
    
    - ` ./src/basic_sketch` contains codes that defines the basic sketch data structures and operations

    - `./src/basic_cm` contains codes of the modified sketch in Redis, which supports basic insertion/query commands and TreeSensing/SketchSensing compression/recovery commands. 

    - `./src/rmutil` contains utilities that aid the operation of Redis

    - `./src/util` contains header files that define some common functions of our algorithms, including the recovery algorithm of compressive sensing, the ShiftBfEncoder class, the hash functions, etc. 
    

- `RedisControl.py`: a python program that runs our tests through Redis interface calling, and generates a `COMP_DECOMP_TIME_.csv`
file containing the test results.

## Requirements
  - **g++, C++17, CMake**: the code is implemented with C++ and are built with g++ 7.5.0.
  - **Redis**: the code runs on top of the environment of **Redis 7.0.5**
  - **Python**: the `.py` file is to be run through **Python3** cli command.


## Dataset

The dataset used in Redis experiments is a 1-min CAIDA dataset. You can download the CAIDA dataset from the link below, and put the dataset into `./dataset` folder. 

* CAIDA: [CAIDA_redis.dat](https://drive.google.com/file/d/15AzaS9u_7PPHa8xfshGGPgD_8ktNBnR-/view?usp=sharing)


## How to run

- **Build:** run `make all` to build the codes, and then run `make run` to load the output files onto a Redis server.

- **Run:** Open a new instance of `shell` in this directory, run `python3 RedisControl.py ` with appended parameters. You can check out the usage prompt below:
  
```bash
$ python3 RedisSmooth.py [-h] -Mode MODE -w W -d D [-r R] [-s S] -runs RUNS [-shape SHAPE]
```


*  Options:

   *  `-h`: --help  show this help message and exit

   *  `-Mode MODE`:  switching between the Mode of SketchSensing(SS) and TreeEncoding+SketchSensing(TS)

   *  `-w W`: the w parameter, specifiying number of counters in each array of a sketch
   *  `-d D`: the d parameter, specifiying number of arrays in a sketch
   *  `-r R`: compression ratio of SketchSensing, which is set to 3 by default
   *  `-s S`: the separating threshold, which is set to 256 by default

   * `-shape SHAPE`: HierarchicalTree shapes (with default value of 1), we provide the following four shapes of HierarchicalTree:

      | SHAPE | Layers ($l$) | $\delta_1$ | $\delta_2$ | $\delta_3$ | $\kappa_1$ | $\kappa_2$ |
      | - | - | - | - | - | - | - |
      | 1 | 3 | 8 | 8 | 4 | 2 | 2 |
      | 2 | 2 | 4 | 8 | - | 4 | - |
      | 3 | 3 | 4 | 4 | 4 | 2 | 2 |
      | 4 | 2 | 8 | 16 | - | 8 | - |


   
      You can modify the `tower_settings` arrays inside `src\basic_cm\basic_cm.h` to customize more shapes of HierarchicalTree. (You should also set `levelcnt` to the length of the `tower_settings` array) 


   * `-runs RUNS`: number of repeated times

  


### Example

```bash
$ python3 RedisSmooth.py -Mode SS -w 30000 -d 3 -runs 300 -shape 2
```

### Output Format
Our program first prints some information about the dataset and program execution status, and then generates a `.csv` file containing the results of compression/recovery time.