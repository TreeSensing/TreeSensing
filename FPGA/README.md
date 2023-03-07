# TreeSensing on FPGA

## Overview

We implement TreeSensing on an FPGA network platform (Virtex-7 VC709). For TreeEncoding, we use the HierarchicalTree of Shape#3 and do not use the ShiftEncoder optimization. The implementation of TreeEncoding consists of four hardware modules: separating the small sketch, setting the 1-bit indicators, setting the counters in the first layer, and setting the counters in the second layer. For SketchSensing, the implementation consists of m'+1 hardware modules (m' is the # column of the sensing matrix): separating the large sketch and rounding down, and multiplying a counter with a corresponding counter in the $j_{th}$ column of the sensing matrix (multiplying_j). For example, in the multiplying_j module, the $i_{th}$ counter is multiplied with $\phi[i][j]$ where $\phi$ is the sensing matrix. Both TreeEncoding and SketchSensing are fully pipelined, which inputs one 32-bit counter in each clock cycle. As shown in Table 2, the clock frequency of TreeEncoding and SketchSensing in FPGA are 526MHz and 316MHz, respectively. Therefore, the throughput of TreeSensing using both TreeEncoding and SketchSensing can be 316MHz. Moreover, the logic resource usage is 0.17%, and the memory resource usage is 0.31%.


## File Description

### Design Sources
- ```Compress_top.v``` contains the top-module of the TreeSensing. The developers can choose whether counters enter the TreeEncoding or SketchSending module.
  - ```TowerEncoding.v``` contains the top-module of the TreeEncoding. This is used to transmit parameters between submodules.
    - ```Parse_small.v``` contains operations on counters. Compared to the threshold, this will set larger counters to zero and keep smaller counters.
    - ```Indicator.v``` contains the calculation to indicators what counters enter the second layer.
    - ```Tree_Layer1.v``` contains the calculation and storage to the counter's remainder.
    - ```Tree_Layer2.v``` contains the calculation and storage to the counter's quotient. 
  - ```SketchSensing.v``` contains the top-module of the  SketchSensing. This is used to transmit parameters between submodules. Specially, the developers need to instantiate x Col_Compress module where the compressed matrix has x columns. 
    - ```Sparse.v``` contains operations on counters. Compared to the threshold, this will set smaller counters to zero and set larger counters to be a multiple of five down.
    - ```Col_Compress.v``` contains the implementation of matrix multiplication calculation. The developers can modify values of the used compress_matrix in this module. 
- ```IP``` contains RAM IP Cores used by other modules. 

### Simulation Sources
- ```Compress_tb.v``` contains the top_module of the TreeSensing's simulation. 
  - ```Send_Counter.v``` contains the implementation of send test counters. The current approach is to generate different counters randomly.  


### Constraints
- ```Compress_Sketch.xdc``` contains time constraints for synthesis. The current clock frequency is 250MHz.



## Our experimental setups

* Compile: `Vivado 2020.02`

* Language: `Verilog`

* FPGA version: `virtes-7 xc7vx690tffg1761-2`
