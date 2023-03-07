`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 18:56:52
// Design Name: Wang Sha
// Module Name: TowerEncoding
// Project Name: 
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


module TowerEncoding
#(parameter
    NUM_COUNTER = 10,
	NUM_SLICE   = 2,
    THRESHOLD   = 20
)
(
    input           Clk,
	input           Reset_n,
	input	[31:0]	Counter,
	output          Result
);

wire [31:0] Sparse_Counter;
wire [31:0] Indicator_Counter;
wire [31:0] Layer1_Counter;

Parse_small#(
    .THRESHOLD		(THRESHOLD)
)
parse_small_inst(
    .Clk		    (Clk),
    .Reset_n		(Reset_n),
    .Counter		(Counter),
    .Sparse_Result	(Sparse_Counter)
);

Indicator#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE)
)
indicator_inst(
    .Clk		    (Clk),
    .Reset_n		(Reset_n),
    .Counter		(Sparse_Counter),
    .Next_Counter   (Indicator_Counter)
    //.Result			(Result)
);

Tree_Layer1#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE)
)
treelayer1_inst(
    .Clk		    (Clk),
    .Reset_n		(Reset_n),
    .Counter		(Indicator_Counter),
    .Next_Counter   (Layer1_Counter)
    //.Result			(Result)
);

Tree_Layer2#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE)
)
treelayer2_inst(
    .Clk		    (Clk),
    .Reset_n		(Reset_n),
    .Counter		(Layer1_Counter),
    .Result			(Result)
);

//TowerTree#(
//    .NUM_COUNTER    (NUM_COUNTER),
//    .NUM_SLICE      (NUM_SLICE)
//)
//towertree_inst(
//    .Clk		    (Clk),
//    .Reset_n		(Reset_n),
//    .Counter		(Sparse_Counter),
//    .Result			(Result)
//);

endmodule
