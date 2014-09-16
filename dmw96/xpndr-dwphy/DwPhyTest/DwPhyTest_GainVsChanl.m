function data = DwPhyTest_GainVsChanl(varargin)
% data = DwPhyTest_GainVsChanl(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include fcMHz, PrdBm, and AbsPwrH

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
PrdBm = -65;
AbsPwrH = 56;
fcMHz = DwPhyTest_ChannelList;

if DwPhyLab_RadioIsRF22, RadioRegLNA = 256 + 93;
else                     RadioRegLNA = 256 + 95; 
end

DwPhyLab_EvaluateVarargin(varargin);

%% Test Setup
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
DwPhyLab_DisableESG;

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;

Pout_dBm = PrdBm + data.Ploss(1);
DwPhyLab_SetRxPower(Pout_dBm);
DwPhyLab_DisableESG;
N = 4*length(fcMHz);

%% Measure Gain with LNAGAIN = 1
for i=1:length(fcMHz),
    if ~UpdateWaitBar('LNAGAIN = 1', fcMHz(i), i/N), break; end

    % Select channel and override PHY registers
    SetChannel( fcMHz(i) );
    if i==1, data.RegList = DwPhyLab_RegList; end
    data.InitAGain(i) = DwPhyLab_ReadRegister(65);
    data.StepLNA(i)   = DwPhyLab_ReadRegister(74);

    DwPhyLab_EnableESG;
    [Pout_dBm, data.sHigh(i,:)] = DwPhyTest_AdjustInputPower( Pout_dBm, AbsPwrH, 1.0, ...
        'SetRxPower = 0','Verbose = 0');
    data.PrdBmHigh(i) = Pout_dBm - data.Ploss(i);
    DwPhyLab_DisableESG;
end

%% Measure Noise Floor
DwPhyLab_DisableESG;
for i=1:length(fcMHz),
    if ~UpdateWaitBar('No Input', fcMHz(i), 0.25+i/N), break; end
    SetChannel( fcMHz(i) );
    data.nHigh(i,:) = DwPhyLab_AverageRSSI;
end

%% Measure Gain with LNAGAIN = 0, LNAATTENEN = 0
Pout_dBm = PrdBm + data.Ploss(1);
for i=1:length(fcMHz),
    if ~UpdateWaitBar('LNAGAIN = 0, LNAATTENEN = 0', fcMHz(i), 0.5+i/N), break; end
    SetChannel( fcMHz(i) );
    DwPhyLab_WriteRegister(         80, 6, 0); % LNAGAIN = 0
    DwPhyLab_WriteRegister(RadioRegLNA, 0, 0); % LNAATTENEN = 0
    DwPhyLab_WriteRegister(         74,    0); % clear StepLNA

    DwPhyLab_EnableESG;
    [Pout_dBm, data.sLow0(i,:)] = DwPhyTest_AdjustInputPower( Pout_dBm, AbsPwrH, 1.0, ...
        'SetRxPower = 0','Verbose = 0');
    data.PrdBmLow0(i) = Pout_dBm - data.Ploss(i);
    DwPhyLab_DisableESG;
end

%% Measure Gain with LNAGAIN = 0, LNAATTENEN = 1
Pout_dBm = PrdBm + data.Ploss(1);
for i=1:length(fcMHz),
    if ~UpdateWaitBar('LNAGAIN = 0, LNAATTENEN = 1', fcMHz(i), 0.75+i/N), break; end
    SetChannel( fcMHz(i) );
    DwPhyLab_WriteRegister(         80, 6, 0); % LNAGAIN = 0
    DwPhyLab_WriteRegister(RadioRegLNA, 0, 1); % LNAATTENEN = 1
    DwPhyLab_WriteRegister(         74,    0); % clear StepLNA
    
    DwPhyLab_EnableESG;
    [Pout_dBm, data.sLow1(i,:)] = DwPhyTest_AdjustInputPower( Pout_dBm, AbsPwrH, 1.0, ...
        'SetRxPower = 0','Verbose = 0');
    data.PrdBmLow1(i) = Pout_dBm - data.Ploss(i);
    DwPhyLab_DisableESG;
end

%% Calculate Gain Steps
if ishandle(hBar),
    for i=1:length(fcMHz),
        data.StepLNA0dB(i,:) = (data.sHigh(i,:) - data.sLow0(i,:)) + (data.PrdBmLow0(i) - data.PrdBmHigh(i));
        data.StepLNA1dB(i,:) = (data.sHigh(i,:) - data.sLow1(i,:)) + (data.PrdBmLow1(i) - data.PrdBmHigh(i));
        data.StepLNAATTENdB(i,:) = data.StepLNA1dB(i,:) - data.StepLNA0dB(i,:);
        data.mVstd(i,:) = (1000/1024)*(1/1.01) * 2.^((data.sHigh(i,:) - 3)/6);
        data.dBr(i,:) = (data.sHigh(i,:) - data.sHigh(1,:)) ...
            - 1.5*(data.InitAGain(i) - data.InitAGain(1)) ...
            -     (data.PrdBmHigh(i) - data.PrdBmHigh(1));
    end
end
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;


%% FUNCTION: UpdateWaitBar
function ok = UpdateWaitBar(msg, fcMHz, x)
if evalin('caller', 'ishandle(hBar)')
    cmd = sprintf('waitbar(%f, hBar, {''%s'',''Measuring at %d MHz''});',x, msg, fcMHz);
    evalin('caller',cmd);
    ok = 1;
else
    evalin('caller','data.Result = -1');
    fprintf('*** TEST ABORTED ***\n');
    ok = 0;
end


%% FUNCTION: SetChannel
function SetChannel(fcMHz)
DwPhyLab_SetRxFreqESG(fcMHz);
DwPhyLab_SetChannelFreq(fcMHz);

if DwPhyLab_RadioIsRF22
    DwPhyLab_WriteRegister(3, 2);      % enable RXB path only; enabling both paths will generate erroneous results
else
    DwPhyLab_WriteRegister(3, 3);      % enable both RX paths (baseband)
    DwPhyLab_WriteRegister(256+2, 3);  % enable both RX paths (radio)
end

DwPhyLab_WriteRegister(4,1);       % 802.11a mode
DwPhyLab_WriteRegister(103, 2, 0); % disable signal detect
DwPhyLab_WriteRegister(197, 4, 0); % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister(77, 0);     % clear RSSI calibration
DwPhyLab_WriteRegister(80, '7:6', 3); % fixed gain with LNAGAIN=1
DwPhyLab_Wake; 
DwPhyMex('OS_Delay',1000); % 1 ms delay

%% REVISIONS
% 2010-10-12 [SM]: Add support for RF22 registers
