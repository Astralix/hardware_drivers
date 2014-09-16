function data = DwPhyTest_MeasureRxCFO(varargin)
% data = DwPhyTest_MeasureRxCFO(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include fcMHz, fcMHzOfs, Npts, and PrdBm.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters
fcMHz = 2484;
fcMHzOfs = 0;
Npts = 200;
PrdBm = -65; % selected for good SNR but minimum AGC time (better CFO estimate)

DwPhyLab_EvaluateVarargin(varargin);

if isempty(who('RegAddr')) || isempty(who('RegRange')),
    SweepRegister = 0;
    RegRange = NaN;
else
    SweepRegister = 1;
end
if isempty(who('RegField')), RegField = '7:0'; end
if isnumeric(RegField), RegField = sprintf('%d:%d',RegField,RegField); end

data.RxMode = 1; % must use OFDM receiver to get results

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
hBar = waitbar(0.0,'Running...','Name',mfilename,'WindowStyle','modal');

% Load the test waveform.
% Choose low data rate long PSDU and small IFS to give a high duty cycle to
% (a) allow the use for the ESG ALC and (b) to improve the 'hit' rate for
% CFO capture from register (reset at the end of the packet).
DwPhyLab_SetRxFreqESG(fcMHz(1));
DwPhyLab_LoadPER(6, 1000, 1000, 1, 1, 1, 16e-6);

% Enable continuous waveform output and enable ALC
DwPhyLab_SendCommand('ESG',':SOURCE:RADIO:ARB:TRIGGER:TYPE CONTINUOUS');
DwPhyLab_SendCommand('ESG',':SOURCE:POWER:ALC:SEARCH:REFERENCE MODULATED');
DwPhyLab_SendCommand('ESG',':SOURCE:POWER:ALC:STATE ON');

n = 0;
N = length(fcMHz) * length(fcMHzOfs) * length(RegRange);

for k = 1:length(fcMHz)

    DwPhyLab_Initialize;
    DwPhyLab_SetRxPower(PrdBm + data.Ploss(k));
    DwPhyLab_SetChannelFreq(fcMHz(k));
    DwPhyLab_SetRxMode(data.RxMode);
    if ~isempty(who('UserReg'))
        DwPhyLab_WriteUserReg(UserReg);
    end
    if (k==1), data.RegList = DwPhyLab_RegList; end

    for i = 1:length(fcMHzOfs),
        f = fcMHz(k) + fcMHzOfs(i);

        for j = 1:length(RegRange),
            n = n + 1;
            
            if SweepRegister,
                DwPhyLab_WriteRegister(RegAddr, RegField, RegRange(j));
            end

            if ishandle(hBar),
                waitbar(0.1+0.9*n/N, hBar, sprintf('fc = %1.3f MHz',f));
            else
                fprintf('*** TEST ABORTED ***\n'); break;
            end

            DwPhyLab_SendCommand('ESG', ':SOURCE:FREQUENCY:CW %5.9fGHz',f/1000);
            DwPhyLab_Wake; pause(0.01);
            [data.CFOkHz(n), data.CFOppm(n), data.RawCFO] = MeasureCFO(Npts, f);
            DwPhyLab_Sleep;

            if fcMHzOfs(i) == 0,
                ppm = abs(data.CFOppm);
                if length(RegRange) == 1,
                    if (any(ppm > 25)) || (any(ppm > 20) && (fcMHz(k) > 4000)),
                        data.Result = 0; % fail
                    end
                end
            end
        end
    end
    if ~ishandle(hBar), break; end
end

DwPhyLab_DisableESG;
if ishandle(hBar), close(hBar); end;

%% Data Summary
data.AvgCFOppm = mean(data.CFOppm);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

if (length(fcMHzOfs) > 1) && (length(fcMHz) == 1),
    x = diff(data.CFOkHz);
    d = 1e3 * mean( diff(data.fcMHzOfs) );
    y = filtfilt([1 2 4 2 1],10,x);
    e = y-d;
    k1 = find(e>-0.1, 1 );
    k2 = find(e>-0.1, 1, 'last' );
    k = (k1+2):(k2-3);
    data.P = polyfit(data.fcMHzOfs(k)*1e3, data.CFOkHz(k), 1);
    data.z = data.P(2) + 1e3*data.fcMHzOfs;
    data.e = data.CFOkHz - data.z;
end    

%% FUNCTION MeasureCFO
function [CFOkHz, CFOppm, CFO] = MeasureCFO( Npts, fcMHz )

%%% Grab Npts unsynchronized measurements
CFO = zeros(Npts,1);
for i=1:Npts,
    CFO(i) = DwPhyLab_ReadRegister(193);
end

%%% Convert to proper signed value (underlying value is 2's complement
%%% reported as unsigned)
k = find(CFO >= 128);
CFO(k) = CFO(k) - 256;

%%% The data contains some spurious zeros where the CFO is reset between packets.
%%% Assume most points are good and take a histogram. Then find the peak of the
%%% distribution using a moving-average to handle cases where the peak lies between
%%% quantization levels.
x = -128:127;
n = hist(CFO,x);
y = filtfilt([1 2 1],4,n);
k = round(mean( find(y == max(y)) ));

%%% Now, take a weighted average around the peak to improve resolution
k = k + (-1:1);
if min(k) <   1, k=k+1; end;
if max(k) > 256, k=k-1; end;
AvgCFO = sum( n(k).*x(k) ) / sum(n(k));

%%% Convert from baseband units to kHz and ppm
CFOkHz = 20000/8192 * AvgCFO;
CFOppm = 1e6 * (CFOkHz*1e3) / (fcMHz*1e6);
