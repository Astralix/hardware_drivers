function data = DwPhyTest_InitAGain(varargin)
% data = DwPhyTest_InitAGain(...)
%
%   Compute InitAGain settings per frequency band

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
[fcMHz, Band] = DwPhyTest_ChannelList;
PrdBm = -65;
AbsPwrH = 56;
InitAGain = 40;
Verbose = 0;

DwPhyLab_EvaluateVarargin(varargin);
RxMode = 1;  %#ok<NASGU>...force 802.11a mode

%% Test Setup
DwPhyTest_RunTestCommon;
DwPhyLab_WriteRegister(197,   4,  0); % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister( 67, 127    ); % AbsPwrH = 127: no attenuation from AGC
DwPhyLab_WriteRegister( 80,'7:6', 3); % fixed gain with LNAGAIN=1
DwPhyLab_WriteRegister(103,   2,  1); % disable signal detect

data.RegList = DwPhyLab_RegList;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;

%% Loop through channels
for i=1:length(fcMHz),

    if ishandle(hBar)
        waitbar(i/length(fcMHz),hBar,sprintf('Computing InitAGain at %d MHz...',fcMHz(i)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end

    DwPhyLab_Sleep;
    DwPhyLab_SetRxPower(PrdBm + data.Ploss(i));
    DwPhyLab_SetRxFreqESG(fcMHz(i));
    DwPhyLab_SetChannelFreq(fcMHz(i) );
    DwPhyLab_Wake; pause(0.01);

    DwPhyLab_EnableESG;
    [data.InitAGain(i), data.MsrPwr(i,:), data.Vrms(i,:)] = TuneInitAGain( AbsPwrH, InitAGain, Verbose );
    InitAGain = data.InitAGain(i);
    DwPhyLab_DisableESG;
end

%% Compute InitAGain By Band
if ishandle(hBar),
    data.InitAGainByBand = NaN * [0:8];
    if length(data.fcMHz) == length(Band),
        for b = min(Band):max(Band),
            a = sort( data.InitAGain(Band == b) );
            if length(a) > 2,
                a = a(2:length(a));
            end
            data.InitAGainByBand(b+1) = a(1);
        end
    end
    close(hBar);
end
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

%% FUNCTION: TuneInitAGain
function [InitAGain, MsrPwr, Vrms] = TuneInitAGain(AbsPwrH, InitAGain, Verbose)
Pwr100dBm = DwPhyLab_ReadRegister(77, '4:0');
InitAGain = max(0, min(40, InitAGain));

if Verbose, fprintf('Searching...'); end

% step up gain
for AGain = InitAGain:40,
    DwPhyLab_WriteRegister(65, AGain); 
    MsrPwr = 4/3 * (DwPhyLab_AverageRSSI) + Pwr100dBm;
    if Verbose, fprintf('%d(%1.1f) ',AGain,max(MsrPwr)); end
    if max(MsrPwr) > (AbsPwrH - 1),
        break;
    end
end
InitAGain = AGain;

% step down gain
while (InitAGain > 0) && (max(MsrPwr) > (AbsPwrH + 0.5)),
    InitAGain = InitAGain - 1;
    DwPhyLab_WriteRegister(65, InitAGain); 
    MsrPwr = 4/3 * (DwPhyLab_AverageRSSI) + Pwr100dBm;
    if Verbose, fprintf('%d(%1.1f) ',InitAGain,max(MsrPwr)); end
end
if Verbose, fprintf('\n'); end

RSSI = (MsrPwr - Pwr100dBm) * 3/4;
Vrms = (1000/1024)*(1/1.01)*2.^((RSSI - 3 + 3/4*Pwr100dBm)/6);

%% REVISIONS
% 2008-07-28 Added Vrms calculation/reporting
%            Report MsrPwr and Vrms for both receive paths
