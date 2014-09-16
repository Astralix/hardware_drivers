function data = DwPhyTest_SensitivityVsChanl(varargin)
% data = DwPhyTest_SensitivityVsChanl(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters are fcMHz (an array), Verbose, and any parameters
%    that can be passed to DwPhyTest_Sensitivity.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Verbose = 1;
fcMHz = DwPhyTest_ChannelList;
Mbps = 54;
LengthPSDU = 1000;
PktsPerWaveform = 1;
Broadcast = 1;
MinPER = 0.05; %#ok<NASGU>
MinFail = 200; %#ok<NASGU>
PrdBm = -80:-50; %#ok<NASGU>
RxMode = 1;

DwPhyLab_EvaluateVarargin(varargin);
PktsPerSequence = max( 1000, 10*PktsPerWaveform);

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);

if isempty(who('options')), options = {}; end
if isempty(who('ShortPreamble')), ShortPreamble = 1; end
if isempty(who('T_IFS')), T_IFS = []; end
DwPhyLab_LoadPER(Mbps, LengthPSDU, PktsPerSequence, PktsPerWaveform, Broadcast, ...
    ShortPreamble, T_IFS, options);

for i=1:length(fcMHz),

    DwPhyLab_Sleep
    DwPhyLab_SetRxFreqESG(fcMHz(i));
    DwPhyLab_SetChannelFreq(fcMHz(i));
    DwPhyLab_SetRxMode(RxMode);
    if ~isempty(who('DiversityMode'))
        DwPhyLab_SetDiversityMode(DiversityMode);
    end
    if ~isempty(who('UserReg'))
        DwPhyLab_WriteUserReg(UserReg);
    end
    DwPhyLab_Wake;

    fcMHzString = sprintf('fcMHz = %d;\n',data.fcMHz(i));
    data.result{i} = DwPhyTest_SensitivityCore(data, varargin, fcMHzString);
    data.Sensitivity(i) = data.result{i}.Sensitivity;
    
    if data.result{i}.Result == -1,
        data.Result = -1;
        break;
    end
    if Verbose,
        fprintf('   %2d:%d, %1.1f dBm\n',i,length(fcMHz),data.result{i}.Sensitivity);
    end
   
end
DwPhyLab_DisableESG;
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
