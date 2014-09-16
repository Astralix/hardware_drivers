function data = DwPhyTest_MsrPwr_ESGvPSA
% This is a utility function written to measure power on the PSA from the 
% ESG as a function of channel. It was written to verify functionality for
% actual device tests

% Standard results
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);
data.Result = 0;

%% TestSetup
DwPhyLab_Sleep;
DwPhyLab_DisableESG;

data.fcMHz = DwPhyTest_ChannelList;

data.fcMHz = 1000:20:6000;

Pin_dBm = -40;
data.Ploss = DwPhyLab_RxCableLoss(data.fcMHz);

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');
DwPhyLab_LoadWaveform_LTF;
Pout_dBm = Pin_dBm + data.Ploss(1);
DwPhyLab_SetRxPower(Pout_dBm);

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
    
    DwPhyLab_SetTxFreqPSA(data.fcMHz(i))
    DwPhyLab_SetRxFreqESG(data.fcMHz(i));
    pause(0.2);

    data.PdBmHigh(i) = Pout_dBm;     % record value
    data.MsrPwr(i) = DwPhyLab_MsrChanlPwrPSA;

end
DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;
