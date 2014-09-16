function data = DwPhyTest_ThSwitchLNA(varargin)
% data = DwPhyTest_ThSwitchLNA(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, fcMHz, PrdBmL, PrdBmH, and PktsPerWaveform

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
fcMHz = 2412;
PrdBmL = -60:-30;
PrdBmH = -10:-1:-60;

DwPhyLab_EvaluateVarargin(varargin);

%% Build SenitivityCore Argument List
argList{1} = sprintf('fcMHz = %g',fcMHz);
argList{2} = sprintf('Mbps = %g',Mbps);
argList{3} = sprintf('SensitivityLevel = 0.1');
argList{4} = sprintf('Concise = 1');

argListH = argList;
argListH{length(argListH)+1} = sprintf('PrdBm = %g:%g:%g',PrdBmH(1),mean(diff(PrdBmH)),PrdBmH(length(PrdBmH)));

argListL = argList;
argListL{length(argListL)+1} = sprintf('PrdBm = %g:%g:%g',PrdBmL(1),mean(diff(PrdBmL)),PrdBmL(length(PrdBmL)));

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss  = DwPhyLab_RxCableLoss(fcMHz);

hBar = waitbar(0.0,'Initializing','Name',mfilename,'WindowStyle','modal');

%%% Load the PER waveform
waitbar(0.0,hBar,''); set(hBar,'UserData',[0.0 0.2]);
DwPhyLab_LoadPER( data, hBar );
if ~ishandle(hBar), fprintf('*** TEST ABORTED ***\n'); return; end

%%% High-Side Sensitivity with LNAGAIN = 1
waitbar(0.2,hBar,{'','Measuring with LNAGAIN = 1'}); set(hBar,'UserData',[0.2 0.4]);
DwPhyLab_WriteRegister(73,63);
data.resultH = DwPhyTest_SensitivityCore(argListH, varargin, hBar);
if ~ishandle(hBar), fprintf('*** TEST ABORTED ***\n'); return; end

%%% Low-Side Sensitivity with LNAGAIN = 0
waitbar(0.6,hBar,{'','Measuring with LNAGAIN = 0'}); set(hBar,'UserData',[0.6 0.4]);
DwPhyLab_WriteRegister(73, 0);
data.resultL = DwPhyTest_SensitivityCore(argListL, varargin, hBar);

DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;

%% Summary Results
data.SensitivityL = data.resultL.Sensitivity;
data.SensitivityH = data.resultH.Sensitivity;
data.CenterPrdBm  = mean([data.SensitivityL, data.SensitivityH]);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

