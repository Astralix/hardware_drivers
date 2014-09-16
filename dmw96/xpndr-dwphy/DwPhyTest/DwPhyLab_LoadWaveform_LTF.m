function data = DwPhyLab_LoadWaveform_LTF(n)
% DwPhyLab_LoadWaveform_LTF(n) load a waveform that is simply repetitions
%    of the long preamble (legacy long training field) to signal generator n.
%    The particular sequence matches the WiSE Mojave baseband model, so
%    scaling is identical to that used for packet waveforms.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin < 1,
    n = [];
end

%%% Construct the DW52 format LTF
longR = [ 65,-2,17,40,9,25,-48,-16,41,22,0,-57,10,24,-9,50,26,15,-24,-55,34,29,-25,-23,-15,-51,-53,31,-1,-38,38,5,-65,5,38,-38,-1,31,-53,-51,-15,-23,-25,29,34,-55,-24,15,26,50,-9,24,10,-57,0,22,41,-16,-48,25,9,40,17,-2];
longI = [ 0,-50,-46,34,12,-36,-23,-44,-11,2,-48,-20,-24,-6,67,-2,-26,41,16,27,38,6,34,-9,-63,-7,-9,-31,22,48,44,41,0,-41,-44,-48,-22,31,9,7,63,9,-34,-6,-38,-27,-16,-41,26,2,-67,6,24,20,48,-2,11,44,23,36,-12,-34,46,50];
x = 2*(longR + j*longI);

%%% Replicate to get a reasonable length
x = [x x x x x x x x]; % replicas to get a reasonable length

%%% Form the DW52 upsample filter
b0 = [ 0,  0,  0,  0,  0,  0, 64,  0,  0,  0,  0,  0];
b1 = [ 0,  1, -2,  4, -7, 19, 57,-11,  5, -3,  1, -1];
b2 = [-1,  2, -3,  6,-12, 40, 40,-12,  6, -3,  2, -1];
b3 = [-1,  1, -3,  5,-11, 57, 19, -7,  4, -2,  1,  0];
b = [];
for i = 1:length(b0),
    b = [b b0(i) b1(i) b2(i) b3(i)]; %#ok<AGROW>
end

%%% Apply the filter and trim the pre-/post-cursor term to an integer number of periods.
%%% Then scale by 1/512 to give an output waveform on the range +/-1
y = conv(b/32, upsample(x,4));
k = 1:4*length(x);
k = k + mean(grpdelay(b));
y = y(k) / 512;

m = ones(2,length(y));
m(1,90:length(y)) = 0;

DwPhyLab_LoadWaveform(n, 80e6, y, m, 'play', 'L-LTF');

% Calculate and set the waveform RMS level to be used for subsequent power
% search operations. Disable ALC and enable automatic power search
k = m(2,:) > 0;
rms = sqrt(mean(abs(y(k)).^2));
ESG = DwPhyLab_OpenESG(n);
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:HEADER:RMS "%s", %1.6f','L-LTF',rms);
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH:REFERENCE RMS');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH ON');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:STATE OFF');
DwPhyLab_SendQuery  (ESG,'*OPC?');
DwPhyLab_CloseESG(ESG); 

if nargout > 0,
    data.Description = mfilename;
    data.Timestamp = datestr(now);
    data.x = x;
    data.y = y;
    data.m = m;
end

%% REVISIONS
% 2010-10-25 [SM]: Change array 'm' from column vector to row vector to avoid DwPhyLab_LoadWaveform error.
