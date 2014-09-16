function data = DwPhyTest_RunTest( TestID, opt )
%data = DwPhyTest_RunTest(TestID, opt) runs the specified test
%
%   TestID  Description
%   ------  --------------------------------------------------------------------------
%    1      Baseband reset
%    2.1    Baseband register read/write
%    2.2    Radio register read/write
%    3      Baseband/radio interface signals (MC, AGAIN, LNAGAIN)
%    4      PLL channel lock
%    5      DwPhy_SetParameterData
%
%   10.1    54 Mbps PER < 10% at -65 dBm, all channels
%   10.2    11 Mbps PER <  8% at -76 dBm, all 2.4 GHz channels
%   10.3    54 Mbps PER < 10% at -65 dBm, all 2.4 GHz channels
%   10.4    54 Mbps PER <  1% at -50 dBm, all channels, RX at SIFS after TX
%   11.1    54 Mbps PER < 10% at -20 dBm, all 2.4 GHz channels
%   11.2    54 Mbps PER < 10% at -30 dBm, all 5 GHz channels
%   11.3    11 Mbps PER <  8% at -20 dBm, all 2.4 GHz channels
%   12.1    All OFDM data rates (basic function)
%   12.2    All DSSS/CCK data rates (basic function)
%   13.1    Device CFO
%   13.2    OFDM-54 Mbps at maximum CFO
%   13.3     CCK-11 Mbps at maximum CFO
%   13.4    DSSS- 2 Mbps at maximum CFO
%   13.5    OFDM-54 Mbps at maximum CFO in the 2.4 GHz band
%   14.1    Diversity: RXA
%   14.2    Diversity: RXB
%   14.3    Diversity: DualRX (MRC)
%   15.1    RxMode = 1 (802.11a)
%   15.2    RxMode = 2 (802.11b)
%   15.3    RxMode = 3 (802.11g)
%   15.4    RxMode = 5 (802.11n, 5 GHz)
%   15.5    RxMode = 7 (802.11n, 2.4 GHz)
%   16.1    Basic AGC: 54 Mbps from -62 to -30 dBm, 5200 MHz
%   16.2    Basic AGC: 54 Mbps from -62 to -20 dBm, 2412 MHz
%   16.3    Basic AGC: 11 Mbps from -70 to -20 dBm, 2412 MHz
%   17.1    Nuisance Packet Filter
%   18.1    SetRxSensitivity, OFDM
%   18.2    SetRxSensitivity, CCK
%   19.1    RxIMR at 2412 and 5200 MHz
%   19.2    RxIMR at 2412 MHz
%   20.1    ADC output offset at 2484 MHz
%   21.1    DMW96 I/Q Calibration, 2.4 GHz channels
%   22.1    RIFS: 65 Mbps, PER < 10% at -64 dBm, 2412 MHz
%
%   ------- EVALUATION AND CHARACTERIZATION TESTS (RX) --------------------------
%   100.1   Sensitivity, L-OFDM rates, 5200 MHz
%   100.2   Sensitivity, L-OFDM rates, 2412 MHz
%   100.3   Sensitivity, DSSS/CCK, 2412 MHz
%   100.4   Sensitivity, L-OFDM rates, 5200 MHz, RXA
%   100.5   Sensitivity, L-OFDM rates, 2412 MHz, RXA
%   100.6   Sensitivity, DSSS/CCK, 2412 MHz, RXB
%   100.7   Sensitivity, L-OFDM, 2412 MHz, RXB
%   100.11  Sensitivity, HT-OFDM, 2412 MHz, RXA
%   100.12  Sensitivity, HT-OFDM, 2412 MHz, RXB
%   100.13  Sensitivity, HT-OFDM, 2412 MHz, MRC
%   101.1   Max Input, 54 Mbps, 5200 MHz
%   101.2   Max Input, 54 Mbps, 2412 MHz
%   101.3   Max Input, 11 Mbps, 2484 MHz, Max CFO
%   101.4   Max Input, 54 Mbps, 5200 MHz, RXA
%   101.5   Max Input, 54 Mbps, 2412 MHz, RXA
%   101.6   Max Input, 54 Mbps at 2.4 GHz band
%   102.1   Full Pr[dBm] Range, 54 Mbps, 5200 MHz   
%   102.2   Full Pr[dBm] Range, 54 Mbps, 2412 MHz   
%   102.3   Full Pr[dBm] Range, 11 Mbps, 2412 MHz   
%   102.4   Full Pr[dBm] Range, 72 Mbps, 5200 MHz
%   102.5   Full Pr[dBm] Range, 72 Mbps, 2412 MHz
%   102.6   Full Pr[dBm] Range, 54 Mbps, 5200 MHz, RXA
%   102.7   Full Pr[dBm] Range, 54 Mbps, 5200 MHz, RXB
%   102.8   Full Pr[dBm] Range, 54 Mbps, 2412 MHz, RXA
%   102.9   Full Pr[dBm] Range, 54 Mbps, 2412 MHz, RXB
%   102.10  Full Pr[dBm] Range, 72 Mbps, 2412 MHz, DCXShift = 1
%   102.11  Full Pr[dBm] Range, 65 Mbps, 2412 MHz
%   102.12  Full Pr[dBm] Range, 65 Mbps, 2412 MHz, RXA
%   102.13  Full Pr[dBm] Range, 65 Mbps, 2412 MHz, RXB
%   102.14  Full Pr[dBm] Range, 54 Mbps, 2484 MHz, RXA, Vary CFO
%   102.21  Full Pr[dBm] Range, 65 Mbps, All Channels, RXA
%   102.22  Full Pr[dBm] Range, 65 Mbps, All Channels, RXB
%   103.1   Sensitivity, 54 Mbps, All Channels, RXA
%   103.2   Sensitivity, 54 Mbps, All Channels, RXB
%   103.3   Sensitivity, 54 Mbps, All Channels
%   103.4   Sensitivity, 11 Mbps, 2.4 GHz Band
%   103.5   Sensitivity,  6 Mbps, All Channels
%   103.6   Sensitivity, 72 Mbps, All Channels
%   104.1   Interference Rejection, 54 Mbps, 5 GHz Band
%   104.2   Interference Rejection, 54 Mbps, 2.4 GHz Band
%   104.3   Interference Rejection, 11 Mbps, 2.4 GHz Band
%   104.4   Interference Rejection, DECT, 54 Mbps, 2412 and 5200 MHz
%   104.5   OFDM ACR: All 2.4 GHz Adjacent Channels above 2412 MHz
%   104.6   OFDM ACR: All 2.4 GHz Adjacent Channels below 2484 MHz
%   105.1   Blocking, Tone, 54 Mbps, 5200 MHz
%   105.2   Blocking, Tone, 54 Mbps, 2412 MHz
%   105.3   Blocking, Tone, 12 Mbps, 5200 MHz
%   105.4   Blocking, Tone, 12 Mbps, 2412 MHz
%   105.5   Blocking, DECT, 54 Mbps, 5200 MHz
%   105.6   Blocking, DECT, 54 Mbps, 2412 MHz
%   105.21  Blocking, Tone, 54 Mbps, 2437 MHz,    1-1800 MHz, VLFX-1125
%   105.22  Blocking, Tone, 54 Mbps, 2437 MHz, 3000-6000 MHz, VHF-3100P
%   105.23  Blocking, Tone, 54 Mbps, 2437 MHz, 1600-3200 MHz
%   105.24  Blocking, Tone, 54 Mbps, 2437 MHz, 1800-1950 MHz, VLFX-1125
%   106.1   Sensitivity, 54 Mbps, RX-TX-RX, All Channels
%   106.2   Full Pr[dBm] Range, 54 Mbps, 5200 MHz, RX-TX-RX
%   106.3   Full Pr[dBm] Range, 54 Mbps, 2412 MHz, RX-TX-RX
%   106.4   PER, 54 Mbps, RX-TX-RX, All Channels From Sleep
%   107.1   Receiver Image Rejection (IMR), All Channels
%   108.1   RSSI, 2442 MHz
%   109.1   ADC output offset, 2484 MHz
%   110.1   LgSigAFE Threshold
%   111.1   Full Pr[dBm] Range, 72 Mbps, 2437 MHz, Vary DCXShift, RXA
%   111.2   Full Pr[dBm] Range, 72 Mbps, 2437 MHz, Vary DCXShift, RXB
%   111.3   Full Pr[dBm] Range, 72 Mbps, 2437 MHz, Vary DCXShift, MRC
%   112.1   Full Pr[dBm] Range, 65 Mbps, 2412 MHz, RIFS
%   113.1   DMW96 RX I/Q-Calibration, 2442 MHz
%   114.1   AGC Option: Gain Step between HT-SIG and HT-STF
%
%   ------- EVALUATION AND CHARACTERIZATION TESTS (TX) --------------------------
%   200     TxPower, All Channels
%   200.1   TxPower, 2.4 GHz Band
%   201.1   TxPower vs TXPWRLVL, 2412 MHz
%   201.2   TxPower vs TXPWRLVL, 4920 MHz
%   201.3   TxPower vs TXPWRLVL, 5200 MHz
%   201.4   TxPower vs TXPWRLVL, 5500 MHz
%   201.5   TxPower vs TXPWRLVL, 5805 MHz
%   202.1   L-OFDM TxMask at 2412 MHz
%   202.2   CCK TxMask at 2412 MHz
%   202.3   L-OFDM TxMask at 5200 MHz
%   202.4   HT-OFDM TxMask at 2412 MHz
%   202.5   HT-OFDM TxMask at 5200 MHz
%   203.1   TxPhaseNoise at 2412 MHz
%   203.2   TxPhaseNoise at 5200 MHz
%   204.1   OFDM-EVM vs Pout at 2412 MHz
%   204.2   OFDM-EVM vs Pout at 5200 MHz
%   204.3   DSSS-EVM vs TXPWRLVL at 2412 MHz
%   205.1   VCO Tuning in the 2.4 GHz Band
%   205.2   VCO Tuning in the 5 GHz Band
%   206.1   L-OFDM EVM vs PSDU Length at 2412 MHz
%   206.2   L-OFDM EVM vs Data rate 2412 MHz
%   206.3   HT-OFDM EVM vs Data rate 2412 MHz
%   207.1   TxPower vs PADGAIN at 2412 MHz
%   207.2   TxPower vs PADGAIN at 5200 MHz
%   208.1   TxPower vs DACOFFSET at 2412 MHz
%   208.2   TxPower vs DACOFFSET at 5200 MHz
%   210.1   OFDM-EVM vs PADGAIN at 2412 MHz
%   210.2   OFDM-EVM vs PADGAIN at 5200 MHz
%   211.1   L-OFDM Spectrum Mask vs PADGAIN at 2412 MHz
%   211.2   L-OFDM Spectrum Mask vs PADGAIN at 5200 MHz
%   212.1   Pout vs TXPWRLVL at PADGAIN = 210, 2412 MHz
%   213.1   Combined OFDM-EVM and Spectrum Mask vs PADGAIN at 2412 MHz
%   214.1   Sleep-Wake Cycle with Single packet TX (ALC bug on B21)
%   215.1   DMW96 TX I/Q-Calibration at 2442 MHz
%
%   ------- TUNING AND OPTIMIZATION PROCEDURES ----------------------------------
%   300.1   InitAGain
%   300.2   Gain and StepLNA
%   300.3   ThSwitchLNA
%   300.4   PER vs Pr[dBm]
%   301.1   {Ca,Cb} Carrier DPLL Loop Gain
%   301.2   {Sa,Sb} Timing DPLL Loop Gain
%   302.1   SigDetTh1
%   302.2   SigDetTh2
%   302.3   CQthreshold (11 Mbps) in 802.11b
%   302.4   CQthreshold ( 1 Mbps) in 802.11b
%   302.5   CQthreshold ( 1 Mbps) in 802.11g
%   303.1   REFOSCTUNE, 2412 MHz
%   304.1   Sensitivity & CCK-ACR vs InitAGain, 2437 MHz, DualRX
%   304.2   Sensitivity & CCK-ACR vs InitAGain, 2437 MHz, RXB
%   304.3   Sensitivity & CCK-ACR vs InitAGain, 2437 MHz, RXA
%   305.1   Sensitivity vs SetRxSensitivity, 2.4 GHz, MRC
%   305.2   Sensitivity vs SetRxSensitivity, 2.4 GHz, RXB
%   305.3   Sensitivity vs SetRxSensitivity, 2.4 GHz, RXA
%   306.1   CCK-ACR vs SGAIBIASTRIM
%   307.1   RF22 TX Power Control
%
%   ------- PASS/FAIL ON MODE-SWITCH TIMING -------------------------------------
%   400.1   Standby-to-RX, OFDM in 802.11g
%   400.2   Standby-to-RX, CCK in 802.11g
%   400.3   Standby-to-RX, OFDM in 802.11a
%   400.4   Standby-to-RX, CCK in 802.11b
%   401.1   TX-to-RX, OFDM, 802.11g
%   401.2   TX-to-RX, OFDM, 802.11a
%   401.3   TX-to-RX, CCK, 802.11g
%   401.4   TX-to-RX, CCK, 802.11b
%   401.5   TX-to-RX, HT-OFDM, 802.11n
%   402.1   TX-OFDM to RX-CCK in 8 usec
%   402.2   TX-OFDM to RX-CCK in 12 usec
%   402.3   TX-OFDM to RX-CCK in 50 usec
%   403.1   RX-CCK, TX-ACK(1Mbps), RX-OFDM in 16 usec
%   403.2   RX-CCK, TX-ACK(1Mbps), RX-OFDM in 34 usec
%   403.3   RX-OFDM, TX-ACK(6Mbps), RX-CCK in 8 usec

% Written by Barrett Brickner
% Copyright 2008-2011 DSP Group, Inc., All Rights Reserved.

%% Create generic data structure
data.TestID = TestID;
data.Timestamp = datestr(now);
data.Result = 1;

if isnumeric(TestID),
    TestID = sprintf('%g',TestID);
end

if nargin<2, 
    opt = []; 
else
    data.opt = opt;
end

%% Determine Baseband Capability
DwPhyLab_Setup;
BasebandID = DwPhyLab_ReadRegister(1);
if BasebandID > 6,
    MbpsOFDM = [6 9 12 18 24 36 48 54 6.5 13 19.5 26 39 52 58.5 65];
    Mbps72Str = 'Mbps = 65';
    MaxRxModeStr = 'RxMode = 7';
else
    MbpsOFDM = [6 9 12 18 24 36 48 54];
    Mbps72Str = 'Mbps = 72';
    MaxRxModeStr = 'RxMode = 3';
end

%% Generic Single Channel List for Single/Dual-Band
if ~DwPhyLab_Parameters('DualBand'),
    fcMHzList = 2412;
else
    fcMHzList = [2412 5200];
end

%% PrdBm List
MidRangePrdBmS = 'PrdBm = -60:-30'; % range for expected low PER operation

%% TXPWRLVL List
if str2double(TestID) >= 200 && str2double(TestID) < 300,

    if any(strcmpi(DwPhyLab_Parameters('DwPhyTestExceptions'),'RF52B1'))
        ListTXPWRLVLs  = 'TXPWRLVL = 1:2:63';
        ListTXPWRLVL1  = 23;
        ListTXPWRLVL4  = [1 19 37 55];
        ListTXPWRLVL8  = [1 9 19 27 37 45 55 63];
        ListTXPWRLVL8s = 'TXPWRLVL = [1 9 19 27 37 45 55 63]';
        ListTXPWRLVL16s= 'TXPWRLVL = 3:4:63';
    elseif any(strcmpi(DwPhyLab_Parameters('DwPhyTestExceptions'),'RF52A'))
        ListTXPWRLVLs  = 'TXPWRLVL = 0:63';
        ListTXPWRLVL1  = 23;
        ListTXPWRLVL4  = [0 18 36 54];
        ListTXPWRLVL8  = [0 9 18 27 36 45 54 63];
        ListTXPWRLVL8s = 'TXPWRLVL = [0 9 18 27 36 45 54 63]';
        ListTXPWRLVL16s= 'TXPWRLVL = 2:4:63';
    elseif any(strcmpi(DwPhyLab_Parameters('DwPhyTestExceptions'),'RF22A0'))
        ListTXPWRLVLs  = 'TXPWRLVL = 49:63';
        ListTXPWRLVL1  = 49;
        ListTXPWRLVL4  = [49 53 57 63];
        ListTXPWRLVL8  = [49 51 53 55 57 59 61 63];
        ListTXPWRLVL8s = 'TXPWRLVL = [49 51 53 55 57 59 61 63]';
        ListTXPWRLVL16s= 'TXPWRLVL = 49:1:63';            
    else
        ListTXPWRLVLs  = 'TXPWRLVL = 32:63';
        ListTXPWRLVL1  = 43;
        ListTXPWRLVL4  = [43 51 57 63];
        ListTXPWRLVL8  = [39:4:59 61 63]; % [110429 changed from 35:4:63
        ListTXPWRLVL8s = 'TXPWRLVL = [39:4:59 61 63]'; % [110429 'TXPWRLVL = [35:4:63]'
        ListTXPWRLVL16s= 'TXPWRLVL = 33:2:63';%'TXPWRLVL = 3:4:63'
    end
end

%% SetRxSensitivity
if isempty(DwPhyLab_Parameters('SetRxSensitivityDuringTx'))
    SetRxSensitivityString = 'SetRxSensitivity = -50';
else
    SetRxSensitivityString = sprintf('SetRxSensitivity = %d', DwPhyLab_Parameters('SetRxSensitivityDuringTx'));    
end

%% Execute/Dispatch Test
try
    switch upper(TestID),

        %%% DEVICE/DRIVER PROGRAMMING REGRESSION %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %%% -------------------------------------------------------------------------
        case '1',   data = DwPhyTest_BasebandReset;
        case '2',   data = RunSet(data, [2.1 2.2], opt);
        case '2.1', data = DwPhyTest_BasebandRegisters;
        case '2.2', data = DwPhyTest_RadioSerialRegisters;
        case '3',   data = DwPhyTest_BasicIO;
        case '4',   data = DwPhyTest_PllLock;
        case '4.1', data = DwPhyTest_PllLock(1);
        case '5',   data = DwPhyTest_SetParameterData;

        %%% Sensitivity/MaxInput %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        case '10', data = RunSet(data, [10.1 10.2 10.4], opt);

        case '10.1', 
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 54','PrdBm = -65','PERLimit = 0.10',...
                'MaxPkts = 1000','fcMHz = DwPhyTest_ChannelList', opt);

        case '10.2', 
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 11','PrdBm = -76','PERLimit = 0.08',...
                'MaxPkts = 1000','fcMHz = [2412:5:2472, 2484]', opt);

        case '10.3', 
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 54','PrdBm = -65','PERLimit = 0.10',...
                'MaxPkts = 1000','fcMHz = [2412:5:2472, 2484]', opt);

        case '10.4', % TX-to-RX, OFDM, All Channels
            data = DwPhyTest_PERvChanl('fcMHz=DwPhyTest_ChannelList','PrdBm=-50', ...
                'RxMode=1','Mbps=54','T_IFS=76e-6','Broadcast=0',...
                'MaxPkts=1000', opt...
                );
            data.Result = all(data.PER < 0.01);
            
        case '11', data = RunSet(data, [11.1, 11.2, 11.3], opt);

        case '11.1', 
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 54','PrdBm = -20','PERLimit = 0.10',...
                'MaxPkts = 1000','fcMHz = [2412:5:2472, 2484]', opt);

        case '11.2',
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 54','PrdBm = -30','PERLimit = 0.10','MaxPkts = 1000',...
                'fcMHz = DwPhyTest_ChannelList; fcMHz = fcMHz(find(fcMHz>4000));', opt);

        case '11.3', 
            data = DwPhyTest_PERvChanl( ...
                'Mbps = 11','PrdBm = -20','PERLimit = 0.08','MaxPkts = 1000',...
                'fcMHz = [2412:5:2472, 2484]', opt);
            
        case '12', data = RunSet(data, [12.1 12.2], opt);

        case '12.1',
            data.Mbps = MbpsOFDM;
            for i=1:length(data.Mbps),
                result = DwPhyTest_PERvPrdBm( ...
                    sprintf('Mbps = %g',data.Mbps(i)), ...
                    sprintf('LengthPSDU = %d', 101*i), ...
                    'fcMHz = 2412','MaxPkts = 1000','PrdBm = -60', opt );
                if result.Result == -1, data.Result = -1; break; end
                data.PER(i) = result.PER;
            end
            data.Result = all(data.PER < 0.01);
            
        case '12.2',
            data.Mbps          = [1 2 2 5.5 5.5 11 11];
            data.ShortPreamble = [0 0 1   0   1  0  1];
            for i=1:length(data.Mbps),
                result = DwPhyTest_PERvPrdBm( ...
                    sprintf('Mbps = %g',data.Mbps(i)), ...
                    sprintf('LengthPSDU = %d', 100+i), ...
                    sprintf('ShortPreamble = %d',data.ShortPreamble(i)), ...
                    'fcMHz = 2412','MaxPkts = 1000','PrdBm = -60', opt );
                if result.Result == -1, data.Result = -1; break; end
                data.PER(i) = result.PER;
            end
            data.Result = all(data.PER < 0.01);
            
        %%% Carrier Frequency Offset %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        case '13', data = RunSet(data, [13.1 13.2 13.3 13.4], opt);

        case '13.1',
            data = DwPhyTest_MeasureRxCFO(opt);
            data = rmfield(data, 'RegList');
            if isfield(data,'CFOppm')
                DwPhyLab_Parameters('CFOppm',data.CFOppm);
            end
            
        case '13.2',
            fcMHz = 5805;
            CFOppm = DwPhyLab_Parameters('CFOppm');
            if isempty(CFOppm), CFOppm = 0; end;
            data = DwPhyTest_PERvChanl( 'Mbps = 54','MaxPkts = 2000','PrdBm = -60', ...
                sprintf('fcMHz = [%g %g]',fcMHz, fcMHz),...
                sprintf('fcMHzOfs = %d*[%1.6f %1.6f]/1e6',fcMHz, -40-CFOppm, 40-CFOppm), opt );
            data.Result = all(data.PER) < 0.01;

        case '13.3',
            fcMHz = 2484;
            CFOppm = DwPhyLab_Parameters('CFOppm');
            if isempty(CFOppm), CFOppm = 0; end;
            data = DwPhyTest_PERvChanl( 'Mbps = 11','MaxPkts = 2000','PrdBm = -60', ...
                sprintf('fcMHz = [%g %g]',fcMHz, fcMHz),...
                sprintf('fcMHzOfs = %d*[%1.6f %1.6f]/1e6',fcMHz, -50-CFOppm, 50-CFOppm), opt );
            data.Result = all(data.PER) < 0.01;

        case '13.4',
            fcMHz = 2484;
            CFOppm = DwPhyLab_Parameters('CFOppm');
            if isempty(CFOppm), CFOppm = 0; end;
            data = DwPhyTest_PERvChanl( 'Mbps = 2','LengthPSDU=200','MaxPkts = 2000','PrdBm = -60', ...
                sprintf('fcMHz = [%g %g]',fcMHz, fcMHz),...
                sprintf('fcMHzOfs = %d*[%1.6f %1.6f]/1e6',fcMHz, -50-CFOppm, 50-CFOppm), opt );
            data.Result = all(data.PER) < 0.01;

        case '13.5',
            fcMHz = 2484;
            CFOppm = DwPhyLab_Parameters('CFOppm');
            if isempty(CFOppm), CFOppm = 0; end;
            data = DwPhyTest_PERvChanl( 'Mbps = 54','MaxPkts = 4000','PrdBm = -60', ...
                sprintf('fcMHz = [%g %g]',fcMHz, fcMHz),...
                sprintf('fcMHzOfs = %d*[%1.6f %1.6f]/1e6',fcMHz, -50-CFOppm, 50-CFOppm), opt );
            data.Result = all(data.PER) < 0.01;

        %%% DwPhy_DiversityMode() %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        case '14', data = RunSet(data, [14.1 14.2 14.3], opt);
            
        case '14.1',
            data = DwPhyTest_PERvPrdBm( 'DiversityMode = 1',...
                'Mbps = 36','PrdBm = -60','MaxPkts = 1000','fcMHz=2412', opt );
            if DwPhyLab_RadioIsRF22, data.Result = (data.PER < 0.01);
            else                     data.Result = (data.PER < 0.01) & (data.RegList(3,2) == 1);
            end
        
        case '14.2',
            data = DwPhyTest_PERvPrdBm( 'DiversityMode = 2',...
                'Mbps = 36','PrdBm = -60','MaxPkts = 1000','fcMHz=2412', opt );
            if DwPhyLab_RadioIsRF22, data.Result = (data.PER < 0.01);
            else                     data.Result = (data.PER < 0.01) & (data.RegList(3,2) == 2);
            end

        case '14.3',
            data = DwPhyTest_PERvPrdBm( 'DiversityMode = 3',...
                'Mbps = 36','PrdBm = -60','MaxPkts = 1000','fcMHz=2412', opt );
            if DwPhyLab_RadioIsRF22, data.Result = (data.PER < 0.01);
            else                     data.Result = (data.PER < 0.01) & (data.RegList(3,2) == 3);
            end

        %%% DwPhy_SetRxMode() %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        case '15', data = RunSet(data, [15.1 15.2 15.3 15.4 15.5], opt);

        case '15.1',
            ArgList = {'RxMode=1','fcMHz=5200','PrdBm=-60','LengthPSDU=100','MaxPkts=1000'};
            result11 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 11',opt); data.PER11 = result11.PER;
            result36 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 36',opt); data.PER36 = result36.PER;
            data.Result = (data.PER11 == 1) && (data.PER36 < 0.01);
            
        case '15.2',
            ArgList = {'RxMode=2','fcMHz=2412','PrdBm=-60','LengthPSDU=100','MaxPkts=1000'};
            result11 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 11',opt); data.PER11 = result11.PER;
            result36 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 36',opt); data.PER36 = result36.PER;
            data.Result = (data.PER11 < 0.01) && (data.PER36 == 1);
            
        case '15.3',
            ArgList = {'RxMode=3','fcMHz=2412','PrdBm=-60','LengthPSDU=100','MaxPkts=1000'};
            result11 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 11',opt); data.PER11 = result11.PER;
            result36 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 36',opt); data.PER36 = result36.PER;
            data.Result = (data.PER11 < 0.01) && (data.PER36 < 0.01);
    
        case '15.4',
            ArgList = {'RxMode=5','fcMHz=5200','PrdBm=-60','LengthPSDU=100','MaxPkts=1000'};
            result11 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 11',opt); data.PER11 = result11.PER;
            result36 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 36',opt); data.PER36 = result36.PER;
            result39 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 39',opt); data.PER39 = result39.PER;
            data.Result = (data.PER11 == 1) && (data.PER36 < 0.01) && (data.PER39 < 0.01);
            
        case '15.5',
            ArgList = {'RxMode=5','fcMHz=2412','PrdBm=-60','LengthPSDU=100','MaxPkts=1000'};
            result11 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 11',opt); data.PER11 = result11.PER;
            result36 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 36',opt); data.PER36 = result36.PER;
            result39 = DwPhyTest_PERvPrdBm(ArgList,'Mbps = 39',opt); data.PER39 = result39.PER;
            data.Result = (data.PER11 < 0.01) && (data.PER36 < 0.01) && (data.PER39 < 0.01);

        %%% AGC / PER Across Input Power %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        case '16', data = RunSet(data, [16.1, 16.2, 16.3],opt);
            
        case '16.1',
            data = DwPhyTest_PERvPrdBm( 'fcMHz = 5200', ...
                'Mbps = 54','LengthPSDU = 500','PrdBm = -62:-30','MaxPkts = 1000',opt);
            if data.Result >= 0,
                data.Result = all(data.PER <= 0.01);
            end
            
        case '16.2',
            data = DwPhyTest_PERvPrdBm( 'fcMHz = 2412', ...
                'Mbps = 54','LengthPSDU = 500','PrdBm = -62:-20','MaxPkts = 1000',opt);
            if data.Result >= 0,
                data.Result = all(data.PER <= 0.01);
            end
            
        case '16.3',
            data = DwPhyTest_PERvPrdBm( 'fcMHz = 2412', ...
                'Mbps = 11','LengthPSDU = 100','PrdBm = -70:-20','MaxPkts = 1000',opt);
            if data.Result >= 0,
                data.Result = all(data.PER <= 0.01);
            end
        
        %%% Nuisance Packet Filtering and Restart
        case '17', data = RunSet(data, 17.1, opt);
            
        case '17.1',
            param = DwPhyLab_Parameters;
            StationAddress = sprintf('StationAddress = ''%s''',param.STA.WiFiMAC);
            data.desired = DwPhyTest_PERvPrdBm( 'fcMHz = 2412','PrdBm = -60',...
                'Mbps = 24','LengthPSDU = 100',...
                'Broadcast = 0', StationAddress, 'AddressFilter = 1', opt);

            StationAddress = 'StationAddress = ''001100110011''';
            data.nuisance = DwPhyTest_PERvPrdBm( 'fcMHz = 2412','PrdBm = -60',...
                'Mbps = 24','LengthPSDU = 100',...
                'Broadcast = 0', StationAddress, 'AddressFilter = 1', opt);

            data.unfiltered = DwPhyTest_PERvPrdBm( 'fcMHz = 2412','PrdBm = -60',...
                'Mbps = 24','LengthPSDU = 100',...
                'Broadcast = 0', StationAddress, 'AddressFilter = 0', opt);
            
            data.Result = (data.desired.PER==0) && (data.nuisance.PER==1) && (data.unfiltered.PER==0);
            
        %%% Sensitivity Limits
        case '18', data = RunSet(data, [18.1, 18.2], opt);
            
        case '18.1',
            data.result90 = DwPhyTest_Sensitivity('fcMHz = 2412','PrdBm = -95:-50', ...
                'Mbps = 6','LengthPSDU = 100','MaxPkts = 1000','MinPER = 0.05', ...
                'SetRxSensitivity = -90', opt);
            data.result70 = DwPhyTest_Sensitivity('fcMHz = 2412','PrdBm = -95:-50', ...
                'Mbps = 6','LengthPSDU = 100','MaxPkts = 1000','MinPER = 0.05', ...
                'SetRxSensitivity = -70', opt);
            data.Result = (data.result90.Sensitivity < -82) ...
                       && (data.result70.Sensitivity > -75);
                   
        %%% Miscellaneous Radio Tests
        case '19.1',
            data = DwPhyTest_RxIMR('fcMHz = [2412 5200]', opt);

        case '19.2',
            data = DwPhyTest_RxIMR('fcMHz = [2412]', opt);

        case '20.1', % ADC output offset at 2484 MHz
            data = DwPhyTest_MeasureRxOffset('fcMHz=2484','Npts=100');
            
        case '21.1', % DMW96 I/Q Calibration, 2.4 GHz channels, ~1 min
            data = DwPhyTest_IQCalibration(opt);
            
        case '22.1', % RIFS
            data = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                'PrdBm = -60:10:-40','MinFail = 1000','MaxPkts = 10000', ...
                'PktsPerWaveform = 1', 'T_IFS = 2e-6', opt);
            
        %%% CHARACTERIZATION TESTS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %%% -------------------------------------------------------------------------
        case '100', data = RunSet(data, [100.1, 100.2, 100.3 100.4], opt);
        
        case '100.1', % L-OFDM Sensitivity at 5 GHz, ~5 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 5200', ...
                'MinFail = 1000','MaxPkts = 10000',  'Mbps = [6 9 12 18 24 36 48 54]', opt);

        case '100.2', % L-OFDM Sensitivity at 2.4 GHz, ~5 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', ...
                'MinFail = 1000','MaxPkts = 10000',  'Mbps = [6 9 12 18 24 36 48 54]', opt);

        case '100.3', % DSSS/CCK Sensitivity at 2.4 GHz, ~9 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [1 2 5.5 11]', opt);

        case '100.4', % L-OFDM Sensitivity at 5 GHz, RXA, ~6 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 5200','DiversityMode = 1', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [6 9 12 18 24 36 48 54]', opt);

        case '100.5', % L-OFDM Sensitivity at 2.4 GHz, RXA, ~6 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412','DiversityMode = 1', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [6 9 12 18 24 36 48 54]', opt);

        case '100.6', % DSSS/CCK Sensitivity at 2.4 GHz, RXB, ~9 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', 'DiversityMode = 2', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [1 2 5.5 11]', opt);

        case '100.7', % L-OFDM Sensitivity at 2.4 GHz, RXB, ~6 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412','DiversityMode = 2', ...
                'MinFail = 1000','MaxPkts = 10000',  'Mbps = [6 9 12 18 24 36 48 54]', opt);

        case '100.11', % HT-OFDM Sensitivity at 2.4 GHz, RXA, ~5 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', 'DiversityMode = 1', 'LengthPSDU = 4095', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [6.5 13 19.5 26 39 52 58.5 65]', opt);

        case '100.12', % HT-OFDM Sensitivity at 2.4 GHz, RXB, ~5 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', 'DiversityMode = 2','LengthPSDU = 4095', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [6.5 13 19.5 26 39 52 58.5 65]', opt);

        case '100.13', % HT-OFDM Sensitivity at 2.4 GHz, MRC, ~5 minutes
            data = DwPhyTest_Sensitivity('fcMHz = 2412', 'DiversityMode = 3','LengthPSDU = 4095', ...
                'MinFail = 1000','MaxPkts = 10000', 'Mbps = [6.5 13 19.5 26 39 52 58.5 65]', opt);

        case '101', data = RunSet(data, [101.1, 101.2, 101.3 101.4], opt);
            
        case '101.1', % Max Input, 54 Mbps at 5200 MHz
            data = DwPhyTest_Sensitivity('Mbps = [54 36]', 'fcMHz = 5200', ...
                'PrdBm = 0:-1:-40','MinFail = 1000','MaxPkts = 10000', opt);

        case '101.2', % Max Input, 54 Mbps at 2412 MHz
            data = DwPhyTest_Sensitivity('Mbps = [54 36 12]', 'fcMHz = 2412', ...
                'PrdBm = 0:-1:-40','MinFail = 1000','MaxPkts = 10000', opt);

        case '101.3', % Max Input, 11 Mbps at 2484 MHz
            result = DwPhyTest_MeasureRxCFO(opt);
            if isfield(result,'CFOppm')
                DwPhyLab_Parameters('CFOppm',result.CFOppm);
                data = DwPhyTest_Sensitivity('Mbps = 11', 'fcMHz = 2484', ...
                    sprintf('fcMHzOfs = %1.6f',2484 * (50-result.CFOppm)/1e6), ...
                    'PrdBm = 6:-1:-40','MinFail = 1000','MaxPkts = 10000', opt);
                data.CFOppm = result.CFOppm;
            end

        case '101.4', % Max Input, 54 Mbps at 5200 MHz, RXA
            data = DwPhyTest_Sensitivity('Mbps = [54 36]', 'fcMHz = 5200', ...
                'PrdBm = 0:-1:-40','MinFail = 1000','MaxPkts = 10000','DiversityMode = 1', opt);

        case '101.5', % Max Input, 54 Mbps at 2412 MHz, RXA
            data = DwPhyTest_Sensitivity('Mbps = [54 36]', 'fcMHz = 2412', ...
                'PrdBm = 0:-1:-40','MinFail = 1000','MaxPkts = 10000','DiversityMode = 1', opt);

        case '101.6'  % Max Input, 54 Mbps at 2.4 GHz band
            fcMHz = DwPhyTest_ChannelList;
            data.fcMHz = fcMHz(fcMHz < 3000);
            for i=1:length(data.fcMHz)
                data.result{i} = DwPhyTest_Sensitivity('Mbps = 54', sprintf('fcMHz = %g',data.fcMHz(i)), ...
                'PrdBm = 0:-1:-40','MinFail = 1000','MaxPkts = 10000', opt);
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end
            
        case '102', data = RunSet(data, [102.1, 102.2, 102.3], opt);
            
        case '102.1', % Full Range, 54 Mbps, 5200 MHz, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 5200', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10', opt);
            
        case '102.2', % Full Range, 54 Mbps, 2412 MHz, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10', opt);
            
        case '102.3', % Full Range, 11 Mbps, 2412 MHz, ~15 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 11','fcMHz = 2412', ...
                'PrdBm = -99:3','MinFail = 1000','MaxPkts = 10000', opt);
            
        case '102.4', % Full Range, 65/72 Mbps, 5200
            data = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 5200', ...
                'PrdBm = -80:0','MinFail = 1000','MaxPkts = 10000',...
                'PktsPerWaveform = 10','UserReg = {{25,1,0}}', opt);
            
        case '102.5', % Full Range, 65/72 Mbps, 2412
            data = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 2412', ...
                'PrdBm = -80:0','MinFail = 1000','MaxPkts = 10000',...
                'PktsPerWaveform = 10','UserReg = {{25,1,0}}', opt);
            
        case '102.6', % Full Range, 54 Mbps, 5200 MHz, RXA, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 5200', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 1', opt);

        case '102.7', % Full Range, 54 Mbps, 5200 MHz, RXB, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 5200', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 2', opt);

        case '102.8', % Full Range, 54 Mbps, 2412 MHz, RXA, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 1', opt);

        case '102.9', % Full Range, 54 Mbps, 2412 MHz, RXB, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 2', opt);
            
        case '102.10', % Full Range, 72 Mbps, 2412
            data = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 2412', ...
                'PrdBm = -80:0','MinFail = 1000','MaxPkts = 10000',...
                'PktsPerWaveform = 10','UserReg = {{25,1,0},{144,''2:0'',1}}', opt);
            
        case '102.11', % Full Range, 65 Mbps, 2412 MHz, MRC, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 3', opt);
            
        case '102.12', % Full Range, 65 Mbps, 2412 MHz, RXA, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 1', opt);

        case '102.13', % Full Range, 65 Mbps, 2412 MHz, RXB, ~7 minutes
            data = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                'PktsPerWaveform = 10','DiversityMode = 2', opt);

        case '102.14', % Full Range, 54 Mbps, 2484 MHz, RXA, Vary CFO
            result = DwPhyTest_MeasureRxCFO(opt);
            if isfield(result,'CFOppm')
                DwPhyLab_Parameters('CFOppm',result.CFOppm);
                data.CFOppmRange = 2*[5 10 15 20 25];
                for i=1:length(data.CFOppmRange),
                    data.result{i} = DwPhyTest_PERvPrdBm('Mbps = 54','fcMHz = 2484', ...
                                  'PrdBm = -90:0','MinFail = 1000','MaxPkts = 20000', ...
                                  'PktsPerWaveform = 10','DiversityMode = 1', ...
                                  sprintf('fcMHzOfs = fcMHz*%1.6f/1e6', data.CFOppmRange(i) - result.CFOppm), ...
                                  opt);
                end
                data.CFOppm = result.CFOppm;
            end
            
           
        case '102.21', % Full Range, 65 Mbps, All Channels, DiversityMode = 1
            data.fcMHz = DwPhyTest_ChannelList;
            data.PrdBm = -80:0;
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_PERvPrdBm('Mbps = 65','DiversityMode=1', ...
                    sprintf('fcMHz = %g',data.fcMHz(i)), 'MinFail = 500','MaxPkts = 1000', ...
                    sprintf('PrdBm = %d:%d',min(data.PrdBm),max(data.PrdBm)), ...
                    'PktsPerWaveform = 10', opt );
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end
            
        case '102.22', % Full Range, 65 Mbps, All Channels, DiversityMode = 2
            data.fcMHz = DwPhyTest_ChannelList;
            data.PrdBm = -80:0;
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_PERvPrdBm('Mbps = 65','DiversityMode=2', ...
                    sprintf('fcMHz = %g',data.fcMHz(i)), 'MinFail = 500','MaxPkts = 1000', ...
                    sprintf('PrdBm = %d:%d',min(data.PrdBm),max(data.PrdBm)), ...
                    'PktsPerWaveform = 10', opt );
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end
            
            
        case '103', data = RunSet(data, [103.1, 103.2, 103.3, 103.4 103.5], opt);

        case '103.1', % 54 Mbps Sensitivity, All Channels, RXA ~6 minutes
            data = DwPhyTest_SensitivityVsChanl('Mbps = 54', ...
                'PrdBm = -90:-60','MinFail = 1000','MaxPkts = 10000','MinPER = 0.01',...
                'PktsPerWaveform = 10','DiversityMode = 1', opt);
            
        case '103.2', % 54 Mbps Sensitivity, All Channels, RXB ~6 minutes
            data = DwPhyTest_SensitivityVsChanl('Mbps = 54', ...
                'PrdBm = -90:-60','MinFail = 1000','MaxPkts = 10000','MinPER = 0.01',...
                'PktsPerWaveform = 10','DiversityMode = 2', opt);
            
        case '103.3', % 54 Mbps Sensitivity, All Channels, ~6 minutes
            data = DwPhyTest_SensitivityVsChanl('Mbps = 54', ...
                'PrdBm = -90:-60','MinFail = 1000','MaxPkts = 10000','MinPER = 0.01',...
                'PktsPerWaveform = 10', opt);
            
        case '103.4', % 11 Mbps Sensitivity, 2.4 GHz, ~2 minutes
            data = DwPhyTest_SensitivityVsChanl('Mbps = 11',...
                'fcMHz = DwPhyTest_ChannelList; fcMHz = fcMHz(fcMHz < 3000);',...
                'PrdBm = -95:-60','MinFail = 1000','MaxPkts = 10000','MinPER = 0.01', opt);
            
        case '103.5', % 6 Mbps Sensitivity, All Channels, ~10 minutes
            data = DwPhyTest_SensitivityVsChanl('Mbps = 6', ...
                'PrdBm = -96:-60','MinFail = 200','MaxPkts = 1000','MinFail = 0.01', opt);
            
        case '103.6', % 65/72 Mbps Sensitivity, All Channels, ~6 minutes
            data = DwPhyTest_SensitivityVsChanl(Mbps72Str, ...
                'PrdBm = -80:-55','MinFail = 1000','MaxPkts = 10000','MinPER = 0.05',...
                'PktsPerWaveform = 10','UserReg = {{25,1,0}}', opt);

        case '104', data = RunSet(data, [104.1 104.2 104.3], opt);
            
        case '104.1', % Interference Reject, 54 Mbps, 5 GHz Band
            fcMHz = DwPhyTest_ChannelList;
            data.fcMHz = fcMHz(fcMHz > 3000);
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_InterferenceRejection('Mbps = 54',...
                    sprintf('fcMHz = %g',data.fcMHz(i)), opt );
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end
                
        case '104.2', % Interference Reject, 54 Mbps, OFDM, 2.4 GHz Band
            fcMHz = DwPhyTest_ChannelList;
            data.fcMHz = fcMHz(fcMHz < 3000);
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_InterferenceRejection('Mbps = 54',...
                    sprintf('fcMHz = %g',data.fcMHz(i)), 'fiMHz = fcMHz + [-25 25]', opt);
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end
                
        case '104.3', % Interference Reject, 11 Mbps, 2.4 GHz Band
            fcMHz = DwPhyTest_ChannelList;
            data.fcMHz = fcMHz(fcMHz < 3000);
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_InterferenceRejection('Mbps = 11',...
                    'PrdBm = -70','PidBm = -25:-1:-45','InterferenceType = ''ACI-CCK''',...
                    sprintf('fcMHz = %g',data.fcMHz(i)), 'fiMHz = fcMHz + [-25 25]', opt);
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end

        case '104.4', % Interference Reject, 54 Mbps, OFDM, DECT, ~21 minutes
            if DwPhyLab_Parameters('DualBand') == 0,
                data.fcMHz = 2412;
            else
                data.fcMHz =  [2412 5200 5805];
            end
            for i=1:length(data.fcMHz),
                data.result{i} = DwPhyTest_InterferenceRejection( 'Mbps = 54', ...
                    sprintf('fcMHz = %d',data.fcMHz(i) ), ...
                    'InterferenceType = ''DECT''','PrdBm = -62','PidBm = 0:-1:-40', opt);
                if data.result{i}.Result == -1, break; end
            end
            
        case '104.5', % OFDM ACR: All 2.4 GHz Adjacent Channels above 2412 MHz
            fcMHz = DwPhyTest_ChannelList;
            fcMHz = fcMHz(fcMHz < 3000); % All 2.4 GHz Channels
            data.fiMHz = fcMHz; % all adjacent higher channels
            fcMHz = fcMHz(1);
            for i=1:length(data.fiMHz),
                data.result{i} = DwPhyTest_InterferenceRejection( 'Mbps = 54', ...
                    sprintf('fcMHz = %g',fcMHz),sprintf('fiMHz = %g',data.fiMHz(i)), ...
                    'PidBm = -30:-1:-90', opt );
                data.PidBm0(i) = data.result{i}.PidBm0;
                data.Rejection_dB(i) = data.result{i}.Rejection_dB;
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end

        case '104.6', % OFDM ACR: All 2.4 GHz Adjacent Channels below 2484 MHz
            fcMHz = DwPhyTest_ChannelList;
            fcMHz = fcMHz(fcMHz < 3000); % All 2.4 GHz Channels
            data.fiMHz = fcMHz(1:(length(fcMHz)-1)); % all adjacent higher channels
            fcMHz = fcMHz(length(fcMHz));
            for i=1:length(data.fiMHz),
                data.result{i} = DwPhyTest_InterferenceRejection( 'Mbps = 54', ...
                    sprintf('fcMHz = %g',fcMHz),sprintf('fiMHz = %g',data.fiMHz(i)), ...
                    'PidBm = -30:-1:-85', opt );
                data.PidBm0(i) = data.result{i}.PidBm0;
                data.Rejection_dB(i) = data.result{i}.Rejection_dB;
                if data.result{i}.Result ==  0, data.Result = 0; end
                if data.result{i}.Result == -1, break; end
            end

        case '105', data = RunSet(data, [105.1 105.2 105.3 105.4], opt);

        case '105.1', % Tone Blocking, 54 Mbps, ~1.2 hrs
            data = DwPhyTest_Blocking('fcMHz = 5200','Mbps = 54','PrdBm = -62', opt);
            
        case '105.2', % Tone Blocking, 54 Mbps, ~1.2 hrs
            data = DwPhyTest_Blocking('fcMHz = 2412','Mbps = 54','PrdBm = -62', opt);
            
        case '105.3', % Tone Blocking, 12 Mbps, ~3.1 hrs
            data = DwPhyTest_Blocking('fcMHz = 5200','Mbps = 12','PrdBm = -76', opt);
            
        case '105.4', % Tone Blocking, 12 Mbps, ~3.1 hrs
            data = DwPhyTest_Blocking('fcMHz = 2412','Mbps = 12','PrdBm = -76', opt);
            
        case '105.5', % DECT Blocking, 54 Mbps, 5200 MHz
            data = DwPhyTest_InterferenceRejection('Mbps = 54','PrdBm = -62', ...
                        'InterferenceType = ''DECT''','PidBm = 0:-1:-50',...
                        'fcMHz = 5200','MaxPkts = 1000', opt);

        case '105.6', % DECT Blocking, 54 Mbps, 2412 MHz
            data = DwPhyTest_InterferenceRejection('Mbps = 54','PrdBm = -62', ...
                        'InterferenceType = ''DECT''','PidBm = 0:-1:-50',...
                        'fcMHz = 2412','MaxPkts = 1000', opt);

        case '105.21', % Tone Blocking, 54 Mbps, LPF, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -62', ...
                'fiMHz = 1:1800','Filter = ''VLFX-1125''','PidBmMax = 10', opt);
            
        case '105.22', % Tone Blocking, 54 Mbps, HPF, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -62', ...
                'fiMHz = 3000:6000','Filter = ''VHF-3100P''','PidBmMax = 3', opt);
                    
        case '105.23', % Tone Blocking, 54 Mbps, No Filter, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -62', ...
                'fiMHz = 1600:3200', 'PidBmMax = 6', 'PidBmMin = -90', opt);
            
        case '105.24', % Tone Blocking, 54 Mbps, LPF, 2437 MHz
            X{1} = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -62', ...
                     'fiMHz = 1800:1899','Filter = ''VLFX-1125''','PidBmMax = 3', opt);
            X{2} = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -62', ...
                     'fiMHz = 1900:1950','Filter = ''VLFX-1125''','PidBmMax = 0', opt);
            data.result = X;
            data.fiMHz = [X{1}.fiMHz X{2}.fiMHz];
            data.PidBm = [X{1}.PidBm X{2}.PidBm];
            
        case '105.31', % Tone Blocking, 54 Mbps, LPF, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -69', ...
                'fiMHz = 1:1800','Filter = ''VLFX-1125''','PidBmMax = 10', opt);
            
        case '105.32', % Tone Blocking, 54 Mbps, HPF, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -69', ...
                'fiMHz = 3000:6000','Filter = ''VHF-3100P''','PidBmMax = 3', opt);
                    
        case '105.33', % Tone Blocking, 54 Mbps, No Filter, 2437 MHz
            data = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -69', ...
                'fiMHz = 1600:3200', 'PidBmMax = 6', 'PidBmMin = -90', opt);
            
        case '105.34', % Tone Blocking, 54 Mbps, LPF, 2437 MHz
            X{1} = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -69', ...
                     'fiMHz = 1800:1899','Filter = ''VLFX-1125''','PidBmMax = 3', opt);
            X{2} = DwPhyTest_Blocking('fcMHz = 2437','Mbps = 54','PrdBm = -69', ...
                     'fiMHz = 1900:1950','Filter = ''VLFX-1125''','PidBmMax = 0', opt);
            data.result = X;
            data.fiMHz = [X{1}.fiMHz X{2}.fiMHz];
            data.PidBm = [X{1}.PidBm X{2}.PidBm];
        case '106.1', % Sensitivity, 54 Mbps, RX-TX-RX, All Channels
            data = DwPhyTest_SensitivityVsChanl('Mbps = 54', 'Broadcast = 0', ...
                'PrdBm = -90:-60','MinFail = 1000','MaxPkts = 10000','MinPER = 0.01',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 75.1e-6', 'PktsPerWaveform = 10', opt);
                    
        case '106.2', % Sensitivity, 54 Mbps, RX-TX-RX, 5200 MHz
            data = DwPhyTest_PERvPrdBm('Mbps = 54', 'Broadcast = 0','fcMHz = 5200', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 10000',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 75.1e-6', opt);
                    
        case '106.3', % Sensitivity, 54 Mbps, RX-TX-RX, 2412 MHz
            data = DwPhyTest_PERvPrdBm('Mbps = 54', 'Broadcast = 0','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 10000',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 75.1e-6', opt);
            
        case '106.4', % PER vs Chanl, 54 Mbps, RX-TX-RX, Sleep Between Chans
            DwPhyLab_Sleep; pause(10);
            data = DwPhyTest_PERvChanl('Mbps = 54', 'Broadcast = 0','PrdBm = -50',...
                'fcMHz = DwPhyTest_ChannelList','MinFail = 500','MaxPkts = 10000',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 75.1e-6','SleepWakeDelay = 5', opt);
            
        case '107.1', % Image Reject
            data = DwPhyTest_RxIMR('fcMHz = DwPhyTest_ChannelList', opt);

        case '108.1', % RSSI at 2442 MHz
            data = DwPhyTest_PERvPrdBm('fcMHz=2442','Mbps=6','LengthPSDU=50', ...
                'GetPacketRSSI=1','PrdBm=-85:0.5:-20', opt);
            data.Result = data.AccurateRSSI;
            
        case '109.1', % ADC output offset at 2484 MHz
            data = DwPhyTest_MeasureRxOffset('fcMHz=2484','Npts=1000','DCXShift=4', opt);
            
        case '110.1', % LgSigAFE Threshold at 2412 MHz
            data = DwPhyTest_PERvPrdBm('fcMHz=2412','Mbps=24','LengthPSDU=28',...
                'PrdBm=-50:0.5:-20', 'UserReg={{70,0},{80,''2:0'',7}}', opt);
            k = find(data.PER > 0.5, 1 );
            if length(k) == 1,
                data.ThLgSigAFE_dBm = data.PrdBm(k);
            else
                data.ThLigSigAFE_dBm = 0;
            end
            
        case '111',   data = RunSet(data, [111.1 111.2 111.3], opt);

        case '111.1', % Full Range, 65/72 Mbps, 2437 MHz, Variable DCXShift, RXA
            data.DCXShift = [0 1 2 3 4 5];
            for i = 1:length(data.DCXShift),
                data.result{i} = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 2437', ...
                    'DiversityMode=1','PrdBm = -80:1:-10','MinFail = 1000', ...
                    'MaxPkts = 10000','PktsPerWaveform=10',...
                    sprintf('UserReg = {{25,1,0},{144,''2:0'',%d}}',data.DCXShift(i)), opt);
                data.PrdBm = data.result{i}.PrdBm;
                data.PER(i,:) = data.result{i}.PER;
            end
            
        case '111.2', % Full Range, 65/72 Mbps, 2437 MHz, Variable DCXShift, RXB
            data.DCXShift = [0 1 2 3 4 5];
            for i = 1:length(data.DCXShift),
                data.result{i} = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 2437', ...
                    'DiversityMode=2','PrdBm = -80:1:-10','MinFail = 1000', ...
                    'MaxPkts = 10000','PktsPerWaveform=10',...
                    sprintf('UserReg = {{25,1,0},{144,''2:0'',%d}}',data.DCXShift(i)), opt);
                data.PrdBm = data.result{i}.PrdBm;
                data.PER(i,:) = data.result{i}.PER;
            end

        case '111.3', % Full Range, 65/72 Mbps, 2437 MHz, Variable DCXShift, MRC
            data.DCXShift = [0 1 2 3 4 5];
            for i = 1:length(data.DCXShift),
                data.result{i} = DwPhyTest_PERvPrdBm(Mbps72Str,'fcMHz = 2437', ...
                    'DiversityMode=3','PrdBm = -80:1:-10','MinFail = 1000', ...
                    'MaxPkts = 10000','PktsPerWaveform=10',...
                    sprintf('UserReg = {{25,1,0},{144,''2:0'',%d}}',data.DCXShift(i)), opt);
                data.PrdBm = data.result{i}.PrdBm;
                data.PER(i,:) = data.result{i}.PER;
            end

        case '112.1', % RX RIFS
            data = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                'PrdBm = -90:0','MinFail = 1000','MaxPkts = 10000', ...
                'PktsPerWaveform = 10', 'T_IFS = 2e-6', opt);
            
        case '113.1', % DMW96 RX I/Q-Calibration at 2442 MHz, ~15 min
            data = DwPhyTest_IQCalibrationRx(opt);
        
        case '114.1', % HT-STF
            data.GainStepdB = [-20 -16 -12 0 12 16 20];
            data.AttnRMS = 'no_AttnRMS';
            
            for i = 1:numel(data.GainStepdB),
                data.result{i} = DwPhyTest_PERvPrdBm('Mbps = 65','fcMHz = 2412', ...
                    'PrdBm = -90:0','MinFail = 1000','MaxPkts = 10000', ...
                    'PktsPerWaveform = 10','LoadPER.GainStep.t = (80*4+28)/80e6+28e-6', ...
                    ['LoadPER.GainStep.dB = ',num2str(data.GainStepdB(i))], ...
                    ['LoadPER.GainStep.AttnRMS = ','''',data.AttnRMS,''''], opt);
                
                % recover the rms level of the packet
                if data.GainStepdB(i)>0
                    if strcmpi(data.AttnRMS, 'no_AttnRMS');
                        data.result{i}.PrdBm = data.result{i}.PrdBm - data.GainStepdB(i); 
                    end 
                end
                
                data.PrdBm(:,i) = data.result{i}.PrdBm;
                data.PER(:,i) = data.result{i}.PER;
                data.LegendStr{i} = sprintf('%d dB',data.GainStepdB(i));
            end
            
        case '200', % TX Power Level across channels, ~3 min
            data = DwPhyTest_TxPower(ListTXPWRLVL8s,'fcMHz = DwPhyTest_ChannelList', ...
                SetRxSensitivityString, 'Average=1', opt);
        
        case '200.1', % TX Power Level 2.4 GHz band, ~3 min
            data = DwPhyTest_TxPower(ListTXPWRLVL8s, 'fcMHz = [2412:5:2472 2484]', ...
                SetRxSensitivityString, opt);
        
        case '200.2', % TX Power Level with TXPWRLVL = [63 59]
            data = DwPhyTest_TxPower('TXPWRLVL=[63 59]','fcMHz = DwPhyTest_ChannelList', ...
                SetRxSensitivityString, 'Average=10',opt);
        
        case '201.1', % TX Power Level at 2412 MHz, ~0.5 min
            data = DwPhyTest_TxPower(ListTXPWRLVLs,'fcMHz = 2412', ...
                SetRxSensitivityString, 'Average=1', opt);
                    
        case '201.2', % TX Power Level at 4920 MHz
            data = DwPhyTest_TxPower(ListTXPWRLVLs,'fcMHz = 4920', ...
                SetRxSensitivityString, opt);
                    
        case '201.3', % TX Power Level at 5200 MHz
            data = DwPhyTest_TxPower(ListTXPWRLVLs,'fcMHz = 5200', ...
                SetRxSensitivityString, opt);
                    
        case '201.4', % TX Power Level at 5500 MHz
            data = DwPhyTest_TxPower(ListTXPWRLVLs,'fcMHz = 5500', ...
                SetRxSensitivityString, opt);
                    
        case '201.5', % TX Power Level at 5805 MHz
            data = DwPhyTest_TxPower(ListTXPWRLVLs,'fcMHz = 5805', ...
                SetRxSensitivityString, opt);
                    
        case '202.1', % L-OFDM TxMask at 2412 MHz, ~2.5 minutes
            data.fcMHz = 2412;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            
        case '202.2', % CCK TxMask at 2412 MHz, ~2.5 minutes
            data.fcMHz = 2412;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 11', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            
        case '202.3', % L-OFDM TxMask at 5200 MHz, ~2.3 minutes
            data.fcMHz = 5200;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
        
        case '202.4', % HT-OFDM TxMask at 2412 MHz, ~2.5 minutes
            data.fcMHz = 2412;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6.5', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            
        case '202.5', % HT-OFDM TxMask at 5200 MHz, ~2.5 minutes
            data.fcMHz = 5200;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6.5', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            
        case '203.1', % Phase Noise at 2412 MHz, ~1.7 min
            data.fcMHz = 2412;
            data.TXPWRLVL = ListTXPWRLVL4;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxPhaseNoise( ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, 
                    data.Result=-1; break; 
                else
                    data.PhaseNoise_dBc(i) = data.result{i}.PhaseNoise_dBc;
                end
            end
            
        case '203.2', % Phase Noise at 5200 MHz
            data.fcMHz = 5200;
            data.TXPWRLVL = ListTXPWRLVL8;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxPhaseNoise( ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, 
                    data.Result=-1; break; 
                else
                    data.PhaseNoise_dBc(i) = data.result{i}.PhaseNoise_dBc;
                end
            end
            
        case '204', data = RunSet(data, [204.1 204.2 204.3], opt);
            
        case '204.1', % EVM at 2412 MHz vs TXPWRLVL, ~5 min
            data = DwPhyTest_TxEVMvPout('fcMHz = 2412', ListTXPWRLVL16s, ...
                'MeasurePower = 1','Npts=20', SetRxSensitivityString, opt);

        case '204.2', % EVM at 5200 MHz vs TXPWRLVL
            data = DwPhyTest_TxEVMvPout('fcMHz = 5200', ListTXPWRLVL16s, ...
                'MeasurePower = 1','Npts=20', SetRxSensitivityString, opt);
            
        case '204.3', % DSSS EVM at 2412 MHz vs TXPWRLVL, ~5 min
            data.fcMHz = 2412;
            data.TXPWRLVL = ListTXPWRLVL4;
            for i=1:length(data.TXPWRLVL),
                data.result{i} = DwPhyTest_TxEVMb('fcMHz=2412', ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, 
                    data.Result=-1; break; 
                else
                    data.Margin_dB(i) = data.result{i}.Margin_dB;
                end
            end
            
        case '205',   data = RunSet(data, [205.1 205.2], opt);
        case '205.1', data = DwPhyTest_Kvco('fcMHz = 2412', 'SweepByBand=1', opt);
        case '205.2', data = DwPhyTest_Kvco('fcMHz = 5200', 'SweepByBand=1', opt);
            
        case '206',   data = RunSet(data, [206.1 206.2 206.3], opt);
            
        case '206.1',
            data.fcMHz = 2412;
            data.Mbps = 6;
            data.LengthPSDU = [50 100 150 200 250 500 750 1000 1250 1500 1550];
            data.TXPWRLVL = ListTXPWRLVL1;
            for i=1:length(data.LengthPSDU),
                ScopeSetup = (i==1);
                data.result{i} = DwPhyTest_SingleTxEVM('Npts = 10', ...
                    sprintf('ScopeSetup = %d',ScopeSetup), ...
                    sprintf('ScopeAmplitude = %d',ScopeSetup), ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('Mbps = %d',data.Mbps), ...
                    sprintf('LengthPSDU = %d',data.LengthPSDU(i)), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                    SetRxSensitivityString, opt );
                data.AvgEVMdB(i) = data.result{i}.AvgEVMdB;
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            
        case '206.2',
            data.fcMHz = 2412;
            data.Mbps = [6 9 12 18 24 36 48 54];
            data.LengthPSDU = floor((20*4*data.Mbps - 16 - 6)/8);
            data.TXPWRLVL = ListTXPWRLVL1;
            wiParse_Line('CalcEVM_OFDM.aModemTX = 0');
            for i=1:length(data.Mbps),
                ScopeSetup = (i==1);
                data.result{i} = DwPhyTest_TxEVM('Npts = 20', ...
                    sprintf('ScopeSetup = %d',ScopeSetup), ...
                    sprintf('ScopeAmplitude = %d',ScopeSetup), ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('Mbps = %d',data.Mbps(i)), ...
                    sprintf('LengthPSDU = %d',data.LengthPSDU(i)), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                    'PacketType = 1', SetRxSensitivityString, opt );
                data.AvgEVMdB(i) = data.result{i}.AvgEVMdB;
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            clear wiseMex;
            
        case '206.3',
            data.fcMHz = 2412;
            data.Mbps = [6.5 13 19.5 26 39 52 58.5 65];
            data.LengthPSDU = floor((20*4*data.Mbps - 16 - 6)/8);
            data.TXPWRLVL = ListTXPWRLVL1;
            wiParse_Line('CalcEVM_OFDM.aModemTX = 0');
            for i=1:length(data.Mbps),
                ScopeSetup = (i==1);
                data.result{i} = DwPhyTest_TxEVM('Npts = 20', ...
                    sprintf('ScopeSetup = %d',ScopeSetup), ...
                    sprintf('ScopeAmplitude = %d',ScopeSetup), ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('Mbps = %d',data.Mbps(i)), ...
                    sprintf('LengthPSDU = %d',data.LengthPSDU(i)), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                    'PacketType = 1', SetRxSensitivityString, opt );
                data.AvgEVMdB(i) = data.result{i}.AvgEVMdB;
                if data.result{i}.Result == -1, data.Result=-1; break; end
            end
            clear wiseMex;
            
        case '207.1',
            data.fcMHz = 2412;
            data.TXPWRLVL = [63 43];
            RegRangeS = '128:2:254';
            data.PADGAINO = eval(RegRangeS);
            data.result63 = DwPhyTest_TxPowerVsReg('fcMHz=2412','TXPWRLVL = 63', ...
                'RegAddr=256+67','UserReg={{256+65,0,1}}','Average=1', ...
                sprintf('RegRange=%s',RegRangeS));
            data.result43 = DwPhyTest_TxPowerVsReg('fcMHz=2412','TXPWRLVL = 43', ...
                'RegAddr=256+67','UserReg={{256+65,0,1}}','Average=1', ...
                sprintf('RegRange=%s',RegRangeS));
            data.Pout_dBm = [data.result63.Pout_dBm' data.result43.Pout_dBm'];

        case '207.2',
            data.fcMHz = 5200;
            data.TXPWRLVL = [63 43];
            RegRangeS = '128:2:254';
            data.PADGAINO = eval(RegRangeS);
            data.result63 = DwPhyTest_TxPowerVsReg('fcMHz=5200','TXPWRLVL = 63', ...
                'RegAddr=256+67','UserReg={{256+65,0,1}}','Average=1', ...
                sprintf('RegRange=%s',RegRangeS));
            data.result43 = DwPhyTest_TxPowerVsReg('fcMHz=5200','TXPWRLVL = 43', ...
                'RegAddr=256+67','UserReg={{256+65,0,1}}','Average=1', ...
                sprintf('RegRange=%s',RegRangeS));
            data.Pout_dBm = [data.result63.Pout_dBm' data.result43.Pout_dBm'];
            
        case '208.1',
            data.fcMHz = 2412;
            data.TXPWRLVL = [63 43];
            RegRangeS = '32:2:120';
            data.DACOFFSET = eval(RegRangeS);
            data.result63 = DwPhyTest_TxPowerVsReg('fcMHz=2412','TXPWRLVL = 63', ...
                'RegAddr=256+84','Average=1', sprintf('RegRange=%s',RegRangeS));
            data.result43 = DwPhyTest_TxPowerVsReg('fcMHz=2412','TXPWRLVL = 43', ...
                'RegAddr=256+84','Average=1', sprintf('RegRange=%s',RegRangeS));
            data.Pout_dBm = [data.result63.Pout_dBm' data.result43.Pout_dBm'];

        case '208.2',
            data.fcMHz = 5200;
            data.TXPWRLVL = [63 43];
            RegRangeS = '32:2:120';
            data.DACOFFSET = eval(RegRangeS);
            data.result63 = DwPhyTest_TxPowerVsReg('fcMHz=5200','TXPWRLVL = 63', ...
                'RegAddr=256+84','Average=1', sprintf('RegRange=%s',RegRangeS));
            data.result43 = DwPhyTest_TxPowerVsReg('fcMHz=5200','TXPWRLVL = 43', ...
                'RegAddr=256+84','Average=1', sprintf('RegRange=%s',RegRangeS));
            data.Pout_dBm = [data.result63.Pout_dBm' data.result43.Pout_dBm'];

        case '209.2',
            data = DwPhyTest_TxPower('TXPWRLVL = 63','fcMHz = DwPhyTest_ChannelList', ...
                SetRxSensitivityString, 'Average=1', opt);
            
        case '210.1',
            data = DwPhyTest_TxEVMvRegister('fcMHz = 2412','MeasurePower = 1', ...
                'Npts = 20','Mbps = 6','LengthPSDU = 80',SetRxSensitivityString, ...
                'UserReg = {{256+65,0,1}}','RegAddr=256+67','RegRange=192:2:254');

        case '210.2',
            data = DwPhyTest_TxEVMvRegister('fcMHz = 5200','MeasurePower = 1', ...
                'Npts = 20','Mbps = 6','LengthPSDU = 80',SetRxSensitivityString, ...
                'UserReg = {{256+65,0,1}}','RegAddr=256+67','RegRange=192:2:254');

        case {'211.1','211.2'},
            switch upper(TestID)
                case '211.1', data.fcMHz = 2412;
                case '211.2', data.fcMHz = 5200;
            end
            data.TXPWRLVL = 63;
            data.PADGAIN = 192:2:254;
            data.k = 1;
            for i=1:length(data.PADGAIN),
                data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6', ...
                    sprintf('fcMHz = %d',data.fcMHz), ...
                    sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                    sprintf('UserReg = {{256+65,0,1},{256+67,%d}}',data.PADGAIN(i)), ...
                    SetRxSensitivityString, opt );
                if data.result{i}.Result == -1, data.Result=-1; break; end
                if data.result{i}.Result == 1, data.k = i; end
            end
            data.resultCCK = DwPhyTest_TxSpectrumMask('Mbps = 11', ...
                sprintf('fcMHz = %d',data.fcMHz), ...
                sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                sprintf('UserReg = {{256+65,0,1},{256+67,%d}}',data.PADGAIN(data.k)), ...
                SetRxSensitivityString, opt );
            if data.resultCCK.Result == -1, data.Result=-1; end
            
        case '212.1', % TX Power vs TXPWRLVL at PADGAIN = 210, 2412 MHz
            data = DwPhyTest_TxPower('TXPWRLVL=32:63','fcMHz = 2412', ...
                'UserReg = {{256+65,0,1},{256+67,210}}', ...
                SetRxSensitivityString, 'Average=1', opt);
                    
        case '213.1',
            data = DwPhyTest_TxEVMvRegister('fcMHz = 2412','MeasurePower = 1', ...
                'Npts = 1','Mbps = 6','LengthPSDU = 80',SetRxSensitivityString, ...
                'UserReg = {{256+65,0,1}}','RegAddr=256+67','RegRange=192:2:254');
            data.PADGAIN = 192:2:254;
            data.k = find(data.AvgEVMdB > -25, 1 );
            if ~isempty(data.k) && data.Result ~= -1,
                for i=data.k:length(data.PADGAIN),
                    data.result{i} = DwPhyTest_TxSpectrumMask('Mbps = 6', ...
                        sprintf('fcMHz = %d',data.fcMHz), ...
                        sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                        sprintf('UserReg = {{256+65,0,1},{256+67,%d}}',data.PADGAIN(i)), ...
                        SetRxSensitivityString, opt );
                    if data.result{i}.Result == -1, data.Result=-1; break; end
                    if data.result{i}.Result == 1, data.k = i; end
                    if data.result{i}.Result == 0, break; end % found mask failure
                end
                if data.Result ~= -1
                    data.resultCCK = DwPhyTest_TxSpectrumMask('Mbps = 11', ...
                        sprintf('fcMHz = %d',data.fcMHz), ...
                        sprintf('TXPWRLVL = %d',data.TXPWRLVL), ...
                        sprintf('UserReg = {{256+65,0,1},{256+67,%d}}',data.PADGAIN(data.k)), ...
                        SetRxSensitivityString, opt );
                    if data.resultCCK.Result == -1, data.Result=-1; end
                end
            end
            
        case '214.1',
            data = DwPhyTest_SleepWakeALC;

        case '215.1', % DMW96 TX I/Q-Calibration at 2442 MHz, ~25 min
            data = DwPhyTest_IQCalibrationTx(opt);
            
        %%% OPTIMIZATION/TUNING PROCEDURES %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %%% -------------------------------------------------------------------------
        case '300.1',
            data = DwPhyTest_InitAGain(opt);
            RxTuning = DwPhyLab_Parameters('RxTuningVsChanl');
            RxTuning.fcMHz     = data.fcMHz;
            RxTuning.InitAGain = data.InitAGain;
            DwPhyLab_Parameters('RxTuning',RxTuning);
            
        case '300.2',
            data = DwPhyTest_GainVsChanl(opt);
            RxTuning = DwPhyLab_Parameters('RxTuning');
            RxTuning.fcMHz    = data.fcMHz;
            RxTuning.StepLNA0 = round(mean(data.StepLNA0dB,2)/1.5);
            RxTuning.StepLNA1 = round(mean(data.StepLNA1dB,2)/1.5);
            DwPhyLab_Parameters('RxTuning',RxTuning);
            
        case '300.3',
            RxTuning = DwPhyLab_Parameters('RxTuning');
            for fcMHz = fcMHzList,
                for LNAATTENEN = 0:1,
                    if isfield(RxTuning,'InitAGain') && isfield(RxTuning,'StepLNA0')
                        if DwPhyLab_RadioIsRF22
                            UserReg = sprintf('UserReg = {{65,%d},{74,%d},{256+93,0,%d}}', ...
                                              interp1(RxTuning.fcMHz, RxTuning.InitAGain, fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.StepLNA0,  fcMHz, 'nearest'), ...
                                              LNAATTENEN );                            
                        else
                            UserReg = sprintf('UserReg = {{65,%d},{74,%d},{256+95,0,%d}}', ...
                                              interp1(RxTuning.fcMHz, RxTuning.InitAGain, fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.StepLNA0,  fcMHz, 'nearest'), ...
                                              LNAATTENEN );
                        end
                    else
                        UserReg = {};
                        data.Result = 0;
                    end
                    fcMHzArg = sprintf('fcMHz = %d',fcMHz);
                    FieldName = sprintf('result%d_%d',fcMHz,LNAATTENEN);
                    data.(FieldName) = DwPhyTest_ThSwitchLNA(fcMHzArg, UserReg, opt);
                end
            end
            for i = 1:length(RxTuning.fcMHz),
                if length(fcMHzList) > 1,
                    x = min( max(fcMHzList), max( min(fcMHzList), RxTuning.fcMHz(i) ) );
                    y = interp1( fcMHzList, fcMHzList, x, 'nearest');
                else
                    y = fcMHzList;
                end
                z0 = data.(sprintf('result%d_0',y)).CenterPrdBm;
                z1 = data.(sprintf('result%d_1',y)).CenterPrdBm;
                RxTuning.ThSwitchLNA0(i) = round((z0 + 65)/1.5);
                RxTuning.ThSwitchLNA1(i) = round((z1 + 65)/1.5);
            end
            DwPhyLab_Parameters('RxTuning',RxTuning);
            
        case '300.4',
            RxTuning = DwPhyLab_Parameters('RxTuning');
            for fcMHz = fcMHzList,
                for LNAATTENEN = 0:1,
                    if isfield(RxTuning,'InitAGain') && isfield(RxTuning,'StepLNA0') ...
                        && isfield(RxTuning,'ThSwitchLNA0') && isfield(RxTuning,'ThSwitchLNA1')
                        ThSwitchLNA = sprintf('ThSwitchLNA%d',LNAATTENEN);
                        if DwPhyLab_RadioIsRF22
                            UserReg = sprintf('UserReg = {{65,%d},{74,%d},{73,%d},{256+93,0,%d}}', ...
                                              interp1(RxTuning.fcMHz, RxTuning.InitAGain,     fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.StepLNA0,      fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.(ThSwitchLNA), fcMHz, 'nearest'), ...
                                              LNAATTENEN );                            
                        else
                            UserReg = sprintf('UserReg = {{65,%d},{74,%d},{73,%d},{256+95,0,%d}}', ...
                                              interp1(RxTuning.fcMHz, RxTuning.InitAGain,     fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.StepLNA0,      fcMHz, 'nearest'), ...
                                              interp1(RxTuning.fcMHz, RxTuning.(ThSwitchLNA), fcMHz, 'nearest'), ...
                                              LNAATTENEN );
                        end
                    else
                        UserReg = {}; 
                        data.Result = 0;
                    end
                    fcMHzArg = sprintf('fcMHz = %d',fcMHz);
                    FieldName = sprintf('result%d_%d',fcMHz,LNAATTENEN);
                    data.(FieldName) = DwPhyTest_PERvPrdBm(fcMHzArg, UserReg, ...
                        'Mbps = 54','PktsPerWaveform = 10','MaxPkts = 2000','MinFail = 200', opt);
                end
            end
            
        case '301.1',
            for fcMHz = fcMHzList,
                fcMHzArg = sprintf('fcMHz = %d',fcMHz);
                FieldName = sprintf('result%d',fcMHz);
                data.(FieldName) = DwPhyTest_TuneDPLL( fcMHzArg, opt );
            end

        case '301.2', % Invalid prior to 2008-02-08 @ 19:10
            for fcMHz = fcMHzList,
                fcMHzArg = sprintf('fcMHz = %d',fcMHz);
                FieldName = sprintf('result%d',fcMHz);
                data.(FieldName) = DwPhyTest_TuneDPLL(fcMHzArg,'TimingDPLL = 1','LengthPSDU = 1500', opt);
            end
            
        case '302.1', % 802.11a Signal Detect Sensitivity, SigDetTh1
            data = DwPhyTest_SensitivityVsRegister('fcMHz = 2412','RxMode = 1',...
                'Mbps = 6','LengthPSDU = 100',...
                'PrdBm = -99:-60','MaxPkts = 1000','MinPER = 0.01', ...
                'UserReg = {{113,0},{114,0}}',...
                'RegAddr = 112','RegField = ''5:0''','RegRange = [2 3 4 5 6 8 11 16 22 32 45 63]', opt);
            
        case '302.2', % 802.11a Signal Detect Sensitivity, SigDetTh2
            data = DwPhyTest_SensitivityVsRegister('fcMHz = 2412','RxMode = 1',...
                'Mbps = 6','LengthPSDU = 100',...
                'PrdBm = -99:-60','MaxPkts = 1000','MinPER = 0.01', ...
                'UserReg = {{112,2}}',...
                'RegAddr = 113','RegField = ''15:0''','RegRange = round(logspace(0, log10(65535), 33))', opt);

        case '302.3', % 802.11b Carrier Sense Sensitivity (11 Mbps), CQthreshold
            x = 0:48;
            t = bitand(x,3) .* 2.^bitshift(x,-2);
            [t,I] = sort(t);
            n=hist(t,t);
            y = x(I((n > 0) & (t>0)));
            
            data = DwPhyTest_SensitivityVsRegister('fcMHz = 2412','RxMode = 2',...
                'Mbps = 11','LengthPSDU = 50',...
                'PrdBm = -99:-60','MaxPkts = 1000','MinPER = 0.01', ...
                'UserReg = {{113,255},{114,255}}',...
                'RegAddr = 162','RegField = ''5:0''',sprintf('RegRange = [%s];',num2str(y)), opt );

        case '302.4', % 802.11b Carrier Sense Sensitivity (1 Mbps), CQthreshold
            x = 0:48;
            t = bitand(x,3) .* 2.^bitshift(x,-2);
            [t,I] = sort(t);
            n=hist(t,t);
            y = x(I((n > 0) & (t>0)));
            
            data = DwPhyTest_SensitivityVsRegister('fcMHz = 2412','RxMode = 2',...
                'Mbps = 1','LengthPSDU = 50',...
                'PrdBm = -99:-60','MaxPkts = 1000','MinPER = 0.01', ...
                'UserReg = {{113,255},{114,255}}',...
                'RegAddr = 162','RegField = ''5:0''',sprintf('RegRange = [%s];',num2str(y)), opt );

        case '302.5', % 802.11g DSSS Carrier Sense Sensitivity (1 Mbps), CQthreshold
            x = 0:48;
            t = bitand(x,3) .* 2.^bitshift(x,-2);
            [t,I] = sort(t);
            n=hist(t,t);
            y = x(I((n > 0) & (t>0)));
            
            data = DwPhyTest_SensitivityVsRegister('fcMHz = 2412',...
                'Mbps = 1','LengthPSDU = 50',...
                'PrdBm = -99:-60','MaxPkts = 1000','MinPER = 0.01', ...
                'UserReg = {{103,2,1},{113,255},{114,255}}',...
                'RegAddr = 162','RegField = ''5:0''',sprintf('RegRange = [%s];',num2str(y)), opt );

        case '303.1', % CFO vs REFOSCTUNE
            DwPhyLab_Initialize;
            if ~DwPhyLab_RadioIsRF22
                data = DwPhyTest_MeasureRxCFO('RegAddr=256+38','RegRange=0:255', ...
                                              'fcMHz = 2484','SetRxSensitivity = -79','DiversityMode = 1');
            else
                data = DwPhyTest_MeasureRxCFO('RegAddr=256+21','RegRange=0:255', ...
                                              'fcMHz = 2484','SetRxSensitivity = -79','DiversityMode = 1');
            end
            
        case {'304.1','304.2','304.3'} % Sensitivity/CCK-ACR vs InitAGain, 15 minutes
            data.fcMHz = 2437;
            fcMHzS = sprintf('fcMHz = %d',data.fcMHz);
            switch upper(TestID),
                case '304.1', data.DiversityMode = 3; DiversityModeS='DiversityMode = 3';
                case '304.2', data.DiversityMode = 2; DiversityModeS='DiversityMode = 2';
                case '304.3', data.DiversityMode = 1; DiversityModeS='DiversityMode = 1';
            end
            DwPhyLab_Initialize; DwPhyLab_SetChannelFreq(data.fcMHz);
            InitAGain = DwPhyLab_ReadRegister(65);
            data.InitAGain = max(1,InitAGain-8) : min(40,InitAGain+9);
            InitAGain = data.InitAGain;
            RegRangeS = sprintf('RegRange = %g:%g:%g',min(InitAGain),mean(diff(InitAGain)),max(InitAGain));

            for i=1:length(InitAGain),
                if i == 1,
                    SkipWaveformLoadS = 'SkipWaveformLoad = 0';
                    PidBmS = 'PidBm = -22:-0.5:-80';
                else
                    SkipWaveformLoadS = 'SkipWaveformLoad = 1';
                    PidBmMax = min(-25, ceil(max(data.resultI{i-1}.PidBm0) + 2));
                    PidBmS = sprintf('PidBm = %d:-0.5:-80',PidBmMax);
                end
                data.resultI{i} = DwPhyTest_InterferenceRejection(fcMHzS, 'SetRxSensitivity=-91',...
                    'InterferenceType =''ACI-CCK''', SkipWaveformLoadS, PidBmS, ...
                    sprintf('UserReg={{65,%d}}',InitAGain(i)), DiversityModeS );
                data.Rejection_dB(i) = min( data.resultI{i}.Rejection_dB );
                fprintf('InitAGain = %d (%2d:%d), Rejection = %1.1f dB\n',InitAGain(i),i,...
                    length(InitAGain),data.Rejection_dB(i) );
            end
            data.resultS = DwPhyTest_SensitivityVsRegister(fcMHzS, ...
                'RegAddr=65', RegRangeS, DiversityModeS );
            data.Sensitivity = data.resultS.Sensitivity;
            data.Runtime = 24*3600*(now-datenum(data.Timestamp));

        case '305.1',
            data = DwPhyTest_SetRxSensitivity('fcMHz = 2437','DiversityMode = 3', opt);
            
        case '305.2',
            data = DwPhyTest_SetRxSensitivity('fcMHz = 2437','DiversityMode = 2', opt);

        case '305.3',
            data = DwPhyTest_SetRxSensitivity('fcMHz = 2437','DiversityMode = 1', opt);

        case '306.1', % CCK-ACR vs RF52 SGAIBIASTRIM at 2437 MHz
            data.fcMHz = 2437;
            fcMHzS = sprintf('fcMHz = %d',data.fcMHz);
            DwPhyLab_Initialize; DwPhyLab_SetChannelFreq(data.fcMHz);
            RegRange = 0:7;
            for i=1:length(RegRange),
                if i == 1,
                    SkipWaveformLoadS = 'SkipWaveformLoad = 0';
                    PidBmS = 'PidBm = -25:-0.5:-80';
                else
                    SkipWaveformLoadS = 'SkipWaveformLoad = 1';
                    PidBmMax = min(-25, ceil(max(data.resultI{i-1}.PidBm0) + 6));
                    PidBmS = sprintf('PidBm = %d:-0.5:-80',PidBmMax);
                end
                data.resultI{i} = DwPhyTest_InterferenceRejection(fcMHzS, 'SetRxSensitivity=0',...
                    'InterferenceType =''ACI-CCK''', SkipWaveformLoadS, PidBmS, ...
                    sprintf('UserReg={{256+107,''7:5'',%d}}',RegRange(i)) );
                data.Rejection_dB(i) = min( data.resultI{i}.Rejection_dB );
                fprintf('SGAIBIASTRIM = %d (%2d:%d), Rejection = %1.1f dB\n',RegRange(i),i,...
                    length(RegRange),data.Rejection_dB(i) );
            end
            RegRangeS = sprintf('RegRange = %d:%d',min(RegRange),max(RegRange));
            data.resultS = DwPhyTest_SensitivityVsRegister(fcMHzS, ...
                'RegAddr=256+107', 'RegField=''7:5''', RegRangeS );
            data.Sensitivity = data.resultS.Sensitivity;
            data.Runtime = 24*3600*(now-datenum(data.Timestamp));
            
        case '307.1', % RF22 Tx Power Control Calibration
            if DwPhyLab_ReadRegister(256+1) < 65,
                data.Description = 'Radio ID < 65; test 307.1 not run';
                data.Result = 1;
            else
                data = DwPhyTest_TxCal(SetRxSensitivityString, opt);
                numReg = size(data.CalParam, 2);           
                x = zeros(1, 3*numReg);
                x(1, 1:3:end) = bitand(data.CalParam(1,:), hex2dec('FF'));
                x(1, 2:3:end) = bitshift(data.CalParam(1,:), -8);
                x(1, 3:3:end) = data.CalParam(2,:);
                x = uint8(x);

                DwPhyParameterData = DwPhyLab_Parameters('DwPhyParameterData');
                if isfield(DwPhyParameterData, 'DefaultRegisters')
                    existingRegs =     uint16(DwPhyParameterData.DefaultRegisters(1:3:end)) + ...
                                   256*uint16(DwPhyParameterData.DefaultRegisters(2:3:end));
                    newRegs      =     uint16(x(1:3:end)) + ...
                                   256*uint16(x(2:3:end));
                    for i = 1:numReg
                        idx = find(existingRegs == newRegs(i));
                        if isempty(idx)
                            DwPhyParameterData.DefaultRegisters = [DwPhyParameterData.DefaultRegisters ...
                                                                   x(3*(i-1)+1 : 3*(i-1)+3)];
                        else
                            DwPhyParameterData.DefaultRegisters(3*(idx-1)+3) = x(3*(i-1)+3);
                        end
                    end
                else
                    DwPhyParameterData.DefaultRegisters = x;
                end            
                DwPhyLab_Parameters('DwPhyParameterData', DwPhyParameterData);
            end
            
        %%% MODE SWITCH TIMING %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        %%% -------------------------------------------------------------------------
        
        case '400', data = RunSet(data, [400.1 400.2 400.3], opt);

        case '400.1', % Standby-to-RX, OFDM in 802.11g
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,...
                'DiversityMode=1','MinFail=1e9','MaxPkts=10000',...
                'Mbps=[6 54]','LengthPSDU=[120 100]','T_IFS=[16e-6 100e-6]',...
                'Broadcast=[0 1]','PacketUseRate = 0.5',...
                'UserReg={{198,0,1},{220,16}}', opt...
                );
            data.Result = all(data.PER < 0.01);
            
        case '400.2', % Standby-to-RX, CCK in 802.11g
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,...
                'DiversityMode=1','MinFail=1e9','MaxPkts=2e3',...
                'Mbps=[6 11]','LengthPSDU=[150 1000]','T_IFS=[10e-6 100e-6]',...
                'Broadcast=[0 1]','PacketUseRate = 0.5',...
                'UserReg={{198,0,1},{220,10}}', opt...
                );
            data.Result = all(data.PER < 0.01);

        case '400.3', % Standby-to-RX, OFDM in 802.11a
            data = DwPhyTest_PERvPrdBm('fcMHz=5200',MidRangePrdBmS,'RxMode=1',...
                'DiversityMode=1','MinFail=1e9','MaxPkts=10000',...
                'Mbps=[54 54]','LengthPSDU=[1000 100]','T_IFS=[16e-6 100e-6]',...
                'Broadcast=[0 1]','PacketUseRate = 0.5',...
                'UserReg={{198,0,1},{220,16}}', opt...
                );
            data.Result = all(data.PER < 0.01);

        case '400.4', % Standby-to-RX, CCK in 802.11b
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,'RxMode=2',...
                'DiversityMode=1','MinFail=1e9','MaxPkts=10000',...
                'Mbps=[11 11]','LengthPSDU=[100 100]','T_IFS=[10e-6 100e-6]',...
                'Broadcast=[0 1]','PacketUseRate = 0.5',...
                'UserReg={{198,0,1},{220,10}}', opt...
                );
            data.Result = all(data.PER < 0.01);

        case '401.1', % TX-to-RX, OFDM, 802.11g
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,...
                'Mbps=54','T_IFS=76.2e-6','Broadcast=0','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'MinFail=1e9', 'MaxPkts=10000', opt...
                );
            data.Result = all(data.PER < 0.01);

        case '401.2', % TX-to-RX, OFDM, 802.11a
            data = DwPhyTest_PERvPrdBm('fcMHz=5200',MidRangePrdBmS,'RxMode=1',...
                'Mbps=54','T_IFS=76.2e-6','Broadcast=0','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'MinFail=1e9','MaxPkts=10000',opt...
                );
            data.Result = all(data.PER < 0.01);
            
        case '401.3', % TX-to-RX, CCK, 802.11g
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,...
                'Mbps=11','T_IFS=324.2e-6','Broadcast=0','DiversityMode=1', ...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'MinFail=1e9','MaxPkts=10000',opt...
                );
            data.Result = all(data.PER < 0.01);

        case '401.4', % TX-to-RX, CCK, 802.11b
            data = DwPhyTest_PERvPrdBm('fcMHz=2412',MidRangePrdBmS,'RxMode=2',...
                'Mbps=11','T_IFS=324.2e-6','Broadcast=0','DiversityMode=1', ...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'MinFail=1e9','MaxPkts=10000',opt...
                );
            data.Result = all(data.PER < 0.01);
            
        case '401.5', % TX-to-RX, OFDM, 802.11n
            data = DwPhyTest_PERvPrdBm('fcMHz=2412','PrdBm = -58:-30',...
                'Mbps=65','T_IFS=76.2e-6','Broadcast=0','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'MinFail=1e9', 'MaxPkts=10000', opt...
                );
            data.Result = all(data.PER < 0.01);
            
        case '402.1', % TX-OFDM to RX-CCK in 8 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [6 11]','Broadcast = [0 1]',...
                'LengthPSDU=[50 1000]','fcMHz=2484', MidRangePrdBmS,'MinFail = 100',...
                'MinFail=1e9','MaxPkts=2e3','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 60e-6 + 8e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        case '402.2', % TX-OFDM to RX-CCK in 12 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [6 11]','Broadcast = [0 1]',...
                'LengthPSDU=[50 1000]','fcMHz=2484', MidRangePrdBmS,'MinFail = 100',...
                'MinFail=1e9','MaxPkts=2e3','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 60e-6 + 12e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        case '402.3', % TX-OFDM to RX-CCK in 50 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [6 11]','Broadcast = [0 1]',...
                'LengthPSDU=[50 50]','fcMHz=2484', MidRangePrdBmS,'MinFail = 100',...
                'MinFail=1e9','MaxPkts=1e4','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = 60e-6 + 50e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        case '403.1', % RX-CCK, TX-ACK(1Mbps), RX-OFDM in 16 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [11 54]', 'Broadcast = [0 1]',...
                'LengthPSDU=[50 1000]','fcMHz=2484', MidRangePrdBmS,...
                'MinFail=1e9','MaxPkts=1e4','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = (10.2+304+16)*1e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        case '403.2', % RX-CCK, TX-ACK(1Mbps), RX-OFDM in 34 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [11 54]', 'Broadcast = [0 1]',...
                'LengthPSDU=[50 1000]','fcMHz=2484', MidRangePrdBmS,...
                'MinFail=1e9','MaxPkts=1e4','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = (10.2+304+34)*1e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        case '403.3', % RX-OFDM, TX-ACK(6Mbps), RX-CCK in 8 usec
            data = DwPhyTest_PERvPrdBm('Mbps = [6 11]', 'Broadcast = [0 1]',...
                'LengthPSDU=[50 100]','fcMHz=2484', MidRangePrdBmS,...
                'MinFail=1e9','MaxPkts=1e4','DiversityMode=1',...
                'UserReg={{DwPhyLab_Parameters(''BaseAddress'')+hex2dec(''90A8''),1}}',...
                'T_IFS = (16.2+44+8)*1e-6', opt);
            data.Result = all(data.PER < 0.01);
            
        otherwise,
            data.Result = -9;
    end
catch ME
    data.Result = -2;
    data.lasterror = ME;
end

%% Make sure data.Result is 0 if any individual result is 0
if isfield(data, 'result') && iscell(data.result) && isfield(data.result{1}, 'Result')
    if isfield(data, 'Result')
        if data.Result == 1 && any(cellfun(@(x) x.Result, data.result) == 0)
            data.Result = 0;
        end
    end
end

%% Make sure that data.fcMHz records the true value if fcMHz is changed from default through 'opt'.
%% Note that this only checks tests with multiple 'result' on a single channel.
if isfield(data, 'result') && iscell(data.result) && isfield(data.result{1}, 'fcMHz')
    if isfield(data, 'fcMHz') && (numel(data.fcMHz) == 1)
        if data.fcMHz ~= data.result{1}.fcMHz
            data.fcMHz = data.result{1}.fcMHz;
        end
    end
end

%% Cleanup data structure
if ~isfield(data,'Runtime')
    if isfield(data,'Timestamp'),
        data.Runtime = 24*3600*(now - datenum(data.Timestamp));
    end
end

%% Function RunSet
function data = RunSet(data, TestList, opt)
data.Result = 1;

for i=1:length(TestList),
    TestName = sprintf('Test%g',TestList(i));
    TestName(TestName == '.') = '_';
    data.(TestName) = DwPhyTest_RunTest(TestList(i), opt);
    if data.(TestName).Result == -1,
        data.Result = -1; 
        return;
    elseif data.(TestName).Result == 0,
        data.Result = 0;
    end
end

%% REVISIONS
% 2008-05-15: Tests 14.x, 17.1 now use 2412 MHz as the channel
% 2008-06-02: Changed fcMHz=5200 to fcMHz=2412 for tests 302.1 and 302.2
% 2008-06-08: Add option field opt
% 2008-07-09: Reduced Rx sensitivity during TX tests
% 2008-08-01: Updated TXPWRLVL values for RF52B2
% 2008-10-20: Corrected T_IFS in 401.3, 401.4 from 376e-6 to 324.2e-6
% 2010-04-08: Added parameter opt to RunSet call for TestID = 2
% 2010-??-??: Added tests 206.3, 207.2, 208.2, 210.2, 211.2
% 2010-11-17 [SM]: Corrected input arguments for 11.3 and 103.4 to match test description
% 2010-12-?? [SM]: 1. Add missing test descriptions
%                  2. Add test 307.1 for RF22 power control calibration
%                  3. Add DwPhyTestExceptions for RF22A0 TXPWRLVLs 
% 2011-03-21 [SM]: Utilized 'SetRxSensitivityDuringTx' param to set the RX sensitivity used by TX tests.
% 2011-04-19 [SM]: Added tests for DMW96 I/Q calibration. 
