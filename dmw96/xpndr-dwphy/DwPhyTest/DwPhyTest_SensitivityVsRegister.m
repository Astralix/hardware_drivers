function data = DwPhyTest_SensitivityVsRegister(varargin)
% data = DwPhyTest_SensitivityVsRegister(varargin)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, fcMHz...
%
%    To specify a register field and range use the parameters RegAddr, RegField, and
%    RegRange. These are supplied to DwPhyLab_WriteRegister.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
fcMHz = 2412;
MaxPkts = 1000;
MinFail = 100;
PktsPerWaveform = 1;

DwPhyLab_EvaluateVarargin(varargin);

if isempty(who('RegAddr')) || isempty(who('RegRange')),
    error('Must specify at least RegAddr and RegRange');
end
if isempty(who('RegField')), RegField = '7:0'; end
if isnumeric(RegField), RegField = sprintf('%d:%d',RegField,RegField); end

PktsPerSequence = max( 1000, 10*PktsPerWaveform);

%% Run Test
DwPhyTest_RunTestCommon;
hBar = waitbar(0.0,'Initializing','Name',mfilename,'WindowStyle','modal');
data.Ploss   = DwPhyLab_RxCableLoss(fcMHz);

%%% Load the PER waveform
waitbar(0.0,hBar,''); set(hBar,'UserData',[0.0 0.2]);
DwPhyLab_LoadPER( Mbps, LengthPSDU, PktsPerWaveform, PktsPerSequence, varargin, data, hBar );

argList{1} = sprintf('Concise = 1');
argList{2} = sprintf('MaxPkts = %d',MaxPkts);
argList{3} = sprintf('MinFail = %d',MinFail);
argList{4} = sprintf('SensitivityLevel = 0.1');
argList{5} = sprintf('MinPER = 0.05');
argList{6} = sprintf('PrdBm = -82:-50');

for i = 1:length(RegRange),
    
    DwPhyLab_WriteRegister(RegAddr, RegField, RegRange(i));
    
    if ~ishandle(hBar), 
        fprintf('*** TEST ABORTED ***\n'); 
        break;
    else
        waitbar( i/length(RegRange), hBar, ...
            sprintf('Measuring with Reg[%g][%s] = %d',RegAddr,RegField,RegRange(i)) );
        set(hBar,'UserData',[0.1+0.9*(i-1)/length(RegRange) 1/64]);
    end
    data.result{i} = DwPhyTest_SensitivityCore(argList, varargin, hBar);
    data.Sensitivity(i) = data.result{i}.Sensitivity;
end

DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
