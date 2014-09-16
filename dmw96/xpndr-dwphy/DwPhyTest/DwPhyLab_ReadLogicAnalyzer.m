function data = DwPhyLab_ReadLogicAnalyzer(TestMode)
% data = DwPhyLab_ReadLogicAnalyzer(TestMode)
% 
%       Read data from the Agilent 1672G Logic Analyzer and format for the specified
%       baseband test mode.

% Written by Barrett Brickner and Xinying Yu
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, TestMode=0; end
data.TimeStamp = datestr(now);

%% Read the Waveform
[status, h] = viOpen('GPIB::7');
if (status~=0)||(~h), error('Problem Opening Logic Analyzer'); end

viWrite(h,'*CLS'); % added 3/19 to clear aborted download
viWrite(h, ':SEL 1') ;% Select Control to be Analyzer; Required
viWrite(h, ':SYSTEM:HEADER ON');
viWrite(h, ':SYSTEM:LONGFORM OFF'); 
viSetAttribute(h,'VI_ATTR_TMO_VALUE', 300e3); % set the GPIB time out value large enough for max memory length
viWrite(h, ':SYST:DATA?'); %   
[status, sys_data]=viReadBinary(h, 50000000);
viClose(h);
if status ~=0, error('Problem reading waveform from the logic analyzer'); end

%% Map the Data to TestPort[19:]
off=21;  % # of bytes for the returned data header; depends on the mode;
n_row=bitshift(uint32(sys_data(off+257)),24)+bitshift(uint32(sys_data(off+258)),16)+bitshift(uint32(sys_data(off+259)),8)+uint32(sys_data(off+257));
ofs = 21 + 590;
i=1:n_row;   
off=ofs+12*(i-1);
Y.clk(i) = bitand(sys_data(off+4), 1);
%        data.LoByte(i) = uint32(sys_data(off+12)); % D9-D0 of Pod1
% Words are 10 bit (match data converter words)
LoWord(i) = bitshift(uint32(bitand(sys_data(off+11),3)),8) + uint32(sys_data(off+12)); % D9-D0 of Pod1
HiWord(i) = bitshift(uint32(bitand(sys_data(off+9),15)),6)+uint32(bitshift(sys_data(off+11),-2)); % [D11-D8 of pod 2 D15-D10 of pod 1]
data.X(i) = 2^10*HiWord(i) + LoWord(i);

%% Data formatting

switch TestMode,
    case -1, % A321 PLL Lock Debug (T = 8ns timing)
        data.T = 4e-9;
        data.MC0       = bitget(data.X, 1);
        data.MC1       = bitget(data.X, 2);
        data.PHY_SEN   = bitget(data.X, 3);
        data.PHY_SDCLK = bitget(data.X, 4);
        data.PHY_SDATA = bitget(data.X, 5);
    case 5,
        data.DW_PHYEnB = bitget(data.X,1);
        data.State     = bitand(data.X,bin2dec('00000000000000111110'))/2^1;
        data.FrontCtrl = bitand(data.X,bin2dec('00000000111111000000'))/2^6;
        data.PHY_CCA   = bitand(data.X,bin2dec('00000001000000000000'))/2^12;
        data.PHY_RX_EN = bitand(data.X,bin2dec('00000010000000000000'))/2^13;
        data.MAC_TX_EN = bitand(data.X,bin2dec('00000100000000000000'))/2^14;
        data.DFE       = bitand(data.X,bin2dec('11111000000000000000'))/2^15;
        data.t         = ((1:length(data.X))-1) / 80;
    case 6,
        data.RSSI      = bitand(data.X,bin2dec('00000000000011111111'))/2^0;
        data.ValidRSSI = bitand(data.X,bin2dec('00000000000100000000'))/2^8;
        data.StepDown  = bitand(data.X,bin2dec('00000000001000000000'))/2^9;
        data.StepUp    = bitand(data.X,bin2dec('00000000010000000000'))/2^10;
        data.RxFault   = bitand(data.X,bin2dec('00000000100000000000'))/2^11;
        data.PHY_CCA   = bitand(data.X,bin2dec('00000001000000000000'))/2^12;
        data.PHY_RX_EN = bitand(data.X,bin2dec('00000010000000000000'))/2^13;
        data.PulseDet  = bitand(data.X,bin2dec('00000100000000000000'))/2^14;
        data.DFE       = bitand(data.X,bin2dec('11111000000000000000'))/2^15;
        data.t         = ((1:length(data.X))-1) / 80;
    case {'4000','2000'},
        data.PortWrite = bitand(data.X,bin2dec('10000000000000000000'))/2^19;
        data.PortData  = bitand(data.X,bin2dec('01111111100000000000'))/2^11;
        data.InstAddr  = bitand(data.X,bin2dec('00000000011111111111'))/2^0;
    case {'10'},
        data.rxd_pct_dec_err = bitand(data.X,bin2dec('10000000000000000000'))/2^19;
        data.rxm_pver_err    = bitand(data.X,bin2dec('01000000000000000000'))/2^18;
        data.rxm_fcs_err     = bitand(data.X,bin2dec('00100000000000000000'))/2^17;
        data.rxm_len_eof_err = bitand(data.X,bin2dec('00010000000000000000'))/2^16;
        data.rxm_rxq_full_err= bitand(data.X,bin2dec('00001000000000000000'))/2^15;
        data.rxf_undflw      = bitand(data.X,bin2dec('00000100000000000000'))/2^14;
        data.rxf_ovrflw      = bitand(data.X,bin2dec('00000010000000000000'))/2^13;
        data.rxm_intrpt      = bitand(data.X,bin2dec('00000001000000000000'))/2^12;
        data.rxq_fcount      = bitand(data.X,bin2dec('00000000111111110000'))/2^4;
        data.rx_type_cck     = bitand(data.X,bin2dec('00000000000000001000'))/2^3;
        data.rxm_sof         = bitand(data.X,bin2dec('00000000000000000100'))/2^2;
        data.rxm_eof         = bitand(data.X,bin2dec('00000000000000000010'))/2^1;
        data.rxm_inprog      = bitand(data.X,bin2dec('00000000000000000001'))/2^0;
    case {'24'},
        data.rx_dma_eof      = bitand(data.X,bin2dec('10000000000000000000'))/2^19;
        data.rx_dma_enbl     = bitand(data.X,bin2dec('01000000000000000000'))/2^18;
        data.dev_mac_irq     = bitand(data.X,bin2dec('00100000000000000000'))/2^17;
        data.rxm_intrpt      = bitand(data.X,bin2dec('00010000000000000000'))/2^16;
        data.rx_dma_req      = bitand(data.X,bin2dec('00001000000000000000'))/2^15;
        data.rxd_fifo_rdy    = bitand(data.X,bin2dec('00000100000000000000'))/2^14;
        data.phy_rx_d        = bitand(data.X,bin2dec('00000011111111000000'))/2^6;
        data.phy_rx_wr       = bitand(data.X,bin2dec('00000000000000100000'))/2^5;
        data.phy_rx_en       = bitand(data.X,bin2dec('00000000000000010000'))/2^4;
        data.hbm_ps_schd_desc3=bitand(data.X,bin2dec('00000000000000001000'))/2^3;
        data.txm_state_idle  = bitand(data.X,bin2dec('00000000000000000100'))/2^2;
        data.rxm_inprog      = bitand(data.X,bin2dec('00000000000000000010'))/2^1;
        data.phy_cca         = bitand(data.X,bin2dec('00000000000000000001'))/2^0;
end

