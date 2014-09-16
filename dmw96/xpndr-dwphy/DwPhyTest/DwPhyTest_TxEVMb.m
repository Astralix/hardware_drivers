function data = DwPhyTest_TxEVMb(varargin)
% data = DwPhyTest_TxEVMb(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 2;
LengthPSDU = 40;
fcMHz = 2412;
PacketType = 1;
Npts = 1;
ScopeSetup = 1;
hBar = [];
UseWaitBar = 1;
hBarClose = 1;
SkipSetup = 0;
Concise = 1;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','Npts','ScopeSetup','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 63;
end

%% Get Waitbar Data or Create a New One
if UseWaitBar

    hBarOfs = 0;
    hBarLen = 1;
    hBarStr = {};

    if isempty(hBar),
        hBar = waitbar(0.0,'Measuring EVM...','Name',mfilename,'WindowStyle','modal');
    else
        if ishandle(hBar),
            hBarClose = 0;
            UserData = get(hBar,'UserData');
            if length(UserData) == 2,
                hBarOfs = UserData(1);
                hBarLen = UserData(2);
            end
            hBarStr = get(get(get(hBar,'Children'),'Title'),'String');
            if ischar(hBarStr)
                clear hBarStr;
                hBarStr{1} = get(get(get(hBar,'Children'),'Title'),'String');
            end
        end
    end
    hBarStrIndex = length(hBarStr) + 1;
end

%% Run Test
if ~SkipSetup
    DwPhyTest_RunTestCommon;

    if ScopeSetup,
        if isempty(who('VppScope')), VppScope = 1.0; end
        DwPhyTest_ScopeSetup(1, VppScope, VppScope/8, 'AUTO');
    end
    DwPhyTest_ConfigSpectrumAnalyzer('EVM');

    DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, TXPWRLVL, PacketType)
    pause(0.1);
    [data.VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude;
else
    data.Timestamp = datestr(now);
end

if UseWaitBar && ~ishandle(hBar),
    fprintf('*** TEST ABORTED ***\n');
    data.Result = -1;
    return;
end

data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
data.VerrMax = [];
data.PowerRamp = [];

% Change triggering to normal and wait for changes
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'ARM; WAIT 10.0');
DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM; ARM; WAIT 5.0');
DwPhyLab_CloseScope(scope);

BadData = 0;
while length(data.VerrMax) < Npts && ~BadData,

    [result] = DwPhyLab_GetEVMb([],[],sprintf('Concise = %d',Concise));

    data.VerrMax = [data.VerrMax result.VerrMax];
    data.PowerRamp=[data.PowerRamp result.PowerRamp];
    data.Margin_dB = 20*log10(0.35 / max(data.VerrMax));

    if ~isempty(who('Concise')) && ~Concise,
        data.LastResult = result;
        if exist('tracelog','var'),
            data.AdjustScopeAmplitude = tracelog; 
        end
    end
    
    if UseWaitBar,
        if ishandle(hBar)
            w = hBarOfs + hBarLen * min(1,(length(data.VerrMax)/Npts));
            hBarStr{hBarStrIndex} = sprintf('Max Verr = %1.2f',max(data.VerrMax));
            waitbar(w,hBar,hBarStr);
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
end
DwPhyLab_TxBurst(0); 
if any(data.VerrMax >= 0.35) || any(data.PowerRamp == 0), data.Result = 0; end
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar) && hBarClose, close(hBar); end;

%% REVISIONS
% 2008-09-23 Increased default LengthPSDU from 30 to 40
% 2008-09-24 Removed eMax
% 2010-08-04 Use DwPhyLab functions to control the scope