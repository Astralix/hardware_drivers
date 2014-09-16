function data = DwPhyTest_RxGainProfile(varargin)
%data = DwPhyTest_RxGainProfile(...)
%   RX Gain Profile (all LNAGAIN, AGAIN settings)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.


%% Default and User Parameters
fcMHz = 2412;
AbsPwrH = 56;

DwPhyLab_EvaluateVarargin(varargin);

%% TestSetup
DwPhyTest_RunTestCommon;

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');

DwPhyLab_WriteRegister(3,3);     % enable both RX paths (baseband)
DwPhyLab_WriteRegister(256+2,3); % enable both RX paths (radio)
DwPhyLab_WriteRegister(4,1);     % 802.11a mode
DwPhyLab_WriteRegister(103, 2, 0); % disable signal detect
DwPhyLab_WriteRegister(197, 4, 0); % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister(77, 0);        % clear RSSI calibration
DwPhyLab_WriteRegister(80, '7:6', 3); % fixed gain with LNAGAIN=1
DwPhyLab_WriteRegister(74, 0);  % StepLNA = 0

data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
Pout_dBm = -65 + data.Ploss;

DwPhyLab_LoadWaveform_LTF;
DwPhyLab_SetRxPower(Pout_dBm);
DwPhyLab_EnableESG; 
pause(0.1);

ListLNAGAIN = [1 0 2];
ListAGAIN = 40:-1:0;

i = 0;
for LNAGAIN = ListLNAGAIN,
    for AGAIN = ListAGAIN;
        i = i+1;
        data.LNAATTENEN(i) = bitand(LNAGAIN, 2) / 2;
        data.LNAGAIN(i)    = bitand(LNAGAIN, 1);
        data.AGAIN(i)      = AGAIN;
    end
end
N = length(data.AGAIN);

DwPhyLab_WriteRegister(80, 6, data.LNAGAIN(1));
DwPhyLab_WriteRegister(65,    data.AGAIN(1));
Pout_dBm = DwPhyTest_AdjustInputPower(Pout_dBm, AbsPwrH, 1.0, [-80 min(10, data.Ploss)], 'Verbose = 0');  % adjust input power

%% Loop through settings
for i=1:N,

    if ishandle(hBar)
        waitbar(i/N,hBar,sprintf('Measuring at LNAGAIN=%d, AGAIN=%d...',data.LNAGAIN(i),data.AGAIN(i)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end

    DwPhyLab_WriteRegister(256+95, 0, data.LNAATTENEN(i)); % Set LNAATTENEN
    DwPhyLab_WriteRegister(    80, 6, data.LNAGAIN(i)   ); % Set forced LNAGAIN
    DwPhyLab_WriteRegister(    65,    data.AGAIN(i)     ); % Set forced AGAIN

    [Pout_dBm, data.AvgRSSI(i,:)] = DwPhyTest_AdjustInputPower(Pout_dBm, AbsPwrH, 1, [-80 min(10, data.Ploss)], 'Verbose = 0');
    data.PrdBm(i) = Pout_dBm - data.Ploss;     % record value
    Pout_dBm = Pout_dBm + 1.5;   % reduce for next gain step

end
DwPhyLab_DisableESG;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
