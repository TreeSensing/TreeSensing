`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/28 10:48:05
// Design Name: 
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

module Indicator
#(parameter
    NUM_COUNTER = 10,
	NUM_SLICE   = 3
)
(
    input               Clk,
	input               Reset_n,
	input	    [31:0]	Counter,
    output  reg [31:0]  Next_Counter
);

localparam divisor=3;//2^8

//-------RAM0--------//indicators
localparam RamDepth_0=10; 
localparam DataDepth_0=1024;
reg     [RamDepth_0-1:0] ram_addr_a_0;					//ram a-port address
reg	    		         ram_data_a_0;					//ram a-port data
reg				         ram_rden_a_0;					//read a-port 
reg				         ram_wren_a_0;					//write a-port
wire    		         ram_out_a_0;					//data a-port

reg		[9:0]	Index_Layer1;
reg		[6:0]	Index_Layer2;
reg		[3:0]	NUM;
reg     [15:0]  SUM_Counter;

always @ (posedge Clk or negedge Reset_n)
begin
	if(~Reset_n)
    begin
		//Result		  <=	1'd0;
		ram_wren_a_0  <=    1'b0;
		ram_addr_a_0  <=    {(RamDepth_0){1'b0}};
		ram_data_a_0  <=    1'b0;
		Index_Layer1  <=	10'b0;
		Index_Layer2  <=	7'b0;
		NUM           <=	4'b0;
		SUM_Counter   <=	16'b0;
    end
	
	else
    begin
       if(Counter!="X" && Index_Layer1<(NUM_COUNTER*NUM_SLICE))
       begin
            Index_Layer1		<=	Index_Layer1+1'b1;
			Next_Counter        <=  Counter;
			//Indicator operates
			if((Counter>>divisor)>0)
			begin
				ram_addr_a_0    <=  Index_Layer1;
				ram_data_a_0    <=  1'b1;
				ram_wren_a_0    <=  1'b1;
				$display("woshi1:Index_Layer1=%d,Indicator=%d,Layer1=%d",Index_Layer1,1'b1,Counter[divisor-1:0]);			
			end
			else
			begin
				ram_addr_a_0    <=  {(RamDepth_0){1'b0}};
				ram_data_a_0    <=  1'b0;
				ram_wren_a_0    <=  1'b0;
				$display("woshi1:Index_Layer1=%d,Indicator=%d,Layer1=%d",Index_Layer1,1'b0,Counter[divisor-1:0]);	
			end
       end
       
       else
       begin
            ram_wren_a_0  <=    1'b0;
		    ram_addr_a_0  <=    {(RamDepth_0){1'b0}};
		    ram_data_a_0  <=    1'b0;
		    //Index_Layer1  <=	10'b0;
		    //Index_Layer2  <=	7'b0;
		    NUM           <=	4'b0;
		    SUM_Counter   <=	16'b0;
		    Next_Counter        <=  Counter;
       end
	   
	end
end

//-------RAM0--------//indicators
	wire							hash0_clka;		
	wire							hash0_ena;	
	wire							hash0_wea;	
	wire	[RamDepth_0-1:0]		hash0_addra;		
	wire							hash0_dina;		
	wire							hash0_douta;		
	wire							hash0_clkb;		
	wire							hash0_enb;	
	wire							hash0_web;	
	wire	[RamDepth_0-1:0]		hash0_addrb;		
	wire							hash0_dinb;		
	wire							hash0_doutb;		

	ASYNCRAM#(
					.DataWidth	(1						    ),	//This is data width	
					.DataDepth	(DataDepth_0				),	//for ASYNC,DataDepth must be 2^n (n>=1). for SYNC,DataDepth is a positive number(>=1)
					.RAMAddWidth(RamDepth_0				    )	//RAM address width, RAMAddWidth= log2(DataDepth).			
	)	
	hash0(
					.aclr		(~Reset_n					),	//Reset the all write signal	
					.address_a	(ram_addr_a_0				),	//RAM A port address
					.address_b	(       					),	//RAM B port assress
					.clock_a	(Clk						),	//Port A clock
					.clock_b	(Clk						),	//Port B clock	
					.data_a		(ram_data_a_0				),	//The Inport of data 
					.data_b		(					        ),	//The Inport of data 
					.rden_a		(ram_rden_a_0				),	//active-high, read signal
					.rden_b		(					        ),	//active-high, read signal
					.wren_a		(ram_wren_a_0				),	//active-high, write signal
					.wren_b		(					        ),	//active-high, write signal
					.q_a		(ram_out_a_0				),	//The Output of data
					.q_b		(					        ),	//The Output of data
					// ASIC RAM
					.reset		(							),	//Reset the RAM, active higt
					.clka		(hash0_clka					),	//Port A clock
					.ena		(hash0_ena					),	//Port A enable
					.wea		(hash0_wea					),	//Port A write
					.addra		(hash0_addra			    ),	//Port A address
					.dina		(hash0_dina					),	//Port A input data
					.douta		(hash0_douta			    ),	//Port A output data
					.clkb		(hash0_clkb					),	//Port B clock
					.enb		(hash0_enb					),	//Port B enable
					.web		(hash0_web					),	//Port B write
					.addrb		(hash0_addrb			    ),	//Port B address
					.dinb		(hash0_dinb					),	//Port B input data
					.doutb		(hash0_doutb			    )	//Port B output data	
	);

	ram_1_1024  hash_0(
					.clka		(hash0_clka					),	//ASYNC WriteClk, SYNC use wrclk
					.ena		(hash0_ena					),	//RAM write address
					.wea		(hash0_wea					),	//RAM write address
					.addra		(hash0_addra				),	//RAM read address
					.dina		(hash0_dina					),	//RAM input data
					.douta		(hash0_douta			    ),	//RAM output data
					.clkb		(hash0_clkb					),	//ASYNC WriteClk, SYNC use wrclk
					.enb		(hash0_enb					),  //RAM write request
					.web		(hash0_web					),	//RAM write address
					.addrb		(hash0_addrb			    ),  //RAM read request
					.dinb		(hash0_dinb					),	//RAM input data
					.doutb		(hash0_doutb			    )	//RAM output data				
				);


endmodule
