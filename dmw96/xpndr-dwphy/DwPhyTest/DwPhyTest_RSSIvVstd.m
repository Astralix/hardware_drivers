function data = DwPhyTest_RSSIvVstd(varargin)
% data = DwPhyTest_RSSIvVstd(...)
%
%    *** THIS FUNCTION WAS USED TO CHARACTERIZE RSSI MEASUREMENTS
%    *** IT IS NOT SUPPORTED AS PART OF THE NORMAL DWPHYTEST

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters

fcMHz = 2412;
PrdBm = -90:1:-60;
UserReg = {};

DwPhyLab_EvaluateVarargin(varargin);

data.fcMHz = fcMHz;
data.PrdBm = PrdBm;
data.Ploss_dB = DwPhyLab_RxCableLoss(data.fcMHz);
data.UserReg = UserReg;

%% TestSetup
DwPhyLab_Initialize;
DwPhyLab_SetDiversityMode(3);
DwPhyLab_WriteUserReg(UserReg);
DwPhyLab_DisableESG;

DwPhyLab_Sleep;
DwPhyLab_WriteRegister(3,3);       % enable both RX paths
DwPhyLab_WriteRegister(256+2,3);   % enable both RX paths
DwPhyLab_WriteRegister(4,1);       % 802.11a mode
DwPhyLab_WriteRegister(103, 2, 0); % disable signal detect
DwPhyLab_WriteRegister(197, 4, 0); % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister(77, 0);     % clear RSSI calibration
DwPhyLab_WriteRegister(80, bin2dec('11000000')); % fixed gain
DwPhyLab_WriteRegister(74, 0); % StepLNA = 0

DwPhyLab_SetRxFreqESG(data.fcMHz);
DwPhyCheck( DwPhyMex('DwPhy_SetChannelFreq', data.fcMHz) );

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;
DwPhyLab_SetRxPower(-60+data.Ploss_dB);
DwPhyLab_EnableESG; pause(0.1);
DwPhyLab_Wake;
data.RegList = DwPhyLab_RegList;
data.Pwr100dBm = DwPhyLab_ReadRegister(77);

o = DwPhyLab_OpenScope;

%% Loop through power range
for i=1:length(data.PrdBm)

    PrdBm = data.PrdBm(i);
    
    if ishandle(hBar)
        waitbar(i/length(data.PrdBm),hBar,sprintf('Measuring RSSI at power at %1.2f dBm...',PrdBm));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
    
    DwPhyLab_SetRxPower(PrdBm + data.Ploss_dB); pause(0.1);
    data.RSSI(i,:) = DwPhyLab_AverageRSSI;
    [V1, Units, State] = o.GetParameterValue('C1','SDEV','V','');
    [V2, Units, State] = o.GetParameterValue('C2','SDEV','V','');
    data.Vstd(i,:) = [V1 V2];
end

data.MsrPwr = 4/3 * (data.RSSI + 3/4*data.Pwr100dBm);

if ishandle(hBar), close(hBar); end;
o.Disconnect;
DwPhyLab_DisableESG;

%% FUNCTION: GetScopeMeasurement
function Vstd = GetScopeMeasurement

h = DwPhyLab_OpenScope;
tic; [d,s1,s2] = h.GetParameterValue('C1','SDEV','',''); toc

h.Disconnect;
