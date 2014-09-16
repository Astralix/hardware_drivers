function data = DwPhyTest_TxPacketTiming(varargin)
%data = DwPhyTest_TxPacketTiming(...)
%   Capture signals at the DAC and align to MC0 to show timing

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

DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
if ~exist('Concise','var'), Concise = 1; end

hBar = waitbar(0.0,{'',''}, 'Name',mfilename, 'WindowStyle','modal');

DwPhyLab_TxBurst(1e9, data.Mbps, 1500, data.TXPWRLVL, 0); pause(0.2);

% Change triggering to normal and wait for changes
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM; ARM; WAIT 5.0'); pause(0.5);
buf = DwPhyLab_SendQuery(scope, sprintf('%s:INSP? ''HORIZ_INTERVAL''','C1'));

k = findstr(buf,'HOR'); buf = buf( k :length(buf));
k = findstr(buf,':');   buf = buf(k+1:length(buf));
T = sscanf(buf,'%f');

N = 200;

for i=1:N,
    if i == 1,
        C4 = DwPhyLab_GetScopeWaveform('C4');
    end
    C1{i} = DwPhyLab_GetScopeWaveform('C1'); %#ok<AGROW>

    if ishandle(hBar)
        waitbar( i/(N*1.1), hBar );
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end

DwPhyLab_CloseScope(scope);

tic;
[data.MC0,data.DACI,data.k0,data.t] = ProcessWaveform(C1,C4,T);
disp(toc);
DwPhyLab_TxBurst(0);

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% ProcessWaveform
function [MC0,DACI,kPkt,t] = ProcessWaveform(C1,C4,T)

FsMHz = 400;
MC0  = resample(C4,FsMHz,round(1e-6/T));
k = 1:length(MC0);
k0 = find(MC0 < 1.5, 1 );
t = (k-k0)/FsMHz;

for i=1:length(C1),
    x = resample(C1{i},FsMHz,round(1e-6/T));

    a=abs(x);
    k0 = find(a>0.3*max(a), 1 );
    k = (k0+FsMHz)+(0:6*FsMHz);
    A0 = sqrt(mean(a(k).^2));
    k0 = find(a/A0 > 1, 1 );
    k = k0 + (0:4*FsMHz); % find a peak
    b = x(k)*sign(x(k0))/A0;
    k4 = find(b<=0, 1 ); % find the zero-crossing
    k = k0 + k4 - 1;
    if abs(x(k-1)) < abs(x(k)),
        k = k - 1;
    end

    DACI(:,i) = reshape(x,length(x),1);
    kPkt(i) = k;
end

%% ScopeSetup
function ScopeSetup
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

DwPhyLab_SendCommand(scope, 'TRIG_SELECT EDGE,SR,C4');
DwPhyLab_SendCommand(scope, 'C4:TRIG_SLOPE NEG; C4:TRIG_COUPLING DC; TRIG_LEVEL 1.5V');
if DwPhyLab_Parameters('ScopeIsWavePro7200'),
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY -6e-6');
else   
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY 20.0 PCT');
end
DwPhyLab_SendCommand(scope, 'TRIG_MODE AUTO');
DwPhyLab_CloseScope(scope);

%% REVISIONS
% 2010-02-19 Added alternate TRIG_DELAY command if ScopeIsWavePro7200
% 2010-08-04 Use DwPhyLab functions to control the scope