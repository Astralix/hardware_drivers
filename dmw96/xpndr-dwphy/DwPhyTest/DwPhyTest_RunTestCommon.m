% Common setup procedure for DwPhyTest. This is a script run at the start of test
% procedures where a common, repeatable initial condition is needed

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

%% Check for missing data structure
if isempty(who('data'))
    data = struct; % empty structure
end

%% Check for and supply missing data header
if ~isfield(data,'Description')
    DwPhyTest_RunTestCommon_S = dbstack;
    if length(DwPhyTest_RunTestCommon_S) >= 2,
        data.Description = DwPhyTest_RunTestCommon_S(2).name;
    end
    clear DwPhyTest_RunTestCommon_S;
end
if ~isfield(data,'Timestamp')
    data.Timestamp = datestr(now);
end
if ~isfield (data,'Result')
    data.Result = 1;
end

%% Check if configuration is required
if isempty(who('UseExistingConfig')) || UseExistingConfig == 0
    data.UseExistingConfig = 0;
    
    %% Configure DwPhyMex/R-VWAL and put the driver into test mode
    DwPhyLab_Setup;

    %% Load User Configuration Data
    DwPhyParameterData = DwPhyLab_Parameters('DwPhyParameterData');
    if ~isempty('DwPhyParameterData')
        data.DwPhyParameterData = DwPhyParameterData;
        if isfield(DwPhyParameterData,'ChipSetAlias')
            DwPhyLab_SetParameterData('DWPHY_PARAM_CHIPSET_ALIAS',   DwPhyParameterData.ChipSetAlias);
        end
        if isfield(DwPhyParameterData,'DefaultRegisters')
            DwPhyLab_SetParameterData('DWPHY_PARAM_DEFAULTREGISTERS',DwPhyParameterData.DefaultRegisters);
        end
        if isfield(DwPhyParameterData,'RegistersByBand')
            DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYBAND', DwPhyParameterData.RegistersByBand);
        end
        if isfield(DwPhyParameterData,'RegistersByChanl')
            DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL',DwPhyParameterData.RegistersByChanl);
        end
        if isfield(DwPhyParameterData,'RegistersByChanl24')
            DwPhyLab_SetParameterData('DWPHY_PARAM_REGISTERSBYCHANL24',DwPhyParameterData.RegistersByChanl24);
        end
        if isfield(DwPhyParameterData,'MsrPwrOfs56')
            DwPhyLab_SetParameterData('DWPHY_PARAM_MSRPWROFS56',DwPhyParameterData.MsrPwrOfs56);
        end
        if isfield(DwPhyParameterData,'RxSensitivity')
            DwPhyLab_SetParameterData('DWPHY_PARAM_RXSENSITIVITY',DwPhyParameterData.RxSensitivity);
        end
    end

    %% Initialize the PHY
    DwPhyLab_Sleep;
    DwPhyLab_Initialize;
    data.BasebandID = DwPhyLab_ReadRegister(1);
    data.RadioID    = DwPhyLab_ReadRegister(1+256);

    %% Configure the test channel
    if isempty(who('fcMHz'))
        DualBand = DwPhyLab_Parameters('DualBand');
        if ~isempty(DualBand) && DualBand,
            fcMHz = 5200;
        else
            fcMHz = 2412;
        end
        data.fcMHz = fcMHz;
    end
    if isempty(who('RunIQCal'))
        if DwPhyLab_RadioIsRF22(data)
            RunIQCal = 1;
        else
            RunIQCal = 0;
        end
    end
    DwPhyLab_SetChannelFreq(fcMHz(1), RunIQCal);

    if ~isempty(DwPhyLab_Parameters('E4440A_Address'))
        if isempty(who('fcMHzOfs'))
            DwPhyLab_SetTxFreqPSA(fcMHz(1))
        else
            DwPhyLab_SetTxFreqPSA(fcMHz(1) + fcMHzOfs(1));
        end
    end
    if ~isempty(DwPhyLab_Parameters('E4438C_Address1'))
        if isempty(who('fcMHzOfs'))
            DwPhyLab_SetRxFreqESG(fcMHz(1));
        else
            DwPhyLab_SetRxFreqESG(fcMHz(1) + fcMHzOfs(1));
        end
    end

    %% Configure RX Signal Generators for low output
    try DwPhyLab_SetRxPower(1, -136); catch end
    try DwPhyLab_SetRxPower(2, -136); catch end

    %% Program the Nuisance Packet Address Filter
    if ~isempty(who('AddressFilter'))
        DwPhyLab_AddressFilter(AddressFilter);
    end
    if ~isempty(who('StationAddress'))
        DwPhyLab_SetStationAddress(StationAddress);
    end

    %% Set RxMode
    if isempty(who('RxMode'))
        if any(fcMHz < 3000),
            DwPhyLab_SetRxMode(3 + 4*(data.BasebandID > 6)); % 802.11g/n for 2.4 GHz band
        else
            DwPhyLab_SetRxMode(1 + 4*(data.BasebandID > 6)); % 802.11a/n for 4-5 GHz band
        end
    else
        DwPhyLab_SetRxMode(RxMode);
    end

    %% Set DiversityMode
    if ~isempty(who('DiversityMode'))
        DwPhyLab_SetDiversityMode(DiversityMode(1));
    end

    %% Set the Minimum Sensitivity Level
    if ~isempty(who('SetRxSensitivity'))
        DwPhyLab_SetRxSensitivity(SetRxSensitivity(1));
        GetRxSensitivity = DwPhyLab_GetRxSensitivity;
    end

    %% User Register Overrides
    if ~isempty(who('UserReg'))
        DwPhyLab_WriteUserReg(UserReg);
    end

    %% Wake the PHY
    if exist('NoWake','var') && NoWake,
    else
        DwPhyLab_Wake;
    end
else
    data.UseExistingConfig = 1;
end

%% Record parameters and the current register settings
DwPhyLab_RecordParameters;
if isempty(who('Concise')) || ~Concise,
    data.RegList = DwPhyLab_RegList;
end

%% REVISIONS
% 2008-08-25 Record parameters/registers after waking the part so readback includes
%            any calibration values
% 2008-11-05 Modified to run with empty E4440A or E4438C address parameters
% 2011-03-07 Added 'RunIQCal' param for DwPhyLab_SetChannelFreq input
% 2011-03-15 Added 'UseExistingConfig' to bypass configurations 
% 2011-04-12 Added more options for 'RunIQCal'