function data = DwPhyTest_BasicIO
% DwPhyTest_BasicIO - Simple baseband/radio interface check

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% Standard results
DwPhyLab_Setup;
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);
data.Result = 1;

if DwPhyLab_RadioIsRF22
    RadioRegAGain = 256 + 6;
    RadioRegMC    = 256 + 8; 
else
    RadioRegAGain = 256 + 6;
    RadioRegMC    = 256 + 7;     
end

%% Check LNAGAIN, AGAIN[5:0]
DwPhyLab_Sleep; 
DwPhyLab_Initialize;
DwPhyLab_Wake; % need to be awake for FixedGain tests

DwPhyLab_WriteRegister(80, 7, 1);
for LNAGAIN = 0:1,
    DwPhyLab_WriteRegister(80, 6, LNAGAIN);
    if LNAGAIN ~= DwPhyLab_ReadRegister(RadioRegAGain, 7),
        data.Result = 0;
        fprintf('### %s> Miscompare for LNAGAIN = %d\n',mfilename, LNAGAIN);
    end
end
for AGAIN = 0:63,
    DwPhyLab_WriteRegister(65, AGAIN);
    dAGAIN = DwPhyLab_ReadRegister(RadioRegAGain, '6:1');
    if AGAIN ~= dAGAIN
        data.Result = 0;
        fprintf('### %s> Miscompare for AGAIN = %d (%d)\n',mfilename, AGAIN, dAGAIN);
    end
end
    

%% Check MC[1:0]
DwPhyLab_Sleep; 
DwPhyLab_Initialize;

for MC = 0:3,
    DwPhyLab_WriteRegister(224, MC+4*MC+16*MC+64*MC);
    MCOUT = DwPhyLab_ReadRegister(RadioRegMC, '5:4');
    if MC ~= MCOUT,
        data.Result = 0;
        fprintf('### %s> Miscompare for MC = %d (read %d)\n',mfilename, MC, MCOUT);
        return;
    end
end


%% Check Sleep/WakePhy (exercises software control of DW_PHYEnB)
DwPhyLab_Sleep;
DwPhyLab_Initialize; 

DwPhyLab_Sleep; pause(0.1); 
data.SleepMC = DwPhyLab_ReadRegister(RadioRegMC, '5:4');

DwPhyLab_Wake; pause(0.1); 
data.WakeMC  = DwPhyLab_ReadRegister(RadioRegMC, '5:4');

if (data.SleepMC ~= 0) || (data.WakeMC ~= 3),
    data.Result = 0;
    fprintf('### %s> Miscompare for Sleep/WakePhy\n',mfilename);
    return;
end

%% REVISIONS
% 2010-10-12 [SM]: Add support for RF22 registers
