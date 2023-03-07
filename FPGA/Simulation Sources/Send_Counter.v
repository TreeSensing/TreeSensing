`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 19:13:33
// Design Name: Wang Sha
// Module Name: Send_Counter
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


module Send_Counter
#(parameter
	NUM_COUNTER=10,
	NUM_SLICE=2
)
(
	input	wire		Clk,
	input	wire		Reset_n,
	output	reg	[31:0]	Counter
);

	reg 	[7:0]		send_cnt;	
	
always@(posedge Clk or negedge Reset_n)
begin
	if(!Reset_n)
	begin
		send_cnt	<=	8'd0;
	end
	
	else
	begin
		if(send_cnt<(NUM_SLICE*NUM_COUNTER))//send the next counter
		begin
			Counter		<=	1+{$random}%100;
			send_cnt    <=  send_cnt+8'd1;
		end
		else                              //send finished
		begin
			Counter		<=	32'd0;
		end
	end
end

endmodule
