`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 18:57:22
// Design Name: Wang Sha
// Module Name: SketchSensing
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


module SketchSensing
#(parameter
    NUM_COUNTER = 10,
    NUM_SLICE = 2,
    THRESHOLD = 20,
    SENSE_COL =3
)
(
    input           Clk,
	input           Reset_n,
	input	[31:0]	Counter,
	output         	Result
);

wire    [31:0]  Mid_Counter_0;
wire    [31:0]  Mid_Counter_1;
wire    [31:0]  Mid_Counter_2;

Sparse#(
    .THRESHOLD      (THRESHOLD)
)
sparse_inst(
    .Clk            (Clk),
    .Reset_n        (Reset_n),
    .Counter        (Counter),
    .Sparse_Result  (Mid_Counter_0)
);

Col_Compress#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE),
    .SENSE_COL      (SENSE_COL),
	.Index          (0)
)
Col_0(
    .Clk            (Clk),
    .Reset_n        (Reset_n),
    .Spa_Counter    (Mid_Counter_0),
    .Next_Spa_Counter   (Mid_Counter_1),
    .Result         ()
);

Col_Compress#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE),
	.Index          (1)
)
Col_1(
    .Clk            (Clk),
    .Reset_n        (Reset_n),
    .Spa_Counter    (Mid_Counter_1),
    .Next_Spa_Counter   (Mid_Counter_2),
    .Result         ()
);

Col_Compress#(
    .NUM_COUNTER    (NUM_COUNTER),
    .NUM_SLICE      (NUM_SLICE),
	.Index          (2)
)
Col_2(
    .Clk            (Clk),
    .Reset_n        (Reset_n),
    .Spa_Counter    (Mid_Counter_2),
    .Next_Spa_Counter   (),
    .Result         (Result)
);

endmodule
