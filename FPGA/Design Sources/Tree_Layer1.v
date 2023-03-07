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

module Tree_Layer1
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

//-------RAM1--------//Layer1
localparam RamDepth_1=10; 
localparam DataDepth_1=1024;
reg     [RamDepth_1-1:0] ram_addr_a_1;					//ram a-port address
reg	    [7:0]	         ram_data_a_1;					//ram a-port data
reg				         ram_rden_a_1;					//read a-port 
reg				         ram_wren_a_1;					//write a-port
wire    [7:0]	         ram_out_a_1;					//data a-port

reg		[9:0]	Index_Layer1;
reg		[6:0]	Index_Layer2;
reg		[3:0]	NUM;
reg     [15:0]  SUM_Counter;

always @ (posedge Clk or negedge Reset_n)
begin
	if(~Reset_n)
    begin
		//Result		  <=	1'd0;
		ram_wren_a_1  <=    1'b0;
		ram_rden_a_1  <=    1'b0;
		ram_addr_a_1  <=    {(RamDepth_1){1'b0}};
		ram_data_a_1  <=    8'b0;
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
			
			 //Layer1 operates
			ram_addr_a_1    	<=  Index_Layer1;
			ram_data_a_1   	 	<=  Counter[divisor-1:0];
			ram_wren_a_1	    <=  1'b1;
			
       end
       
       else
       begin
		    ram_wren_a_1  <=    1'b0;
		    ram_addr_a_1  <=    {(RamDepth_1){1'b0}};
		    ram_data_a_1  <=    8'b0;
		    //Index_Layer1  <=	10'b0;
		    //Index_Layer2  <=	7'b0;
		    NUM           <=	4'b0;
		    SUM_Counter   <=	16'b0;
		    Next_Counter        <=  Counter;
       end
	   
	end
end

//-------RAM1--------//Layer1
	wire							hash1_clka;		
	wire							hash1_ena;	
	wire							hash1_wea;	
	wire	[RamDepth_1-1:0]		hash1_addra;		
	wire	[7:0]					hash1_dina;		
	wire	[7:0]					hash1_douta;		
	wire							hash1_clkb;		
	wire							hash1_enb;	
	wire							hash1_web;	
	wire	[RamDepth_1-1:0]	    hash1_addrb;		
	wire	[7:0]					hash1_dinb;		
	wire	[7:0]					hash1_doutb;		

	ASYNCRAM#(
					.DataWidth	(8						    ),	//This is data width	
					.DataDepth	(DataDepth_1				),	//for ASYNC,DataDepth must be 2^n (n>=1). for SYNC,DataDepth is a positive number(>=1)
					.RAMAddWidth(RamDepth_1				    )	//RAM address width, RAMAddWidth= log2(DataDepth).			
	)	
	hash1(
					.aclr		(~Reset_n					),	//Reset the all write signal	
					.address_a	(ram_addr_a_1				),	//RAM A port address
					.address_b	(       					),	//RAM B port assress
					.clock_a	(Clk						),	//Port A clock
					.clock_b	(Clk						),	//Port B clock	
					.data_a		(ram_data_a_1				),	//The Inport of data 
					.data_b		(					        ),	//The Inport of data 
					.rden_a		(ram_rden_a_1				),	//active-high, read signal
					.rden_b		(					        ),	//active-high, read signal
					.wren_a		(ram_wren_a_1				),	//active-high, write signal
					.wren_b		(					        ),	//active-high, write signal
					.q_a		(ram_out_a_1				),	//The Output of data
					.q_b		(					        ),	//The Output of data
					// ASIC RAM
					.reset		(							),	//Reset the RAM, active higt
					.clka		(hash1_clka					),	//Port A clock
					.ena		(hash1_ena					),	//Port A enable
					.wea		(hash1_wea					),	//Port A write
					.addra		(hash1_addra				    ),	//Port A address
					.dina		(hash1_dina					),	//Port A input data
					.douta		(hash1_douta				    ),	//Port A output data
					.clkb		(hash1_clkb					),	//Port B clock
					.enb		(hash1_enb					),	//Port B enable
					.web		(hash1_web					),	//Port B write
					.addrb		(hash1_addrb				    ),	//Port B address
					.dinb		(hash1_dinb					),	//Port B input data
					.doutb		(hash1_doutb				    )	//Port B output data	
	);

	ram_8_1024  hash_1(
					.clka		(hash1_clka					),	//ASYNC WriteClk, SYNC use wrclk
					.ena		(hash1_ena					),	//RAM write address
					.wea		(hash1_wea					),	//RAM write address
					.addra		(hash1_addra				    ),	//RAM read address
					.dina		(hash1_dina					),	//RAM input data
					.douta		(hash1_douta				    ),	//RAM output data
					.clkb		(hash1_clkb					),	//ASYNC WriteClk, SYNC use wrclk
					.enb		(hash1_enb					),  //RAM write request
					.web		(hash1_web					),	//RAM write address
					.addrb		(hash1_addrb				    ),  //RAM read request
					.dinb		(hash1_dinb					),	//RAM input data
					.doutb		(hash1_doutb				    )	//RAM output data				
				);

endmodule
