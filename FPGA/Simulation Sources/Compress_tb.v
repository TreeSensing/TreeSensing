`timescale 1ns / 1ps
`define clk_cycle 4

//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 19:13:09
// Design Name: Wang Sha
// Module Name: Compress_tb
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


module Compress_tb();

//parameter
localparam NUM_COUNTER=10;		//the number of counters per slice
localparam NUM_SLICE=3;			//the number of slices
localparam THRESHOLD=30;		//the threshold of sparse
localparam SENSE_COL=3;			//the column of sensing matrix

//input
	reg				clk;
	reg				rst_n;
	wire	[31:0]	Counter;
	reg		        pkt_begin;	//send pkt		

//output
    wire            Result_1;
	wire	      	Result_2;
	
//initial signal
initial
begin
	clk=1'b0;
	rst_n=1'b0;
	pkt_begin=1'b0;
	
	//reset release
	#28 rst_n=1'b1;
	#80 pkt_begin=1'b1;
	#1000;
	$finish;
end

//generate clock
always #`clk_cycle clk=~clk;

Send_Counter#(
    .NUM_COUNTER	(NUM_COUNTER),
    .NUM_SLICE		(NUM_SLICE)
)
send_inst(
    .Clk		    (clk),
	.Reset_n		(pkt_begin),
	.Counter        (Counter)
);

Compress_top#(
    .NUM_COUNTER	(NUM_COUNTER),
    .NUM_SLICE		(NUM_SLICE),
    .THRESHOLD		(THRESHOLD),
	.SENSE_COL		(SENSE_COL)
)
test_inst(
    .SYS_CLK		(clk),
	.RESET_N		(rst_n),
	.Counter		(Counter),
	.Flag_TowerEncoding         (Result_1),
	.Flag_SketchSensing			(Result_2)
);

endmodule
