function [data,z,Fs] = DwPhyLab_GetEVM(z, Fs, varargin)
%[data,z] = DwPhyLab_GetEVM(z, varargin)
%
%   Capture a waveform from the oscilloscope and compute EVM. The default EVM
%   calculator in wiseMex is used for the computation.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc. All Rights Reserved

if nargin<1, z = []; end

DwPhyLab_EvaluateVarargin(varargin);

% Standard results
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);

%% EVM Measurement
if isempty(z),
    if exist('MeasureRxEVM','var') && MeasureRxEVM,
        [xI  ] = DwPhyLab_GetScopeWaveform('C1');
        [xQ,T] = DwPhyLab_GetScopeWaveform('C2');
        x = xI + 1i*xQ;
        [y, Fs] = ConvertToBaseband(x, T, 0);
    else
        [x,T] = DwPhyLab_GetScopeWaveform('C3');
        [y, Fs] = ConvertToBaseband(x, T, 70e6);
    end
    z = ParsePackets(y);
    data.scope.x = x;
    data.scope.y = y;
    data.scope.T = T;
end
data.scope.z = z;
data.scope.Fs = Fs;

if ~isempty(z),

    for i=1:length(z),
        result = wiseMex_CalcEVM(z{i}, 1/Fs);

        if ~result.Fault
            data.Fault(i) = 0;
            data.EVMdB(i) = result.EVMdB;
            data.W_EVMdB(i) = result.W_EVMdB;
            [data.Flatness(i) data.Leakage(i)] = CheckFlatness(result.h0);
        else
            data.Fault(i) = result.Fault;
            data.EVMdB(i) = NaN;
            data.W_EVMdB(i) = NaN;
            data.Flatness(i) = NaN;
            data.Leakage(i) = NaN;
        end

        data.result{i} = result; %%%% DEBUG
        data.Fs = Fs;

    end
else
    % no packets found to process
    data.Fault = -1;
    data.EVMdB = [];
    data.W_EVMdB = [];
end
   
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

%% FUNCTION: ConvertToBaseband
function [y, Fs] = ConvertToBaseband(x, T, fc)
% downmix to baseband
if fc,
    k = reshape(1:length(x), size(x));
    w = exp(-j*2*pi*fc*T*k);
    x = x.*w;
end

% downsample to 80 MS/s
Fs = 80e6;
p = Fs/1e6;            % output rate in MHz
q = round(1/T/1e6);    % input rate (round to avoid small floating point errors)
y = resample(x, p, q); % sample rate conversion

%% FUNCTION: CheckFlatness
function [Flatness, Leakage] = CheckFlatness(h0)
k = -32:31;
p = abs(h0).^2;
i = find(abs(k) >= 1 & abs(k) <= 16);
j = find(abs(k) >= 1 & abs(k) <= 26);
p0 = mean(p(i));     % average in subcarriers +/-1 to +/-16
d = 10*log10(p/p0);  % deviation
Flatness = 1;
if max(abs(d(i))) >  2, Flatness = 0; end
if max(   (d(j))) >  2, Flatness = 0; end
if min(   (d(j))) < -4, Flatness = 0; end %#ok<FNDSB>

pLO = 10*log10( p(33) / mean(p(j)));
if pLO < 2, Leakage = 1; else Leakage = 0; end

%% FUNCTION: ParsePackets
function [z,a,d] = ParsePackets(y)
[B,A] = butter(1, 2*0.1/80);
a = filtfilt(B,A, abs(y));   % low-pass filtered amplitude profile
a = a - min(a);              % reduce noise floor
t = max(a) / 2;              % comparator threshold
a = a>t;                     % logical over-under threshold
d = diff(a);                 % differentiate to find start/end

Lifs = round(10e-6 * 80e6);  % length of minimum interframe space
Lpad = round( 8e-6 * 80e6);  % packet start/end pad length

k0 = Lifs;                   % packet search start position
L  = length(d) - 80;         % waveform length less 1 usec
z  = {};

for i=1:length(find(d==1)),
    k1 = k0 + find(d(k0:L) == 1, 1); % find packet start
    if isempty(k1), break; end       % no more packets
    k2 = k1 + find(d(k1:L) ==-1, 1); % find end of packet
    if isempty(k2), break; end       % incomplete packet (no end)

    k0 = k2 + Lifs;                  % update start of earch for next packet
    k1 = max(k1 - Lpad, 1);          % pad start of packet
    k2 = min(k2 + Lpad, length(y));  % pad end of packet
    z{i} = y(k1:k2);                 % record the waveform
end

%% REVISIONS
% 2008-08-03: Added OFDM flatness check
% 2008-08-05: Eliminated DC removal from downconverted signal to allow LO estimation
% 2008-10-23: Fixed carrier leakage; check carrier index 33 instead of 32
% 2008-11-03: Fixed flatness test per Kyle
% 2009-03-13: Fixed LO test to normalize by all subcarriers
% 2009-03-16: Fixed error in the flatness test (it was too strict)
