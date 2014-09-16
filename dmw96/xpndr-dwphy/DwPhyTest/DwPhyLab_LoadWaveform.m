function DwPhyLab_LoadWaveform(n, Fs, y, Markers, play_flag, filename)
%Load a waveform (and markers) to the ESG DualARB
%   DwPhyLab_LoadWaveform(n, Fs, y, Markers, play_flag, filename) loads
%   waveform y with sample rate Fs to signal generator n. Options Markers,
%   play_flag, and filename may be supplied.
%
%   DwPhyLab_LoadWaveform(n, filename) loads the specified file from memory
%   on signal generator n.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

if nargin == 1 && ischar(n),
    % Just want to load a file into the default ESG
    filename = n;
    n = [];
    y = [];
    IQAtt = [];
    
elseif nargin == 2 && ischar(Fs),
    % Just want to load a file into the specified ESG
    filename = Fs;
    y = [];
    IQAtt = [];
    
else
    % Handle case where the waveform vector is supplied
    if nargin < 6,
        filename = 'DwPhyLab';
    end
    if nargin < 5,
        play_flag = 'play';
    end
    if nargin < 4,
        Markers = [];
    end

    % agt_waveform wants vector to be 1xN
    y = reshape(y, 1, length(y));
    
    % force size to be a multiple of two (truncate if necessary)
    if mod(length(y), 2) == 1,
        y = y(1 : (length(y)-1));
    end

    % Generate a 1 microsecond marker pulse if no array is provided
    if isempty(Markers),
        Markers = zeros(2, length(y));
        for i=1:round(Fs/1e6),
            Markers(1,i) = 1;
        end
        Markers(2,:) = ones(1,length(y));
    end
    
    % Select I/Q attenuator setting based on peak-to-average amplitude
    k = find(Markers(2,:)>0);
    IQAtt = 20*log10( 0.7 * max(abs(y(k)))/std(y(k)) ) + 2;
    IQAtt = max(0, min(10, IQAtt)); % bound the value in case something goes wrong
end

%% Configure ESG and Load Waveform
ESG = DwPhyLab_OpenESG(n);
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:STATE OFF');
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:TYPE CONTINUOUS');
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:TYPE:CONTINUOUS:TYPE FREE');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:SOURCE BBG1');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:STATE ON');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:BBFILTER 40E6');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:EXTERNAL:BBFILTER 40E6');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:EXTERNAL:SOURCE BBG1');
DwPhyLab_SendCommand(ESG, ':SOURCE:DM:IQAD:EXT:IQAT 6.0');
if ~isempty(IQAtt),
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:IQ:MODULATION:ATTEN %1.1f',IQAtt);
    DwPhyLab_Parameters('WaveformIQAtt',IQAtt);
end

if isempty(y),
    % Point to non-volatile memory if a path is not specified
    if isempty(findstr(upper(filename),'WFM1:')) && isempty(findstr(upper(filename),'SEQ:')),
        filename = sprintf('WFM1:%s',filename);
    end
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:WAVEFORM "%s"',filename);
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:STATE ON');
else
    [status, description] = WaveformLoad(ESG, y, filename, Fs, play_flag, 'no_normscale', Markers);
    if status<0, error(description); end
    DwPhyLab_Parameters('Twaveform', length(y)/Fs);
end
DwPhyLab_SendCommand(ESG, ':SOURCE:RAD:ARB:MDES:PULS M2');
DwPhyLab_SendQuery  (ESG,'*OPC?');
DwPhyLab_CloseESG(ESG);

%% Select download method
function [status, description] = WaveformLoad(h, y, filename, Fs, play_flag, scaling, Markers)

if DwPhyLab_Parameters('UseAgilentWaveformDownloadAssistent'),
    [status, description] = agt_waveformload(h, y, filename, Fs, play_flag, scaling, Markers);
else
    [status, description] = DwPhyLab_LoadWaveformESG(h, y, filename, Fs, play_flag, scaling, Markers);
end

%% Download E4438C DualARB Waveform without using agt_waveformload()
function [status, description] = DwPhyLab_LoadWaveformESG(h, x, filename, Fs, play_flag, scaling, Markers)

xI = real(x);
xQ = imag(x);
y = [xI; xQ];
y = y(:)';

yPeak = max([max(abs(xI)) max(abs(xQ))]);

if strcmpi(scaling, 'normscale'),
    y = (0.7/yPeak) * y;
elseif yPeak > 1
    error('IQData must be in the range [ -1:1 ]' );
end

c = 2^15 - 1;
M = 2^16;
u = mod(round(y*c)+M, M);
i = 1:length(u);
j = 2*i - 1;
v(j+0) = floor(u(i)/256);
v(j+1) =   mod(u(i),256);
v = uint8(v);

description = '';

HeaderStr = sprintf(':MEM:DATA "WFM1:%s", ',filename);
n = length(v);
m = ceil(log10(n));
LengthStr = sprintf('#%d%d',m,n);
buffer = [uint8(HeaderStr) uint8(LengthStr) uint8(v) uint8(10)];
status = viWriteBinary(h,buffer);
if status < 0, 
    error('viWriteBinary failed on status 0x%s',dec2hex(mod(status+2^32,2^32))); 
end

HeaderStr = sprintf(':MEM:DATA "MKR1:%s", ',filename);
Markers = Markers(1,:) + 2*Markers(2,:);
n = numel(Markers);
m = ceil(log10(n));
LengthStr = sprintf('#%d%d',m,n);
buffer = [uint8(HeaderStr) uint8(LengthStr) uint8(Markers) uint8(10)];
status = viWriteBinary(h,buffer);
if status < 0, 
    error('viWriteBinary failed on status 0x%s',dec2hex(mod(status+2^32,2^32))); 
end

if Fs > 0,
    SampleRateCommand = sprintf(':SOURce:RADio:ARB:CLOCk:SRATe %d', Fs);
    status = viWrite(h, SampleRateCommand);
    if (status < 0)
        error('viWrite failed on status 0x%s',dec2hex(mod(status+2^32,2^32)));
    end
end

if strcmpi(play_flag, 'play')
    playcommand = [':source:rad:arb:wav "arbi:' filename '"'];
    status = viWrite(h, playcommand);
    if (status < 0)
        error('viWrite failed on status 0x%s',dec2hex(mod(status+2^32,2^32)));
    end
    status = viWrite(h,':source:rad:arb:state on');
    if (status < 0)
        error('viWrite failed on status 0x%s',dec2hex(mod(status+2^32,2^32)));
    end
end


%% REVISIONS
% 100210BB -- Add function to download DualARB waveform without using agt_waveformload()