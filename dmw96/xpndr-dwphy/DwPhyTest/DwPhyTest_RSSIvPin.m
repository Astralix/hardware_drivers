function data = DwPhyTest_FixedGainRSSIvPin(varargin)
%data = DwPhyTest_FixedGainRSSIvPin(...)
%
%   Measure RSSI across a range of input power with fixed radio gain. Note that
%   typical mapping from RSSI to input power is Pr(dBm) = RSSI - 100.
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include fcMHz, fcMHzOfs, PrdBm, and nESG.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;
fcMHzOfs = 0;
PrdBm = -90:0.5:-30;
nESG = 1;

DwPhyLab_EvaluateVarargin(varargin);

%% TestSetup
DwPhyTest_RunTestCommon;
DwPhyLab_Sleep;

DwPhyLab_WriteRegister(3,3);       % enable both RX paths (baseband)
DwPhyLab_WriteRegister(256+2,3);   % enable both RX paths (radio)
DwPhyLab_WriteRegister(4,1);       % 802.11a mode
DwPhyLab_WriteRegister(103, 2, 0); % disable signal detect
DwPhyLab_WriteRegister(197, 4, 0); % RSSI in units of 3/4 dB (for better precision)
DwPhyLab_WriteRegister(80, bin2dec('11000000')); % fixed gain
data.RegList   = DwPhyLab_RegList;
data.Pwr100dBm = DwPhyLab_ReadRegister(77);
data.Ploss_dB  = DwPhyLab_RxCableLoss(fcMHz);

% set both ESGs to low output power
try DwPhyLab_SetRxPower(1, -136); catch end
try DwPhyLab_SetRxPower(2, -136); catch end

DwPhyLab_SetRxFreqESG(nESG, fcMHz + fcMHzOfs);
DwPhyLab_SetChannelFreq(fcMHz);

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;
DwPhyLab_SetRxPower(PrdBm(1)+data.Ploss_dB);
DwPhyLab_EnableESG; pause(0.1);
DwPhyLab_Wake;

%% Loop through power range
for i=1:length(data.PrdBm)

    if ishandle(hBar)
        waitbar( i/length(data.PrdBm), hBar, ...
            sprintf('Measuring RSSI at power at %1.2f dBm...',data.PrdBm(i)) );
    else
        fprintf('*** TEST ABORTED ***\n');
        break;
    end
   
    DwPhyLab_SetRxPower(nESG, data.PrdBm(i) + data.Ploss_dB); pause(0.1);
    data.RSSI(i,:) = DwPhyLab_AverageRSSI;
    data.mVstd(i,:) = (1000/1024)*(1/1.01) * 2.^((data.RSSI(i,:) - 3 + 3/4*data.Pwr100dBm)/6);
end

DwPhyLab_SetRxPower(nESG, -136);
if ishandle(hBar), close(hBar); end;

data.MsrPwr = 4/3 * (data.RSSI + 3/4*data.Pwr100dBm);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
