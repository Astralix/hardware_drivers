function data = DwPhyTest_TxPowerRamp(varargin)
%data = DwPhyTest_TxPowerRamp(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1500;
fcMHz = 2412;
PacketType = 0;
MeasurePower = 0;
TXPWRLVL = 63;
TrigSource = 3;
CaptureAll = 1;
mVdivFine = 1;
DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
if ~exist('Concise','var'), Concise = 1; end

hBar = waitbar(0.0,{'',''}, 'Name',mfilename, 'WindowStyle','modal');

% Run packets to set temperature and adapt ALC
DwPhyLab_TxBurst(1e9,data.Mbps,1500,TXPWRLVL,0);
ScopeSetup(TrigSource, 'AUTO');
if ~isfield(data,'PADGAIN')
    pause(1);
    PCOUNT = DwPhyLab_ReadRegister(256+77,'7:1');
    data.PADGAIN = 2*PCOUNT;
end
DwPhyLab_TxBurst(0);

if ~DwPhyLab_RadioIsRF22
    DwPhyLab_WriteRegister(256+65,0,1); % enable PADGAIN override so output power is constant
    DwPhyLab_WriteRegister(256+67,data.PADGAIN);
end

if MeasurePower,
    opt{1} = sprintf('TXPWRLVL = %d; UseExistingConfig = 1;',TXPWRLVL);
    result = DwPhyTest_TxPower(data, opt);
    data.Pout_dBm = result.Pout_dBm;
    data.Attn = max(0, 3 + ceil(result.Pout_dBm - result.Ploss));
    DwPhyTest_ConfigSpectrumAnalyzer('EVM');
else
    DwPhyTest_ConfigSpectrumAnalyzer('EVM');
    data.Attn = 20;
    data.Pout_dBm(1) = NaN;
end
data.Attn = 6;
DwPhyLab_SendCommand('PSA',':SENSE:POWER:RF:ATTENUATION %d dB',data.Attn);

DwPhyLab_TxBurst(1e9, data.Mbps, 1500, data.TXPWRLVL, 0);
pause(2); % give enough time for packet generation
[data.VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude;

% Change triggering to normal and wait for changes
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'C3:VDIV %1.4fV',data.VppScope/8);
DwPhyLab_SendCommand(scope, 'TRIG_LEVEL %1.4fV',data.VppScope/4); % 6 dB below peak
DwPhyLab_SendCommand(scope, 'ARM; WAIT 10.0');
DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM; ARM; WAIT 5.0');
DwPhyLab_CloseScope(scope);

data.scope{1} = ReadWaveforms(CaptureAll, TrigSource);

dAttndB = 0;
dA = 10^(dAttndB/20);

% Change amplitude and wait for changes
DwPhyLab_SendCommand('PSA',':SENSE:POWER:RF:ATTENUATION %d dB',data.Attn-dAttndB);
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, sprintf('C3:VDIV %dmV', mVdivFine)); pause(0.5);
DwPhyLab_SendCommand(scope,'ARM; WAIT 10.0');
DwPhyLab_SendCommand(scope,'TRIG_MODE NORM; ARM; WAIT 5.0');
DwPhyLab_CloseScope(scope);

data.scope{2} = ReadWaveforms(CaptureAll, TrigSource);
DwPhyLab_TxBurst(0);

[data.wfm{1},w,Ar,Ap,Aq] = ProcessWaveform(data.scope{1});
[data.wfm{2}]            = ProcessWaveform(data.scope{2}, w, Ar*dA, Ap*dA, Aq*dA);

if ishandle(hBar)
    set(hBar,'UserData',[(i-1) 1]/length(TXPWRLVL));
else
    fprintf('*** TEST ABORTED ***\n'); 
    data.Result = -1;
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% ProcessWaveform
function [dataOut,w,Ar,Ap,Aq] = ProcessWaveform(scope,w,Ar,Ap,Aq)

FsMHz = scope.FsMHz;
s = scope.C3;

if nargin<2,
    k=1:length(s); 
    k=reshape(k,size(s)); 
    w=exp(-j*2*pi*70/FsMHz*k); 
end

[B,A] = butter(7,2*10/FsMHz);

x = w.*s;
y = filtfilt(B,A,x);
r = resample(y,20,FsMHz);
a = abs(r);               
k16 = 17:length(r);

b = ones(32,1);
p =     filter(b,length(b), a(k16).*     a(k16-16));
q = abs(filter(b,length(b), r(k16).*conj(r(k16-16))));

%if nargout >= 3,
    z = filtfilt(ones(4,1), 4, a.^2);
    th = max(z) / 2;
    k0 = find(z>th, 1 );
    k = (k0+16) : min(length(z), (k0+128));
    
    if nargin<3,
        Ar = sqrt(mean(abs(r(k).^2)));
    end
    z0 = mean(z(k));
    k0 = find(z/z0 >= 0.5, 1 );
%end

t = (1:length(z)); t=t-k0; t=t/20;

if nargin<4,
    th = max(p) / 2;
    k0 = find(p > th);
    k = (k0+16) : min(length(p), (k0+112));
    Ap = mean(p(k));
end
if nargin<5,
    th = max(q) / 2;
    k0 = find(q > th);
    k = (k0+16) : min(length(q), (k0+112));
    Aq = mean(q(k));
end
r = r/Ar;
p = p/Ap;
q = q/Aq;

dataOut.x = x;
dataOut.y = y;
dataOut.r = r;
dataOut.p = p;
dataOut.q = q;
dataOut.t = t;

%% ReadWaveforms
function data = ReadWaveforms(CaptureAll, TrigSource)
data.FsMHz = [];
data.T = [];
if CaptureAll,
    data.C1 = DwPhyLab_GetScopeWaveform('C1');
    data.C2 = DwPhyLab_GetScopeWaveform('C2');
end
[data.C3, data.T] = DwPhyLab_GetScopeWaveform('C3');
if CaptureAll || (TrigSource==4)
    data.C4 = DwPhyLab_GetScopeWaveform('C4');
end
data.FsMHz = round(1/data.T/1e6);
L = length(data.C3);
if TrigSource == 3,
    data.t = ((1:L) - round(0.4*L))/data.FsMHz;
else
    data.t = ((1:L) - round(0.2*L))/data.FsMHz;
end
data.t_units = 1e-6;

%% ScopeSetup
function ScopeSetup(TrigSource, TrigMode)
scope = DwPhyLab_OpenScope;
if isempty(scope), error('Unable to open oscilloscope'); end

DwPhyLab_SendCommand(scope, '*RST');

DwPhyLab_SendCommand(scope, 'TIME_DIV 2E-6');
DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 100K');

DwPhyLab_SendCommand(scope, 'C1:TRACE ON');
DwPhyLab_SendCommand(scope, 'C2:TRACE ON');
DwPhyLab_SendCommand(scope, 'C3:TRACE ON');
DwPhyLab_SendCommand(scope, 'C4:TRACE ON');

DwPhyLab_SendCommand(scope, 'C1:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C1:BWL 20MHz');
DwPhyLab_SendCommand(scope, 'C1:VDIV 0.1V');

DwPhyLab_SendCommand(scope, 'C2:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C2:COUPLING D1M');
DwPhyLab_SendCommand(scope, 'C2:VDIV 0.1V');

DwPhyLab_SendCommand(scope, 'C3:VDIV 100mV');
DwPhyLab_SendCommand(scope, 'C3:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C3:BWL 200MHz');
DwPhyLab_SendCommand(scope, 'C3:COUPLING D50');

DwPhyLab_SendCommand(scope, 'C4:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C4:COUPLING D1M');
DwPhyLab_SendCommand(scope, 'C4:VDIV 0.1V');

switch TrigSource
    case 3,
        DwPhyLab_SendCommand(scope, 'TRIG_SELECT GLIT,SR,C3,HT,PL,HV,5E-6');
        DwPhyLab_SendCommand(scope, 'TRIG_LEVEL 0.05V');
        DwPhyLab_SendCommand(scope, 'C3:TRIG_COUPLING DC');
        if DwPhyLab_Parameters('ScopeIsWavePro7200'),
            DwPhyLab_SendCommand(scope, 'TRIG_DELAY -2e-6');
        else   
            DwPhyLab_SendCommand(scope, 'TRIG_DELAY 10.0 PCT');
        end
    case 4,
        DwPhyLab_SendCommand(scope, 'TRIG_SELECT EDGE,SR,C4');
        DwPhyLab_SendCommand(scope, 'C4:TRIG_SLOPE NEG; C4:TRIG_COUPLING DC; TRIG_LEVEL 1.5V');
        if DwPhyLab_Parameters('ScopeIsWavePro7200'),
            DwPhyLab_SendCommand(scope, 'TRIG_DELAY -6e-6');
        else   
            DwPhyLab_SendCommand(scope, 'TRIG_DELAY 10.0 PCT');
        end
    otherwise, error('Unrecognized TrigSource');
end

if ~isempty(TrigMode),
    DwPhyLab_SendCommand(scope, sprintf('TRIG_MODE %s',TrigMode));
end

DwPhyLab_CloseScope(scope);

%% REVISIONS
% 2010-02-19 Added alternate TRIG_DELAY command if ScopeIsWavePro7200
% 2011-03-09 [SM] Modified for RF22.