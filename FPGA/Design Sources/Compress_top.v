`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 16:27:05
// Design Name: Wang Sha
// Module Name: Compress_top
// Project Name: Wang Sha
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
//"PATTERN_1" represents TowerEnciding
//"PATTERN_2" represents SketchSensing
//"PATTERN_3" represents TowerEnciding and SketchSensing
//`define PATTERN_1
//`define PATTERN_2
`define PATTERN_3

module Compress_top
#(parameter
    NUM_COUNTER = 10,
    NUM_SLICE = 2,
    THRESHOLD = 20,
    SENSE_COL =3
)
(
    input           SYS_CLK,
	input           RESET_N,
	input	[31:0]	Counter,
	output          Flag_TowerEncoding,
	output         	Flag_SketchSensing
);
   
`ifdef PATTERN_1
    TowerEncoding#(
        .NUM_COUNTER    (NUM_COUNTER),
        .NUM_SLICE      (NUM_SLICE),
        .THRESHOLD		(THRESHOLD)
    )
    towerencoding_inst(
        .Clk		    (SYS_CLK),
        .Reset_n		(RESET_N),
        .Counter		(Counter),
        .Result			(Flag_TowerEncoding)
    );
`endif
    
`ifdef PATTERN_2
    SketchSensing#(
        .NUM_COUNTER	(NUM_COUNTER),
        .NUM_SLICE		(NUM_SLICE),
        .THRESHOLD		(THRESHOLD),
	   .SENSE_COL		(SENSE_COL)
    )
    sketchsensing_inst(
        .Clk		    (SYS_CLK),
        .Reset_n		(RESET_N),
        .Counter		(Counter),
        .Result			(Flag_SketchSensing)
    );
`endif

`ifdef PATTERN_3
    TowerEncoding#(
        .NUM_COUNTER    (NUM_COUNTER),
        .NUM_SLICE      (NUM_SLICE),
        .THRESHOLD		(THRESHOLD)
    )
    towerencoding_inst(
        .Clk		    (SYS_CLK),
        .Reset_n		(RESET_N),
        .Counter		(Counter),
        .Result			(Flag_TowerEncoding)
    );
    
    SketchSensing#(
        .NUM_COUNTER	(NUM_COUNTER),
        .NUM_SLICE		(NUM_SLICE),
        .THRESHOLD		(THRESHOLD),
	   .SENSE_COL		(SENSE_COL)
    )
    sketchsensing_inst(
        .Clk		    (SYS_CLK),
        .Reset_n		(RESET_N),
        .Counter		(Counter),
        .Result			(Flag_SketchSensing)
    );
`endif

endmodule

