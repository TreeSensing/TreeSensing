`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/28 10:48:05
// Design Name: Wang Sha
// Module Name: Tree_Layer2
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

module Tree_Layer2
#(parameter
    NUM_COUNTER = 10,
	NUM_SLICE   = 3
)
(
    input               Clk,
	input               Reset_n,
	input	    [31:0]	Counter,
	output  reg         Result
);

localparam divisor=3;//2^8

//-------RAM2--------//Layer2
localparam RamDepth_2=7; 
localparam DataDepth_2=128;
reg     [RamDepth_2-1:0] ram_addr_a_2;					//ram a-port address
reg	    [15:0]	         ram_data_a_2;					//ram a-port data
reg				         ram_rden_a_2;					//read a-port 
reg				         ram_wren_a_2;					//write a-port
wire    [15:0]	         ram_out_a_2;					//data a-port

reg		[9:0]	Index_Layer1;
reg		[6:0]	Index_Layer2;
reg		[3:0]	NUM;
reg     [15:0]  SUM_Counter;

always @ (posedge Clk or negedge Reset_n)
begin
	if(~Reset_n)
    begin
		Result		  <=	1'd0;
		ram_wren_a_2  <=    1'b0;
		ram_rden_a_2  <=    1'b0;
		ram_addr_a_2  <=    {(RamDepth_2){1'b0}};
		ram_data_a_2  <=    16'b0;
		Index_Layer1  <=	10'b0;
		Index_Layer2  <=	7'b0;
		NUM           <=	4'b0;
		SUM_Counter   <=	16'b0;
    end
	
	else
    begin
       if(Counter!="X" && Index_Layer1<(NUM_COUNTER*NUM_SLICE))
       begin
			Index_Layer1    <=   Index_Layer1+1'b1;
			//Layer2 operates
			if(NUM>=7)
			begin
				NUM     	<=	4'b0;
				SUM_Counter <=	16'b0;
				Index_Layer2<=	Index_Layer2+1;
				ram_addr_a_2<=  Index_Layer2;
				ram_data_a_2<=  SUM_Counter+(Counter>>divisor);
				ram_wren_a_2<=  1'b1;			
				$display("woshi2write:Index_Layer2=%d,Layer2=%d",Index_Layer2,SUM_Counter+(Counter>>divisor));	
			end
			else
			begin
				NUM         <=	NUM+1'b1;
				SUM_Counter <=	SUM_Counter+(Counter>>divisor);
				ram_wren_a_2<=  1'b0;
				ram_addr_a_2<=  {(RamDepth_2){1'b0}};
				ram_data_a_2<=  16'b0;
				$display("woshi2:Index_Layer2=%d,Layer2=%d",Index_Layer2,SUM_Counter+(Counter>>divisor));	
			end
			Result   <=  1'b1;
       end
       
       else
       begin
		    ram_wren_a_2  <=    1'b0;
		    ram_addr_a_2  <=    {(RamDepth_2){1'b0}};
		    ram_data_a_2  <=    16'b0;
		    //Index_Layer1  <=	10'b0;
		    //Index_Layer2  <=	7'b0;
		    NUM           <=	4'b0;
		    SUM_Counter   <=	16'b0;
		    Result        <=    1'b0;
       end
	   
	end
end


//-------RAM2--------//Layer2
	wire							hash2_clka;		
	wire							hash2_ena;	
	wire							hash2_wea;	
	wire	[RamDepth_2-1:0]		hash2_addra;		
	wire	[15:0]					hash2_dina;		
	wire	[15:0]					hash2_douta;		
	wire							hash2_clkb;		
	wire							hash2_enb;	
	wire							hash2_web;	
	wire	[RamDepth_2-1:0]	    hash2_addrb;		
	wire	[15:0]					hash2_dinb;		
	wire	[15:0]					hash2_doutb;		

	ASYNCRAM#(
					.DataWidth	(16						    ),	//This is data width	
					.DataDepth	(DataDepth_2				),	//for ASYNC,DataDepth must be 2^n (n>=1). for SYNC,DataDepth is a positive number(>=1)
					.RAMAddWidth(RamDepth_2				    )	//RAM address width, RAMAddWidth= log2(DataDepth).			
	)	
	hash2(
					.aclr		(~Reset_n					),	//Reset the all write signal	
					.address_a	(ram_addr_a_2				),	//RAM A port address
					.address_b	(       					),	//RAM B port assress
					.clock_a	(Clk						),	//Port A clock
					.clock_b	(Clk						),	//Port B clock	
					.data_a		(ram_data_a_2				),	//The Inport of data 
					.data_b		(					        ),	//The Inport of data 
					.rden_a		(ram_rden_a_2				),	//active-high, read signal
					.rden_b		(					        ),	//active-high, read signal
					.wren_a		(ram_wren_a_2				),	//active-high, write signal
					.wren_b		(					        ),	//active-high, write signal
					.q_a		(ram_out_a_2				),	//The Output of data
					.q_b		(					        ),	//The Output of data
					// ASIC RAM
					.reset		(							),	//Reset the RAM, active higt
					.clka		(hash2_clka					),	//Port A clock
					.ena		(hash2_ena					),	//Port A enable
					.wea		(hash2_wea					),	//Port A write
					.addra		(hash2_addra			    ),	//Port A address
					.dina		(hash2_dina					),	//Port A input data
					.douta		(hash2_douta			    ),	//Port A output data
					.clkb		(hash2_clkb					),	//Port B clock
					.enb		(hash2_enb					),	//Port B enable
					.web		(hash2_web					),	//Port B write
					.addrb		(hash2_addrb			    ),	//Port B address
					.dinb		(hash2_dinb					),	//Port B input data
					.doutb		(hash2_doutb			    )	//Port B output data	
	);

	ram_16_128  hash_2(
					.clka		(hash2_clka					),	//ASYNC WriteClk, SYNC use wrclk
					.ena		(hash2_ena					),	//RAM write address
					.wea		(hash2_wea					),	//RAM write address
					.addra		(hash2_addra			    ),	//RAM read address
					.dina		(hash2_dina					),	//RAM input data
					.douta		(hash2_douta				),	//RAM output data
					.clkb		(hash2_clkb					),	//ASYNC WriteClk, SYNC use wrclk
					.enb		(hash2_enb					),  //RAM write request
					.web		(hash2_web					),	//RAM write address
					.addrb		(hash2_addrb			    ),  //RAM read request
					.dinb		(hash2_dinb					),	//RAM input data
					.doutb		(hash2_doutb				)	//RAM output data				
				);

endmodule
