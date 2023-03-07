`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2022/06/27 19:19:15
// Design Name: 
// Module Name: Col_Compress
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


module Col_Compress
#(parameter
	NUM_COUNTER = 10,
	NUM_SLICE   = 2,
	SENSE_COL   = 3,
	Index=0//represent the column Index
)
(
	input				Clk,
	input				Reset_n,
	input		[31:0]	Spa_Counter,
	output  reg [31:0]  Next_Spa_Counter,
	output	reg	    	Result			
    );

//-------RAM--------//
localparam RamDepth=10; 
localparam DataDepth=1024;
reg     [RamDepth-1:0]   ram_addr_a;					//ram a-port address
reg	    [31:0]	         ram_data_a;					//ram a-port data
reg				         ram_rden_a;					//read a-port 
reg				         ram_wren_a;					//write a-port
wire    [31:0]	         ram_out_a;					     //data a-port
	
//-----other parameter-----//
reg     [NUM_COUNTER-1:0]   Sense_Matrix_Col;           //the value of matrix's column
reg     [31:0]	            Mid_Result;                 //storing intermediate computations
reg	    [7:0]	            Flag_Row;                   //current the pointer of matrix's row
reg     [7:0]	            Flag_Slice;                 //current the pointer of slice
	
//set value of the i column of sense matrix
initial
begin
    if(Index==0)
    begin
        Sense_Matrix_Col=10'b1111111111;//right¡ª>the 0th bit
    end
    
    else if(Index==1)
    begin
        Sense_Matrix_Col=10'b0111111111;
    end
    
    else if(Index==2)
    begin
        Sense_Matrix_Col=10'b0;
    end
    
end

always @ (posedge Clk or negedge Reset_n)
begin
	if(~Reset_n)
	begin
		Result		<=	1'd0;
		Mid_Result	<=	32'd0;
		Flag_Row	<=	8'd0;
		Flag_Slice	<=	8'd0;
		ram_wren_a  <=  1'b0;
		ram_addr_a  <=  {(RamDepth){1'b0}};
		ram_data_a  <=  32'b0;
	end
	
	else
	begin
	   if(Flag_Row<(NUM_COUNTER-1) && Flag_Slice<NUM_SLICE && Spa_Counter!="X")
	   begin
	       //disabled RAM write
	       ram_addr_a   <=  {(RamDepth){1'b0}};
		   ram_data_a   <=  {(32){1'b0}};
	       ram_wren_a   <=  1'b0;
	   
	       Mid_Result	<=	(Spa_Counter*Sense_Matrix_Col[Flag_Row])+Mid_Result;
	       Flag_Row	    <=	Flag_Row+8'd1;
	       Next_Spa_Counter <=  Spa_Counter;
	       Result       <=  1'b1;
	       //$display("output:%d",Mid_Result);
	       //$display("Compress process:the %dth row of the %dth column of the %dth slice is %d.",Flag_Row,Index,Flag_Slice,(Spa_Counter*Sense_Matrix_Col[Flag_Row])+Mid_Result);	
		end
		else if(Flag_Row==(NUM_COUNTER-1) && Flag_Slice<NUM_SLICE)
		begin
			//writing result in RAM and output.
			ram_addr_a   <=  Flag_Slice*SENSE_COL+Index;
			ram_data_a   <=  (Spa_Counter*Sense_Matrix_Col[Flag_Row])+Mid_Result;
			ram_wren_a   <=  1'b1;
			
			Flag_Row	<=	8'd0;
			Mid_Result	<=	32'd0;
			Flag_Slice	<=	Flag_Slice+8'd1;
			Next_Spa_Counter <=  Spa_Counter;
			Result       <=  1'b1;
			$display("Compress result:the %dth column of the %dth slice is %d.",Index,Flag_Slice,Spa_Counter*Sense_Matrix_Col[Flag_Row]+Mid_Result);
		end
		
		else
		begin
		     //disabled RAM write
	         ram_addr_a   <=  {(RamDepth){1'b0}};
		     ram_data_a   <=  {(32){1'b0}};
	         ram_wren_a   <=  1'b0;
	         Result       <=  1'b0;
		end		
	end
end

//----ram----//
	wire							hash_clka;		
	wire							hash_ena;	
	wire							hash_wea;	
	wire	[RamDepth-1:0]		    hash_addra;		
	wire	[31:0]					hash_dina;		
	wire	[31:0]					hash_douta;		
	wire							hash_clkb;		
	wire							hash_enb;	
	wire							hash_web;	
	wire	[RamDepth-1:0]		    hash_addrb;		
	wire	[31:0]					hash_dinb;		
	wire	[31:0]					hash_doutb;		

	ASYNCRAM#(
					.DataWidth	(32						    ),	//This is data width	
					.DataDepth	(DataDepth					),	//for ASYNC,DataDepth must be 2^n (n>=1). for SYNC,DataDepth is a positive number(>=1)
					.RAMAddWidth(RamDepth				    )	//RAM address width, RAMAddWidth= log2(DataDepth).			
	)	
	hash(
					.aclr		(~Reset_n					),	//Reset the all write signal	
					.address_a	(ram_addr_a					),	//RAM A port address
					.address_b	(       					),	//RAM B port assress
					.clock_a	(Clk						),	//Port A clock
					.clock_b	(Clk						),	//Port B clock	
					.data_a		(ram_data_a					),	//The Inport of data 
					.data_b		(					        ),	//The Inport of data 
					.rden_a		(ram_rden_a					),	//active-high, read signal
					.rden_b		(					        ),	//active-high, read signal
					.wren_a		(ram_wren_a					),	//active-high, write signal
					.wren_b		(					        ),	//active-high, write signal
					.q_a		(ram_out_a					),	//The Output of data
					.q_b		(					        ),	//The Output of data
					// ASIC RAM
					.reset		(							),	//Reset the RAM, active higt
					.clka		(hash_clka					),	//Port A clock
					.ena		(hash_ena					),	//Port A enable
					.wea		(hash_wea					),	//Port A write
					.addra		(hash_addra				    ),	//Port A address
					.dina		(hash_dina					),	//Port A input data
					.douta		(hash_douta				    ),	//Port A output data
					.clkb		(hash_clkb					),	//Port B clock
					.enb		(hash_enb					),	//Port B enable
					.web		(hash_web					),	//Port B write
					.addrb		(hash_addrb				    ),	//Port B address
					.dinb		(hash_dinb					),	//Port B input data
					.doutb		(hash_doutb				    )	//Port B output data	
	);

	ram_32_1024  hash_0(
					.clka		(hash_clka					),	//ASYNC WriteClk, SYNC use wrclk
					.ena		(hash_ena					),	//RAM write address
					.wea		(hash_wea					),	//RAM write address
					.addra		(hash_addra				    ),	//RAM read address
					.dina		(hash_dina					),	//RAM input data
					.douta		(hash_douta				    ),	//RAM output data
					.clkb		(hash_clkb					),	//ASYNC WriteClk, SYNC use wrclk
					.enb		(hash_enb					),  //RAM write request
					.web		(hash_web					),	//RAM write address
					.addrb		(hash_addrb				    ),  //RAM read request
					.dinb		(hash_dinb					),	//RAM input data
					.doutb		(hash_doutb				    )	//RAM output data				
				);


endmodule
