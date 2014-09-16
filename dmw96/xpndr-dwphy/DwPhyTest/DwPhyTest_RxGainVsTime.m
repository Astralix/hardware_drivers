function data = DwPhyTest_RxGainVsTime(varargin)
% data = DwPhyTest_RxGainVsTime(...)
%
%   Measure receiver gain over time following sleep-to-wake

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2484;
PrdBm = -62;
TimeSleep = 60;    % duration of sleep phase (seconds)
TimeDuration = 60; % duration of measurement phase (seconds)

DwPhyLab_EvaluateVarargin(varargin);
RxMode = 1;  %#ok<NASGU>...force 802.11a mode

%% Test Setup
DwPhyTest_RunTestCommon;
tic; DwPhyLab_Sleep; % begin sleep phase for cool-down
DwPhyLab_WriteRegister(197,   4,  0); % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister( 67, 127    ); % AbsPwrH = 127: no attenuation from AGC
DwPhyLab_WriteRegister( 80,'7:6', 3); % fixed gain with LNAGAIN=1
DwPhyLab_WriteRegister(103,   2,  1); % disable signal detect

data.RegList = DwPhyLab_RegList;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;
DwPhyLab_SetRxPower(PrdBm + data.Ploss);
DwPhyLab_SetRxFreqESG(fcMHz);

%% Wait for Cool-Down
while toc < TimeSleep,
    if ishandle(hBar)
        waitbar(toc/TimeSleep, hBar, sprintf('Sleep phase (%1.0f s)...',TimeSleep));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
    pause(1);
end

%% Power-up and Gain Measurement
i = 0;
tic; DwPhyLab_Wake;
while toc < TimeDuration,
    i = i + 1;
    data.RSSI(i,:) = DwPhyLab_AverageRSSI;
    data.t(i) = toc;

    if ishandle(hBar)
        waitbar(data.t(i)/TimeDuration, hBar, sprintf('RSSI = %1.1f dBm (%1.1f dB)', ...
            data.RSSI(i,1) - 100, data.RSSI(i,1) - data.RSSI(1,1)) );
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end

DwPhyLab_Sleep;
DwPhyLab_DisableESG;

if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

