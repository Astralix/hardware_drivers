function data = DwPhyTest_TuneDPLL(varargin)
%data = DwPhyTest_TuneDPLL(...)
%
%   Measure sensitivity for all DPLL loop gain values (primary DPLL, not the pilot
%   tone PLL). By default, tuning is performed for the carrier phase tracking loop.
%   To select timing phase, set TimingDPLL = 1.
%
%   Input arguments are strings of the form '<param name> = <param value>'
%   Valid parameters include Mbps, fcMHz...

% Written by Barrett Brickner
% Copyright 2008-2011 DSP Group, Inc., All Rights Reserved.

BasebandID = DwPhyLab_ReadRegister(1);

%% Default and User Parameters
if BasebandID < 7, 
    Mbps = 54;
    LengthPSDU = 1500;
    PrdBm = -78:-60;
else
    Mbps = 65;
    LengthPSDU = 4095;
    PrdBm = -72:-50;
end
fcMHz = 2412;
PktsPerWaveform = 10;
TimingDPLL = 0;
aRange = 0:4;
bRange = 0:7;

DwPhyLab_EvaluateVarargin(varargin);


%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
hBar = waitbar(0.0,'Initializing','Name',mfilename,'WindowStyle','modal');

%%% Load the PER waveform
waitbar(0.0,hBar,''); set(hBar,'UserData',[0.0 0.2]);
DwPhyLab_LoadPER( data, hBar );
if ~ishandle(hBar), fprintf('*** TEST ABORTED ***\n'); return; end

argList{1} = sprintf('Concise = 1');
argList{2} = sprintf('fcMHz = %g',fcMHz);
argList{3} = sprintf('Mbps = %g',Mbps);
argList{4} = sprintf('SensitivityLevel = 0.1');
argList{5} = sprintf('MinPER = 0.05');
argList{6} = sprintf('PrdBm = %g:%g:%g',min(PrdBm),mean(diff(PrdBm)),max(PrdBm));

for a = aRange,
    for b = bRange;
        x = 8*a + b;
        if TimingDPLL,
            data.SaSb(a+1,b+1) = x;
            if BasebandID < 7,
                for i=0:15, DwPhyLab_WriteRegister(32+i, x); end
            else
                DwPhyLab_WriteRegister(32, x);
            end
        else
            data.CaCb(a+1,b+1) = x;
            if BasebandID < 7,
                for i=0:15, DwPhyLab_WriteRegister(48+i, x); end
            else
                DwPhyLab_WriteRegister(33, x);
            end
        end
        if ~ishandle(hBar), 
            fprintf('*** TEST ABORTED ***\n'); 
            return;
        else
            waitbar(0.1+0.9*x/40,hBar,{'',sprintf('Measuring with a = %d, b = %d',a,b)}); 
            set(hBar,'UserData',[0.1+0.9*x/64 1/64]);
        end
        data.result{a+1,b+1} = DwPhyTest_SensitivityCore(argList, varargin, hBar);
        data.Sensitivity(a+1,b+1) = data.result{a+1,b+1}.Sensitivity;
    end
end

if ishandle(hBar), close(hBar); end;
DwPhyLab_DisableESG;

%% Summary Results
[I,J] = find(data.Sensitivity == min(min(data.Sensitivity)));
if TimingDPLL,
    data.OptSaSb = [aRange(I) bRange(J)];
else
    data.OptCaCb = [aRange(I) bRange(J)];
end
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

