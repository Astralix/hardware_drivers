function data = DwPhyTest_PllLock(Only24Band)
% DwPhyTest_PllLock - Check whether the PLL locks on all channels

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, Only24Band = 0; end
    
% Standard results
data.Description = mfilename('fullpath');
data.Timestamp = datestr(now);
data.Result = 0;

%% Initialize the baseband
DwPhyLab_Setup;
DwPhyLab_Initialize;

data.RadioID = DwPhyLab_ReadRegister(256+1);

if Only24Band,
    data.fcMHz = [2412:5:2472, 2484];
else
    data.fcMHz = DwPhyTest_ChannelList;
end
hBar = waitbar(0.0,'','Name',mfilename,'WindowStyle','modal');

for i=1:length(data.fcMHz),

    if ishandle(hBar)
        waitbar(i/length(data.fcMHz),hBar,sprintf('Checking PLL Lock at %d MHz...',data.fcMHz(i)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        return ;
    end
    
    DwPhyLab_Sleep;
    DwPhyCheck( DwPhyMex('DwPhy_SetChannelFreq', data.fcMHz(i)) );
    DwPhyLab_Wake; 
    
    if DwPhyLab_RadioIsRF22
        data.Lock(i) = DwPhyLab_ReadRegister(256+ 19, 2 );
        data.VCALDone(i) = DwPhyLab_ReadRegister(256+106, 7 );        
        data.CLCALDone(i) = DwPhyLab_ReadRegister(256+106, 6 );                
    else
        data.Lock(i) = DwPhyLab_ReadRegister(256+41, 7 );
        data.CAPSEL(i) = DwPhyLab_ReadRegister(256+42, '4:0');    
    end

    DwPhyLab_Sleep; pause(0.05); DwPhyLab_Wake; pause(0.01); % sleep/wake cycle
    
    if DwPhyLab_RadioIsRF22
        data.LockAfterWake(i) = DwPhyLab_ReadRegister(256+19, 2 );
        data.VCALDoneAfterWake(i) = DwPhyLab_ReadRegister(256+106, 7 );        
        data.CLCALDoneAfterWake(i) = DwPhyLab_ReadRegister(256+106, 6 );                    
    else
        data.LockAfterWake(i) = DwPhyLab_ReadRegister(256+41, 7 );
        data.CAPSELAfterWake(i) = DwPhyLab_ReadRegister(256+42, '4:0');    
    end

    if ~DwPhyLab_RadioIsRF22
        DwPhyMex('DwPhy_PllClosedLoopCalibration');
        data.LockAfterCLCAL(i) = DwPhyLab_ReadRegister(256+41, 7 );
        data.CAPSELAfterCLCAL(i) = DwPhyLab_ReadRegister(256+42, '4:0');
    end
end
if ishandle(hBar), close(hBar); end;

%% Check for Pass/Fail
if any(data.RadioID == [50 51 52]) % RF52A3xx and RF52A4xx
    data.Result = all(data.Lock == 1) & all(data.LockAfterCLCAL == 1);
elseif DwPhyLab_RadioIsRF22
    data.Result = all(data.Lock == 1) & ...
                  all(data.VCALDone == 1) & all(data.CLCALDone == 1) & ...
                  all(data.VCALDoneAfterWake == 1) & all(data.CLCALDoneAfterWake == 1);                  
else
    data.Result = all(data.Lock == 1) & all(data.LockAfterCLCAL == 1) & all(data.LockAfterWake == 1);
end

%% REVISIONS
% 2008-05-20 Record CAPSEL for each Lock test
% 2010-10-20 [SM]: Add support for RF22