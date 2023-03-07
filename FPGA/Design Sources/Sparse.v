`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 19:18:17
// Design Name: Wang Sha
// Module Name: Sparse
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


module Sparse
#(parameter
	THRESHOLD=20
)
(
	input              Clk,
	input              Reset_n,
	input      [31:0]  Counter,
	output reg [31:0]  Sparse_Result
    );
	
always @ (posedge Clk or negedge Reset_n)
begin
    if(~Reset_n)
    begin
    end
    
    else
    begin
	   if(Counter<=THRESHOLD)
	   begin
		  Sparse_Result	<=	32'd0;
	   end
	   else
	   begin
		  Sparse_Result	<=	Counter-Counter%5;
	   end
	end
end

endmodule
