function data = DwPhyTest_SetParameterData
%data = DwPhyTest_SetParameterData
%   Test the programmable data interface in the DwPhy module.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Basic setup
data.Description = mfilename;
data.Timestamp = datestr(now);
data.Result = 1;

DwPhyLab_Setup;
DwPhyLab_Sleep;

%% DWPHY_PARAM_CHIPSET_ALIAS
DwPhyLab_Shutdown;
DwPhyLab_Startup;
DwPhyLab_SetDwPhyEnableFn;
data.chipset0 = DwPhyMex('DwPhy_ChipSet');
X = uint8([1 2 3 0]);
DwPhyLab_SetParameterData('DWPHY_PARAM_CHIPSET_ALIAS',X);
data.chipset1 = DwPhyMex('DwPhy_ChipSet');
if data.chipset1 ~= hex2dec('00030201'), data.Result = 0; end
DwPhyLab_SetParameterData('DWPHY_PARAM_CHIPSET_ALIAS',[]);
data.chipset2 = DwPhyMex('DwPhy_ChipSet');
if data.chipset2 ~= data.chipset0, data.Result = 0; end

%% DWPHY_PARAM_DEFAULTREGISTERS
DwPhyLab_Shutdown;
DwPhyLab_Startup;
DwPhyLab_SetDwPhyEnableFn;
DwPhyLab_Initialize;
data.Reg65_0 = DwPhyLab_ReadRegister(65);
data.Reg64_0 = DwPhyLab_ReadRegister(64);
data.Reg63_0 = DwPhyLab_ReadRegister(63);

X = uint8([65 0 5, 64 0 4, 63 0 3, 62 0 2, 61 0 1]);
DwPhyLab_SetParameterData('DWPHY_PARAM_DEFAULTREGISTERS', X);
DwPhyLab_Initialize;
data.Reg65_1 = DwPhyLab_ReadRegister(65);
data.Reg64_1 = DwPhyLab_ReadRegister(64);
data.Reg63_1 = DwPhyLab_ReadRegister(63);
if (data.Reg65_1 ~= 5) || (data.Reg64_1 ~= 4) || (data.Reg63_1 ~= 3),
    data.Result = 0;
    disp('Problem setting default registers');
end
    
DwPhyLab_SetParameterData('DWPHY_PARAM_DEFAULTREGISTERS', []);
DwPhyLab_Initialize;
data.Reg65_2 = DwPhyLab_ReadRegister(65);
data.Reg64_2 = DwPhyLab_ReadRegister(64);
data.Reg63_2 = DwPhyLab_ReadRegister(63);
if (data.Reg65_2 ~= data.Reg65_0) || (data.Reg64_2 ~= data.Reg64_0) || (data.Reg63_2 ~= data.Reg63_0)
    data.Result = 0;
    disp('Problem resetting default registers');
end

%% DWPHY_PARAM_REGISTERSBYBAND
if ~DwPhyLab_RadioIsRF22
    DwPhyLab_Shutdown;
    DwPhyLab_Startup;
    DwPhyLab_SetDwPhyEnableFn;
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(5200);
    data.Reg65_5200_0 = DwPhyLab_ReadRegister(65);
    X = uint8([65 0 255 16*ones(1,9)]);
    DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYBAND', X);
    DwPhyLab_SetChannelFreq(5200);
    data.Reg65_5200_1 = DwPhyLab_ReadRegister(65);
    if data.Reg65_5200_1 ~= 16,
        data.Result = 0;
        disp('Problem setting register band');
    end
    DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYBAND', []);
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(5200);
    data.Reg65_5200_2 = DwPhyLab_ReadRegister(65);
    if data.Reg65_5200_2 ~= data.Reg65_5200_0,
        data.Result = 0;
        disp('Problem resetting register band');
    end

    %% DWPHY_PARAM_REGISTERSBYCHANL
    DwPhyLab_Shutdown;
    DwPhyLab_Startup;
    DwPhyLab_SetDwPhyEnableFn;
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(5200);
    data.Reg81_5200_0 = DwPhyLab_ReadRegister(81);
    X = uint8([81 0 255 (255 - (0:255))]);
    DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL', X);
    DwPhyLab_SetChannelFreq(5200);
    data.Reg81_5200_1 = DwPhyLab_ReadRegister(81);
    if data.Reg81_5200_1 ~= (255-40),
        data.Result = 0;
        disp('Problem setting register by channel');
    end
    DwPhyLab_SetChannelFreq(2412);
    data.Reg81_2412_1 = DwPhyLab_ReadRegister(81);
    if data.Reg81_2412_1 ~= (255-241),
        data.Result = 0;
        disp('Problem setting register by channel');
    end
    DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL', []);
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(5200);
    data.Reg81_5200_2 = DwPhyLab_ReadRegister(81);
    if data.Reg81_5200_2 ~= data.Reg81_5200_0,
        data.Result = 0;
        disp('Problem resetting register by channel');
    end
end

%% DWPHY_PARAM_REGISTERSBYCHANL24
DwPhyLab_Shutdown;
DwPhyLab_Startup;
DwPhyLab_SetDwPhyEnableFn;
DwPhyLab_Initialize;
DwPhyLab_SetChannelFreq(2412);
data.Reg90_2412_0 = DwPhyLab_ReadRegister(90);
X = uint8( [ ...
    81 0 255 5*ones(1,14) ...
    90 0 255 (1:14) ...
    91 0  15 [9 5 9 5 9 5 9 5 9 5 9 5 9 5] ...
    ]);
DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL24', X);
DwPhyLab_SetChannelFreq(2412);
data.Reg90_2412_1 = DwPhyLab_ReadRegister(90);
if data.Reg90_2412_1 ~= 1,
    data.Result = 0;
    disp('Problem setting register by channel, 2.4');
end
DwPhyLab_SetChannelFreq(2484);
data.Reg90_2484_1 = DwPhyLab_ReadRegister(90);
if data.Reg90_2484_1 ~= 14,
    data.Result = 0;
    disp('Problem setting register by channel, 2.4');
end
DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL24', []);
DwPhyLab_Initialize;
DwPhyLab_SetChannelFreq(2412);
data.Reg90_2412_2 = DwPhyLab_ReadRegister(90);
if data.Reg90_2412_2 ~= data.Reg90_2412_0,
    data.Result = 0;
    disp('Problem resetting register by channel, 2.4');
end

%% Cleanup
DwPhyLab_Sleep;
DwPhyLab_Shutdown;
DwPhyLab_Startup;
DwPhyLab_SetDwPhyEnableFn;
DwPhyLab_Initialize;

%% REVISIONS
% 2008-03-17 Added DwPhyLab_Startup after DwPhyLab_Shutdown to follow new DwPhy
%            requirements. This code will not work with older version of DwPhyMex
% 2008-05-07 Sleep during cleanup
% 2008-08-28 Add test for RegByChanl24
% 2010-12-03 [SM] Bypass RegByChanl test for RF22
