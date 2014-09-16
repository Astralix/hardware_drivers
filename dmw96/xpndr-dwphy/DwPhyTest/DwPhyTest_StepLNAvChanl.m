function data = DwPhyTest_StepLNAvChanl


% Standard results
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);
data.Result = 0;

%% TestSetup
DwPhyLab_Sleep;
DwPhyLab_Initialize;
DwPhyLab_DisableESG;

DwPhyLab_WriteRegister(3,3);          % enable both RX paths (baseband)
DwPhyLab_WriteRegister(256+2,3);      % enable both RX paths (radio)
DwPhyLab_WriteRegister(4,1);          % 802.11a mode
DwPhyLab_WriteRegister(103, 2, 0);    % disable signal detect
DwPhyLab_WriteRegister(197, 4, 0);    % RSSI in units of 3/4 dB
DwPhyLab_WriteRegister(77, 0);        % clear RSSI calibration
DwPhyLab_WriteRegister(74, 0);        % stepLNA = 0
DwPhyLab_WriteRegister(80, '7:6', 3); % fixed gain with LNAGAIN=1

data.fcMHz = DwPhyTest_ChannelList;
AbsPwrH = 64;

data.Ploss = DwPhyLab_RxCableLoss(data.fcMHz);
data.RegList = DwPhyLab_RegList;

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;
Pout_dBm = -50;
DwPhyLab_SetRxPower(Pout_dBm);
DwPhyLab_WriteRegister(80, 6, 1); % LNAGAIN = 1
DwPhyLab_Wake;

DwPhyLab_SetRxFreqESG(data.fcMHz(1));
DwPhyLab_EnableESG; pause(0.1);

%% Loop through channels
for i=1:length(data.fcMHz),

    if ishandle(hBar)
        waitbar(i/length(data.fcMHz),hBar,sprintf('Measuring power at %d MHz...',data.fcMHz(i)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        return ;
    end
    
    DwPhyLab_SetRxFreqESG  (data.fcMHz(i));
    DwPhyLab_SetChannelFreq(data.fcMHz(i));
    DwPhyLab_WriteRegister(80, 6, 1); % LNAGAIN=1
    DwPhyLab_Wake;
    pause(0.2);

    Pout_dBm = DwPhyTest_AdjustInputPower(Pout_dBm, AbsPwrH, 0.5);  % adjust input power
    data.PdBm(i) = Pout_dBm;
    
    data.sHigh(i,:) = DwPhyLab_AverageRSSI;

    DwPhyLab_WriteRegister(80, 6, 0); % LNAGAIN=0
    data.sLow(i,:) = DwPhyLab_AverageRSSI;

end
DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;
