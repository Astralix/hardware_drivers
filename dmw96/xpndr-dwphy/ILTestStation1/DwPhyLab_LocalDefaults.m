function param = DwPhyLab_LocalDefaults
%param = DwPhyLab_LocalDefaults
%
%   Returns station-specific default parameters. 
%   This file is intended to be modified for each test station. 

%% Station Name (Informational)
param.Station = 'Screen Room';

%% Remote VVWAL Interface
%  The IP address is the ethernet address for the device under test. Generally the
%  port value should not be changed from 30000 unless the server is recompiled to use
%  another value.
param.STA.IP = '10.82.0.12';
param.STA.Port = 30000;

%% PlatformID
% Select the test platform type
param.PlatformID = 0; % DW52/74 ASIC
%param.PlatformID = 1; % ARM Versatile + DMW96 FPGA
param.PlatformID = 2; % DMW96 ASIC

%% Base Address
%  This is the base address offset for the MAC, CMU, et cetera.
%  For DW52/74, this is 0x06500000. For the DMW96 FPGA, it is 86500000
if isfield(param,'PlatformID')
    switch param.PlatformID,
        case 0, param.BaseAddress = hex2dec('06500000'); % DW52/74 ASIC
        case 1, param.BaseAddress = hex2dec('86500000'); % DMW96 FPGA
        case 2, param.BaseAddress = hex2dec('06D00000'); % DMW96 ASIC
    end
end

%% DW52 MAC Address
%  The MAC is programmed to use this address (overrides system defaults). It is used
%  so that PER tests with unicast packets can be correctly formulated. The value
%  '452301900300' may be used to match the BERLab default
param.STA.WiFiMAC = '452301900300';

%% MAC TXPWRLVL for ACK
%  Defines the power level used to send ACK packets
param.STA.ACK_PWRSETTING = 59;

%% Instrument Addresses
%  Program instrument addresses. If an instrument such as the AWG520 is not part of
%  the local test station, comment out the line so that no parameter with the name
%  AWG520_Address exists
%param.AWG520_Address =   '10.82.0.100';
%param.E4438C_Address1=   '172.19.7.218'; % primary ESG
param.E4438C_Address1=   '10.82.0.103'; % primary ESG
%param.E4438C_Address2=   '172.19.7.222'; % used for interference, blocking
param.E4438C_Address2=   '10.82.0.102'; % used for interference, blocking
param.Scope_Address  =   'GPIB::5';%'GPIB::10';
%param.Scope_Address  =   '172.19.7.225';
param.VSA_Address    =   '172.19.7.227';
param.E4440A_Address =   'GPIB::18'; 
% param.E4440A_Address =   'IP:172.19.7.214'; 

%% Instrument Personalities
%  Define configuration terms needed to differentiate between similar but
%  not identical instruments
param.ScopeIsWavePro7200 = 0;%1;  

%% Select Control Library for Agilent ESG/PSA
%  The original DwPhyLab software used the Agilent Waveform Download Assistent. This is no
%  longer supported in Matlab 2009a or later. As an alternative, the mexLab/mexVISA
%  library can be used for instrument communication.
param.UseAgilentWaveformDownloadAssistent = 0;

%% Cable Calibration Data
%  This file contains arrays of loss versus frequency for RX and TX cables.
%  Information is stored in a structure "data" with an element CalData that is a 1xN
%  structure with fields "Freqs" and "Loss" (units are Hz and dB, respectively). The
%  "data" structure also contains elements iTX and iRX which are indices for the
%  CalData array elements for TX and RX loss, respectively.
%param.CalDataFile = 'MNTestStation3_CableLoss_TempChamber.mat';
param.CalDataFile = 'MNTestStation3_CableLoss_TableTop.mat';

%% ESG #2 Used for Interference Signals Has the DualARB Option
param.UseESG2DualArb = 1;

%% Operating Band
%  Specifies whether the DUT is 2.4/5 GHz dual-band (1) or just 2.4 GHz (0). The
%  value can be changed at runtime by calling DwPhyTest_SetBand.
param.DualBand = 0;
param.DualRX   = 0; % RF52 (2 RX paths)

%% Rx Sensitivity for Tx tests
% Set sensitivity during Tx related tests.
param.SetRxSensitivityDuringTx = -60;
