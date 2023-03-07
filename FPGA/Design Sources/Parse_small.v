`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/28 08:45:36
// Design Name: WangSha
// Module Name: Parse_small
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


module Parse_small
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
	   if(Counter>THRESHOLD)
	   begin
		  Sparse_Result	<=	32'd0;
	   end
	   else
	   begin
		  Sparse_Result	<=	Counter;
	   end
	end
end

endmodule
