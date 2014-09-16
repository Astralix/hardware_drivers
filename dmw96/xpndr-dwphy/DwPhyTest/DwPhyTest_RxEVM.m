function data = DwPhyTest_RxEVM(varargin)
% data = DwPhyTest_RxEVM(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 80;
fcMHz = 2484;
PacketType = 0;
Npts = 20;
PrdBm = -70;
ScopeSetup = 1;
hBar = [];
UseWaitBar = 1;
hBarClose = 1;
PktsPerWaveform = 1;
T_IFS = 50e-6;
Broadcast = 1;
DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','Npts','ScopeSetup','PrdBm'});

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
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
data.Leakage = [];
data.Flatness = [];
data.EVMdB = [];
data.W_EVMdB = [];
data.AvgEVMdB = NaN;
%wiseMex('wiParse_Line()','CalcEVM_OFDM.DCXSelect = 1');

if ScopeSetup,
    if isempty(who('VppScope')), VppScope = 0.2; end
    RxScopeSetup(1, VppScope, VppScope/8, 'AUTO');
end

DwPhyLab_LoadPER(Mbps, LengthPSDU, 10*PktsPerWaveform, PktsPerWaveform, Broadcast, ...
    0, T_IFS, 'Continuous = 1');
DwPhyLab_SetRxPower(PrdBm + data.Ploss)
[data.VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude([],'C1');

if UseWaitBar && ~ishandle(hBar),
    fprintf('*** TEST ABORTED ***\n');
    data.Result = -1;
    return;
end

% Change triggering to normal and wait for changes
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'ARM; WAIT 10.0');
DwPhyLab_SendCommand(scope, 'TRIG_MODE SINGLE; ARM; WAIT 5.0');
DwPhyLab_CloseScope(scope);

BadData = 0;
while length(data.EVMdB) < Npts && ~BadData,

    [result, z, Fs] = DwPhyLab_GetEVM([],0,'MeasureRxEVM=1');
    
    data.EVMdB   = [data.EVMdB result.EVMdB];
    data.W_EVMdB = [data.W_EVMdB result.W_EVMdB];
    data.AvgEVMdB   = 10*log10( mean(10.^(data.EVMdB/10)) );
    BadData = any(isnan(data.EVMdB)) || isempty(result.EVMdB);

    if ~isempty(who('Concise')) && ~Concise,
        data.LastResult = result;
        data.AdjustScopeAmplitude = tracelog;
    end
    
    if UseWaitBar,
        if ishandle(hBar)
            w = hBarOfs + hBarLen * min(1,(length(data.EVMdB)/Npts));
            hBarStr{hBarStrIndex} = sprintf('EVM = %1.1f dB',data.AvgEVMdB);
            waitbar(w,hBar,hBarStr);
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
end
DwPhyLab_TxBurst(0); 
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar) && hBarClose, close(hBar); end;

%% RxScopeSetup
function RxScopeSetup(Reset, Vpp, Vtrigger, TrigMode)

if nargin<4, TrigMode = []; end
if nargin<3, Vtrigger = []; end
if nargin<2, Vpp = []; end
if nargin<1,
    Reset = 1;
    Vtrigger = 0.05;
    TrigMode = 'AUTO';
end
    
scope = DwPhyLab_OpenScope;
if isempty(scope), error('Unable to open oscilloscope'); end

if Reset,
    DwPhyLab_SendCommand(scope, '*RST');
end

DwPhyLab_SendCommand(scope, 'C1:TRACE ON');
DwPhyLab_SendCommand(scope, 'C2:TRACE ON');
DwPhyLab_SendCommand(scope, 'C3:TRACE ON');
DwPhyLab_SendCommand(scope, 'C4:TRACE ON');

DwPhyLab_SendCommand(scope, 'C1:VDIV 0.1V; ATTN 1');
DwPhyLab_SendCommand(scope, 'C2:VDIV 0.1V; ATTN 1');
DwPhyLab_SendCommand(scope, 'C1:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C2:OFFSET 0.0');

DwPhyLab_SendCommand(scope, 'C3:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C3:BWL 200MHz');
DwPhyLab_SendCommand(scope, 'C3:COUPLING D50');
DwPhyLab_SendCommand(scope, 'C3:VDIV 1.0');

DwPhyLab_SendCommand(scope, 'C4:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C4:VDIV 1V');
DwPhyLab_SendCommand(scope, 'C4:COUPLING D1M');

DwPhyLab_SendCommand(scope, 'TIME_DIV 200E-6');
DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 1M');

DwPhyLab_SendCommand(scope, 'TRIG_SELECT EDGE,SR,C3');
DwPhyLab_SendCommand(scope, 'C3:TRIG_LEVEL 0.5');
DwPhyLab_SendCommand(scope, 'C3:TRIG_COUPLING DC');
if DwPhyLab_Parameters('ScopeIsWavePro7200'),
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY -800e-6');
else   
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY 10.0 PCT');
end

if ~isempty(TrigMode),
    DwPhyLab_SendCommand(scope, sprintf('TRIG_MODE %s',TrigMode));
end

DwPhyLab_CloseScope(scope);

%% REVISIONS
% 2010-02-19 Added alternate TRIG_DELAY command if ScopeIsWavePro7200