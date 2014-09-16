function [data,z,Fs] = DwPhyLab_GetEVMb(z, Fs, varargin)
%[data,z] = DwPhyLab_GetEVMb(z, varargin)
%
%   Capture a waveform from the oscilloscope and compute EVM for 802.11b.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc. All Rights Reserved

if nargin<1, z = []; end

% Standard results
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);

DwPhyLab_EvaluateVarargin(varargin);

%% EVM Measurement
if isempty(z),
    [x,T] = DwPhyLab_GetScopeWaveform('C3');
    [y, Fs] = ConvertToBaseband(x, T, 70e6);
    z = ParsePackets(y);
    data.scope.x = x;
    data.scope.y = y;
    data.scope.T = T;
end
data.scope.z = z;
data.scope.Fs = Fs;

if ~isempty(z),

    for i=1:length(z),
        result = DwPhyLab_CalcEVM_11b(z{i}, Fs);
        result = CheckRamp(z{i}, Fs, result);

        if 1,
            data.VerrMax(i) = result.VerrMax;
            data.PowerRamp(i) = result.PowerRamp;
        else
            data.VerrMax(i) = NaN;
            data.PowerRamp(i) = NaN;
        end

        data.Fs = Fs;
        if exist('Concise','var') && ~Concise,
            SaveResult{i} = result;
        end
    end
    
    if exist('Concise','var') && ~Concise,
        data.result = SaveResult;
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
y = y - mean(y);       % remove offsets from final waveform

%% FUNCTION: CheckRamp
function data = CheckRamp(z, Fs, data)
[B,A] = butter(7,2*11e6/Fs);
y = filter(B,A,z);
[B,A] = butter(2,2*1e6/Fs);
a = filter(B,A,abs(y));
apk = max(a);
A = mean(a(a > 0.5*apk));
a = a/A;
L = length(a);
k1 = find(a >= 0.1, 1 );
k2 = find(a >= 0.9, 1, 'last' );
data.TRampUp = find(a(k1:L) >= 0.9, 1, 'first') / Fs;
data.TRampDn = find(a(k2:L) >= 0.1, 1, 'last' ) / Fs;
data.PowerRamp = data.TRampUp < 2e-6 & data.TRampDn < 2e-6;

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
N  = length(find(d==1));
z  = {};

for i = 1:N,
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
% 2008-09-24: Removed eMax
% 2010-11-24: [SM] Bug fix in ParsePackets() for cell array z 