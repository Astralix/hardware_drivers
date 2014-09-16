function data = DwPhyLab_CalcEVM_11b(s, Fs, param)
% data = DwPhyLab_CalcEVM_11b(s, Fs, param)
%        s = Complex signal samples
%        Fs = sample rate (must be at least 11 MHz)
%
%        Condition to pass 802.11b is data.VerrMax < 0.35

% Written by Barrett Brickner
% Copyright (c) 2007-2008 DSP Group, Inc. All rights reserved

%% Error Checking
if Fs < 11e6, error('Sample rate must be at least 11 MHz. Note that Fs has units of Hz.'); end

%% Default Parameters
PhaseTracking =  1; % Enable carrier phase tracking (DPLL)
SymbolDelay   = 44; % TimeDelay[microseconds] = SymbolDelay / 11
M = 20;
if nargin ==3,
    if isfield(param,'SymbolDelay'),   SymbolDelay   = param.SymbolDelay; end
    if isfield(param,'M'),             M             = param.M;           end
    if isfield(param,'PhaseTracking'), PhaseTracking = param.PhaseTracking; end
end

%% Trim to remove leading and trailing whitespace
t = 0.25 * max(abs(s));           % threshold is 25% of peak amplitude
k1 = find(abs(s)>t, 1, 'first');  % first point above threshold
k2 = find(abs(s)>t, 1, 'last');   % last point above threshold
s = s(k1:k2);                     % trim waveform

%% Reshape to a column vector
s = reshape(s,length(s),1);

%% Adjust Sample Rate and Coarse Timing
y  = resample(s, M*11e6, Fs); % Resample to M times the chip rate
dt = TimingOffset(y, M);      % Calculate timing phase
k1 = round(M+dt);             % starting sample index
y = y(k1:length(y));          % drop k1-1 samples so timing starts at index 0

%% CFO Estimation and Correction
L  = length(y);                        % waveform length
k1 = M * max(1,SymbolDelay-1);         % start of estimation sequence
k = k1:M:L;                            % coarse timing for estimation sequence
a = angle(y(k));                       % compute the QPSK angle for the sampled  waveform
da = diff(a);                          % compute the differential angle
d = cumsum(mod(da+pi/4,pi/2) - pi/4);  % phase error (difference not due to modulation)
P = polyfit((1:length(d))',d,1);       % linear fit to the cumulative phase error
w = P(1)/M;                            % CFO is the slope of d
a = w * reshape(1:L, size(y));         % phase rotation to correct
y = y .* exp(-j*a);                    % correct phase rotation due to CFO
CFOkHz = (M*11e3)*w/(2*pi);            % convert to frequency

%% Correct Average Phase Rotation
L = 1000;                       % number of symbols to use
k = M:M:min([length(y), M*L]);  % sample indices
a = angle(y(k));                % angle for the QPSK data
d = a(1) - pi/4;                % rotation to put first symbol at pi/4
phi = mean(mod(a-d,pi/2) + d);  % compute average rotation for L symbols
y = y .* exp(-j*(phi-pi/4));    % rotate QPSK to pi/4, 3pi/4, 5pi/4, 7pi/4

%% Gain Scaling and Carrier Phase Tracking
A = 1/mean(abs(y(k))); % level for Baud samples
y = sqrt(2)*A*y;       % gain adjustment so samples are at {±1±j}
if PhaseTracking,
    y = CarrierPhaseTracking(y,M);
end

%% Fine Timing Adjust
dt = TimingOffset(y,M);        % timing phase adjustment
k1 = M*SymbolDelay + 1;        % offset initial sample by SymbolDelay
k2 = length(y) - ceil(dt);     % adjust final sample to be within the original array "y"
t = (k1:k2) + dt;              % phase-adjusted timing
y = interp1(1:length(y),y, t); % interpolate to the desired timing

%% EVM Calculation
k = M:M:length(y);                   % sample times
u = y(k);                            % downsample to the chip rate
c = Demapper(u);                     % ideal data
e = u - c;                           % error vector
EVMdB = 10*log10(mean(abs(e).^2)/2); % rms error in dB

%% Verr Calculation
P = 2;                         % average signal power
Idc = real(u) - mean(real(u)); % "Q" term with DC removed
Qdc = imag(u) - mean(imag(u)); % "I" term with DC removed
Imag = mean(abs(Idc));         % average "I" term amplitude
Qmag = mean(abs(Qdc));         % average "Q" term amplitude 
Verr = sqrt( ((abs(Idc)/Imag - 1).^2 + (abs(Qdc)/Qmag - 1).^2) / P);

%% Pack Data
data.Fs = Fs;
data.M  = M;
data.s  = s;
data.y  = y;
data.a  = a;
data.u  = u;
data.e  = e;
data.dt = dt;
data.c = c; % addded 11/24/08
data.SymbolDelay = SymbolDelay;
data.CFOkHz = CFOkHz;
data.Verr = Verr;
data.EVMdB = EVMdB;
data.VerrMax = max(Verr);
data.VerrMargin_dB = 20*log10(0.35/data.VerrMax);

%% TimingOffset
%  Compute the timing offset to sample the DQPSK data. The offset is computed as the
%  center between the average zero-crossing using the first 1000 samples at oversample
%  rate M
function dt = TimingOffset(y,M)
N = min([length(y) 1000*M]);  % limit to first 1000T
z = real(y(1:N));             % select one quadrature component
a = sign(real(z));            % sign of y
k = find(diff(a) ~= 0);       % find zero-crossings
k = k(2:(length(k)-1));       % drop end-points
m = abs(z(k+1) - z(k));       % slope at zero-crossing
x = abs(z(k)) ./ m;           % distance to zero-crossing (linear interpolation)
b = mod(k-k(1),M);            % sample phase for zero-crossing
if abs(mode(b)-M/2) < M/4,    % check for phase distribution centered between 0 and M
    dk = mean(b) + M/2;       % ...and shift M/2 to peaks
else                          % otherwise
    b = mod(k-k(1)+M/2,M);    % ...compute phase with M/2 shift
    dk = mean(b);             % ...and compute the average without an additional M/2 shift
end
dt = mean(x) + dk + k(1);     % fine positioning
dt = mod(dt+M, M);            % select phase within one period

%% DQPSK Demapper
function c = Demapper(r)
A = pi/2;                          % angle between (D)QPSK symbols
a = angle(r);                      % PSK angle
p = A*round((a(1)-pi/4)/A)+(pi/4); % intial phase
d = (pi/2)*round(diff(a)/(pi/2));  % DQPSK angle
a(1) = p;
a(2:(length(r))) = p+cumsum(d);    % reconstruct angle vector from DQPSK demapping
c = sqrt(2) * exp(j*a);            % convert back to {±1±j} cartesian form

%% Carrier Phase Tracking
function y = CarrierPhaseTracking(y,M)
L = length(y);                 % length of input/output array
k = M:M:L;                     % sample indices
a = Demapper(y(k));            % ideal DQPSK points
q = y(k).*conj(a./(1+j));      % rotate all points back to 1+j
p = angle(q);                  % phase rotation
p0 = p(1)-pi/4;                % initial phase error
dp = mod(diff(p),pi/2);        % differential phase
i  = find(dp > pi/4);          % phase discontinuities due to QPSK symbol change
dp(i) = dp(i) - pi/2;          % remove QPSK phase changes
p = cumsum([p0; dp]);          % cumulative phase error vs time
a = 2^-2; b = 2^-5;            % proportional and integral gain terms
p = decimate  (p,11);          % filter and downsample to 1 MHz rate (Barker code rate)
i = downsample(k,11);          % indices to match p
i = i - 11*M;                  % implies one symbol processing delay
Bz = [1 -2 1];                 % H(z) = Az(z)/Bz(z)
Az = [1 (a+b-2) (1-a)];        % ...2nd order PLL transfer function
e  = filter(Bz,Az,p);          % residual phase error
a  = p - e;                    % phase error to correct
a  = interp1(i,a,1:L,'nearest','extrap'); % interpolate phase back to original sequence
a  = reshape(a, size(y));      % match shape of y
y = y .*exp(-j*a);             % correct phase on input signal

%% REVISIONS
% 2008-08-12 Decimated phase error to 1 Mbps Barker code rate. The previous version ran at
%            symbol rate which introduced too much noise in the phaes error detector.
%            Decimation provides filtering to mimic averaging that occurs during Barker
%            decoding. Decreased loop bandwidth from (a,b)=(1,3) to (2,5) to match the
%            current Mojave tuning.
% 2008-09-04 - Removed 1 symbol delay from k0 in carrier sense. The Barker correlator
%              and subsequent filter delays the start sufficiently.
%            - Perform coarse timing synchronization using only symbols in the duration
%              of the short preamble + header (96 µs).
%            - Adjust k0 after time shift of dt coarse timing sync
%            - Start at k0 when estimating average phase rotation
%            - AGC only within short PLCP preamble
% 2008-09-29 - Removed eMax calculation
%            - Clarified Verr calculation to follow nomenclature in IEEE 802.11
%            - Improved timing synchronization
% 2008-10-01 - Rewrote to work with continuous DQPSK (no Barker coding)
% 2008-11-25 - Automatically reshape input array to a column vector
%              Modify TimingOffset function to check for phase calculation that stradles
%              the sample interval and recompute with a T/2 offset.
