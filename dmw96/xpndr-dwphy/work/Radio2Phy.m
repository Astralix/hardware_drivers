function Radio2Phy()
%Reg12_Reserved      	  12	 7:4	     1	     15->0
%Register 12 (LO) Channel                 dflt=01;(addr_pll_channel)
%7:4	(Reserved)	(Reserved - left for compatibility only).	0x0
RF22_WriteRegister(12,'7:4',0);
%REFOSCTUNE          	  21	 7:0	     1	    91->88
%Register 21 (LO) Ref Osc Control       dflt=80;(addr_ref_osc_tune)
%Bits 7:0	REFOSCTUNE	Used to switch in caps to trim the PLL reference frequency.  Since RF52A does not support a crystal (only an external reference oscillator), this register does not have any effect.	0x80
RF22_WriteRegister(21,'7:0',88);
%% //////////////////////////////////////////////////// 
%PA1CURTRIM_EVM      	  78	 4:0	     1	    27->31
% Register 78 (PA) PA1 Current Trim (EVM)   dflt=1B;(addr_pa1_trim_evm)
% Bits 4:0	PA1CURTRIM_EVM	PA first stage current 0h – min, 1fh – max (EVM mode)	0x1B
RF22_WriteRegister(78,'4:0',31);
%PA2CURTRIM_EVM      	  79	 4:0	     1	    17->19
% Register 79 (PA) PA2 Current Trim (EVM)   dflt=11;(addr_pa2_trim_evm)
% Bits 4:0	PA2CURTRIM_EVM	PA second stage current 0h – min, 1fh – max (EVM mode)	0x11
RF22_WriteRegister(79,'4:0',19);
%% //////////////////////////////////////////////////// 

% EN_TX_RATE_SEL      	  80	 1:1	     1	     1->0
% Register 80 (PA) PA Control Register     dflt=00;(addr_pa_ctrl)
% Bit 1	EN_TX_RATE_SEL	When set to '1' – enable rate selection between EVM and MASK trim values according to LNAGAIN bit.	0x0
RF22_WriteRegister(80,'1:1',0);
% IND_ADDR            	 126	 6:0	     1	    91->60
% Register 126      Indirect Address register   (ind_addr)
% Bits 6:0	IND_ADDR	Indirect Register File index register.	0x0
%RF22_WriteRegister(126,'6:0',60);
% IND_DATA            	 127	 7:0	     1	    46->17
% Register 127      Indirect Data register  (ind_data)
% Bits 7:0	IND_DATA	Indirect Register File data register.	0x0
%RF22_WriteRegister(127,'7:0',17);
% IQ_IF_ATTN          	 155	 5:0	     1	    18->10
% Register 155 (IQ) IQ IF Attenuator config dflt=00;(addr_iq_if_attn)
% Bits 5:0	IQ_IF_ATTN	IF_ATTN value during IQ calibrations.	0x0
%---------------------------------------------------------------------------------------
% TX_GAIN_EOVR        	 134	 6:6	     1	     0
% K1TX_INIT           	 134	 4:0	     1	     0->15
%RF22_WriteRegister(134,'4:0',15);
%RF22_WriteRegister(134,'6:6',1);
%RF22_WriteRegister(134,'6:0',bin2dec('1001111'));
RF22_WriteRegister(134,'6:0', 2^6 + 15);
% TX_PHASE_EOVR       	 135	 6:6	     1	     0->1
% K2TX_INIT           	 135	 4:0	     1	     0->21
% RF22_WriteRegister(135,'4:0',21);
% RF22_WriteRegister(135,'6:6',1);
%RF22_WriteRegister(135,'6:0',bin2dec('1010101'));
RF22_WriteRegister(135,'6:0', 2^6 + 21);
% RX
RF22_WriteRegister(136,'6:0', 2^6 + 15);
RF22_WriteRegister(137,'6:0', 2^6 + 18);
%---------------------------------------------------------------------------------------

RF22_WriteRegister(155,'5:0',10);
% Pmeasured           	 182	 5:0	     0	    16->11
% Register  182 PWR_CTRL_CTRL1 dflt=xx;(addr_pwr_ctrl_ctrl1)
% 	Default: 0h
% Bits	R/W	Name	Description	
% 5:0	RO	Pmeasured	Power measured with the power detector. 
% Valid at the end of TX slot.	
RF22_WriteRegister(182,'5:0',11);
%det_high_set_vovr   	 184	 5:5	     1	     0->1
% Register  184 PWR_DET1 dflt=04;(addr_pwr_det1)
% 	Default: 04h
% Bits	R/W	Name	Description	
% 5	R/W	det_high_set_vovr		
RF22_WriteRegister(184,'5:5',1);
% if_attn_vovr        	 187	 5:0	     1	     0->18
% Register  187 PWR_IF_CTRL_1 dflt=00;(addr_pwr_if_ctrl1)
% 	Default: 00h
% Bits	R/W	Name	Description	
% 5:0	R/W	If_attn_vovr	If_attn overwrite value	0x0
RF22_WriteRegister(187,'5:0',18);
% current_if_attn     	 188	 5:0	     0	    10->17
% Register  188 PWR_IF_CTRL_2 dflt=00;(addr_pwr_if_ctrl2)
% 	Default: 00h
% Bits	R/W	Name	Description	
% 5:0	RO	Current IF_attn	Current IF_attn used by POWER CONTROL.	0x0
RF22_WriteRegister(188,'5:0',17);
% VGA_STATE2_GAIN     	 196	 6:0	     1	     3->0
% Register  196 PWR_VGA_GAIN_3 dflt=0D;(addr_pwr_vga_gain3)
% 	Default: 0Dh
% Bits	R/W	Name	Description	
% 6:0	R/W	VGA_STATE2_GAIN	VGA state gain	0x0D
RF22_WriteRegister(196,'6:0',0);
% VGA_STATE3_GAIN     	 197	 6:0	     1	    15->2
% Register  197 PWR_VGA_GAIN_4 dflt=19;(addr_pwr_vga_gain4)
% 	Default: 19h
% Bits	R/W	Name	Description	
% 6:0	R/W	VGA_STATE3_GAIN	VGA state gain	0x19
RF22_WriteRegister(197,'6:0',2);
% VGA_STATE4_GAIN     	 198	 6:0	     1	    24->11
% Register  198 PWR_VGA_GAIN_5 dflt=25;(addr_pwr_vga_gain5)
% 	Default: 25h
% Bits	R/W	Name	Description	
% 6:0	R/W	VGA_STATE4_GAIN	VGA state gain	0x25
RF22_WriteRegister(198,'6:0',11);
% VGA_STATE5_GAIN     	 199	 6:0	     1	    35->22
% Register  199 PWR_VGA_GAIN_6 dflt=32;(addr_pwr_vga_gain6)
% 	Default: 32h
% Bits	R/W	Name	Description	
% 6:0	R/W	VGA_STATE5_GAIN	VGA state gain	0x32
RF22_WriteRegister(199,'6:0',22);
% VGA_STATE6_GAIN     	 200	 6:0	     1	    45->36
% Register  200 PWR_VGA_GAIN_7 dflt=3F;(addr_pwr_vga_gain7)
% 	Default: 3Fh
% Bits	R/W	Name	Description	
% 6:0	R/W	VGA_STATE6_GAIN	VGA state gain	0x3F
RF22_WriteRegister(200,'6:0',36);
% vga2_state_vovr     	 201	 7:4	     1	     0->8
% vga1_state_vovr     	 201	 3:0	     1	     0->8
% Register  201 PWR_VGA_CTRL dflt=00;(addr_pwr_vga_ctrl)
% 	Default: 00h
% Bits	R/W	Name	Description	
% 3:0	R/W	vga1_state_vovr 	Force state on VGA1 – value.	0
% 7:4	R/W	Vga2_state_vovr 	Force state on VGA2 – value.	0
RF22_WriteRegister(201,'7:4',8);
RF22_WriteRegister(201,'3:0',8);

% CURVE0_0            	 205	 7:0	     1	    29->36
RF22_WriteRegister(205,'7:0',36);
% CURVE0_1            	 206	 7:0	     1	    55->69
RF22_WriteRegister(206,'7:0',69);
% CURVE0_2_msb        	 207	 7:0	     1	    47->60
RF22_WriteRegister(207,'7:0',60);
% CURVE0_3_msb        	 208	 7:0	     1	    80->97
RF22_WriteRegister(208,'7:0',97);
% CURVE0_4_msb        	 209	 7:0	     1	   141->174
RF22_WriteRegister(209,'7:0',174);
% CURVE0_5_msb        	 210	 7:0	     1	   202->217
RF22_WriteRegister(210,'7:0',217);
% DPL_TRCK_msb        	 211	 1:0	     1	     0->3
RF22_WriteRegister(211,'1:0',3);
% DPL_TRCK_lsb        	 212	 7:0	     1	   124->152
RF22_WriteRegister(212,'7:0',152);
% CURVE1_0            	 211	 7:0	     1	    60->75
RF22_WriteRegister(211,'7:0',75);
% CURVE1_1            	 212	 7:0	     1	   124->152
RF22_WriteRegister(212,'7:0',152);
% CURVE1_2_msb        	 213	 7:0	     1	   113->139
RF22_WriteRegister(213,'7:0',139);
% FREE_PLAY_DPL_THR   	 213	 7:0	     1	   113->139
RF22_WriteRegister(214,'7:0',234);
% CURVE1_3_msb        	 214	 7:0	     1	   187->234
RF22_WriteRegister(215,'7:0',255);
% CURVE1_4_msb        	 215	 7:0	     1	   244->255
RF22_WriteRegister(217,'5:0',18);
% normal_state_atten  	 217	 5:0	     1	     6->18
RF22_WriteRegister(220,'2:2',1);
% pwr_det_curve0_4    	 220	 2:2	     1	     0->1
RF22_WriteRegister(220,'4:4',0);
% pwr_det_curve1_2    	 220	 4:4	     1	     1->0
RF22_WriteRegister(220,'6:6',0);
% pwr_det_curve1_4    	 220	 6:6	     1	     1->0

RF22_WriteRegister(238,'6:0', 2^6 + 0);
RF22_WriteRegister(239,'6:0', 2^6 + 37);

% %Overwrite RF LOFT DACI
 RF22_WriteRegister(62,2^7 + 62)
% %Overwrite RF LOFT DACQ
 RF22_WriteRegister(63,2^7 + 56)

%Override for DAC1_I  
%RF22_WriteRegister(229,2^6 + 99)
%vga_state_eovr
% VGA2_ctrl           	 222	 7:4	     0	     8 
% VGA1_ctrl           	 222	 3:0	     0	     4->2

RF22_WriteRegister(201,'7:4', 8);
RF22_WriteRegister(201,'3:0', 2);
RF22_WriteRegister(204,'1:1', 1);
% Register  187 PWR_IF_CTRL_1 
RF22_WriteRegister(187,'6:0',2^6 + 17);

end