function data = DwPhyLab_LoadPER(varargin)
% data = DwPhyLab_LoadPER(...)
% data = DwPhyLab_LoadPER(Mbps, LengthPSDU, PktsPerSequence,
%                         PktsPerWaveform, Broadcast, ShortPreamble, T_IFS)
%
%       Load a waveform to be used for PER measurements with DwPhyLab_GetPER

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc. All Rights Reserved

%#ok<*UNRCH>

%% Default Parameters
Mbps = 54;
LengthPSDU = 1000;
PktsPerSequence = 1000;
PktsPerWaveform = 1;
PacketUseRate = 1.0; % fraction of packets used for PER calculation
Broadcast = 1;
ShortPreamble = 1;
T_IFS = [];
FilterPacket = 1;
FrameBodyType = 0;
Continuous = 0;
SkipWaveformLoad = 0;
UseAMSDU = 0; %#ok<NASGU>
nESG = 1;
SFOppm = 0;
options ={};

%% Parse Argument List
DwPhyLab_EvaluateVarargin( varargin, ...
    {'Mbps','LengthPSDU','PktsPerSequence','PktsPerWaveform','Broadcast',...
    'ShortPreamble','T_IFS','PacketUseRate','SkipWaveformLoad','LoadPER'}, 1 );

%% Select interframe space
% For broadcast packets this is set to T_DIFS for 802.11a (not stressful).
% If not a broadcast packet, the interframe space accomodates two SIFS periods plus
% and ACK packet at the lowest modulation-specific data rate.
if isempty(T_IFS),
    if Broadcast,
        T_IFS = 34e-6;
    else
        if any(Mbps == [1 2 5.5 11]),
            T_IFS = (10 + 192+14*8 + 10)*1e-6;
        else
            T_IFS = (16 + 44 + 16)*1e-6;
        end
    end
end

%% Record Parameters
data.Description = mfilename;
data.Timestamp = datestr(now);
data.Result = 1;
clear i j k ArgList;
S = who;
for i=1:length(S),
    eval( sprintf('data.%s = %s;',S{i},S{i}) );
end

%% Skip Waveform Load 
if SkipWaveformLoad,
   return;
end
    
%% WaitBar Setup
hBarOfs = 0;
hBarLen = 1;
hBarStr = {};

if ~isempty(who('hBar'))
    if ishandle(hBar) %#ok<NODEF>
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
else
    hBar = [];
end

if isempty(hBar),
    NameStr = sprintf('%s, %g Mbps',mfilename,Mbps(1));
    hBar = waitbar(0.1,'Generating Waveform...','Name',NameStr,'WindowStyle','modal');
    hBarClose = 1;
else
    hBarClose = 0;
end

hBarStrIndex = length(hBarStr) + 1;

%% Generate the packet waveform
if mod(PktsPerSequence, PktsPerWaveform) ~= 0,
    error('PktsPerSequence must be multiple of PktsPerWaveform');
else
    Repetitions = PktsPerSequence / PktsPerWaveform;
end
FrameBodyTypeS = sprintf('FrameBodyType = %d',FrameBodyType);

x = []; m = [];
for i=1:length(Mbps),
    [xi,mi] = DwPhyLab_PacketWaveform(Mbps(i), Element(i,LengthPSDU), Element(i,T_IFS), ...
        PktsPerWaveform, Element(i,ShortPreamble), Element(i,Broadcast), 1, options, FrameBodyTypeS);
    x = [x; xi]; %#ok<AGROW>
    m = [m, mi]; %#ok<AGROW>
end
x = x/512 ; % common scaling used in DwPhyLab

%% Packet Filter
if FilterPacket,
    [B,A] = butter(5, 2*12/80); % 5 pole Butterworth, fc = 12 MHz
    x = filtfilt(B, A, x);      % zero-phase filter
end

%% Calculate and set the waveform RMS level to be used for subsequent power
% search operations. 
k = m(2,:) > 0;
rms = sqrt(mean(abs(x(k)).^2)) ;

%%%%%%%%%%  LoadPER Options  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
if isfield(data, 'LoadPER'),

    %% Rapp PA Model (Nonlinear Distortion)
    if isfield(data.LoadPER,'RappPA')
        if isfield(data.LoadPER.RappPA,'p')
            p = data.LoadPER.RappPA.p;
        else
            p = 2;
        end
        if isfield(data.LoadPER.RappPA,'OBOdB')
            OBOdB = data.LoadPER.RappPA.OBOdB;
        else
            OBOdB = 10;
        end
        Pin = rms^2;
        Psat = 10^(OBOdB/10) * Pin / (10^(p/10) - 1)^(1/p);
        y = abs(x).^2 / Psat;
        Av = sqrt(1 ./ (1 + y.^p).^(1/p));
        x = Av .* x;
        rms = sqrt(mean(abs(x(k)).^2)) ;
    end

    %% Apply GainStep
    if isfield(data.LoadPER,'GainStep')
        a = ones(size(x));
        if data.LoadPER.GainStep.dB > 0,
            a(1 : data.LoadPER.GainStep.t*80e6) = 10^(-data.LoadPER.GainStep.dB/20);
            if strcmpi(data.LoadPER.GainStep.AttnRMS, 'AttnRMS')
                rms = rms * 10^(-data.LoadPER.GainStep.dB/20);
            end                
        else
            a(data.LoadPER.GainStep.t*80e6+1 : end) = 10^(data.LoadPER.GainStep.dB/20);
        end
        x = a .* x;
    end
    
    %% Doppler Channel
    if isfield(data.LoadPER,'DopplerChannel')
        h  = data.LoadPER.DopplerChannel.h;
        fD = data.LoadPER.DopplerChannel.fD;
        if isfield(data.LoadPER.DopplerChannel,'phi')
            phi = data.LoadPER.DopplerChannel.phi;
        else
            phi = 2*pi*rand(size(h));
        end
        h = h / sqrt(sum(abs(h).^2)); % normalize power
        s = zeros(size(x));
        T = 1/80e6;
        for i = 1:length(h),
            k = i:length(x);
            w = 2*pi*fD*T*cos(phi(i));
            g = h(i) * exp(1i*w*k');
            s(k) = s(k) + g .* x(k - i + 1);
        end
        x = s;
    end
    
    %%% Sample Frequency Offset
    if isfield(data.LoadPER,'SFOppm')
        SFOppm = data.LoadPER.SFOppm;
    end    
end

%% Clip peaks to meet DualARB dynamic range
k = find(abs(real(x)) > 1);  x(k) = sign(real(x(k))) + j*imag(x(k));
k = find(abs(imag(x)) > 1);  x(k) = real(x(k)) + j*sign(imag(x(k)));


%% Load the waveform and configure the ESG
if ishandle(hBar), 
    hBarStr{hBarStrIndex} = 'Loading Waveform and Configuring Instruments...';
    waitbar(hBarOfs + 0.25*hBarLen, hBar, hBarStr); 
end

Fs = round(80e6 * (1 + SFOppm/1e6));
DwPhyLab_LoadWaveform(nESG, Fs, x, m, 'no_play');

if ishandle(hBar), 
    waitbar(hBarOfs + 0.85*hBarLen, hBar);
end

ESG = DwPhyLab_OpenESG(nESG);
if isempty(ESG), error('Unable to open ESG'); end

if Continuous,
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:TYPE CONTINUOUS');
else
    % Configure the sequence to be triggered from the LAN/GPIB using "*TRG"
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:TYPE SINGLE');
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:SOURCE BUS');
end
DwPhyLab_SendQuery(ESG,'*OPC?');

% Load/create the sequence file. This assumes DWPHYLAB100 and DWPHYLAB1000
% already exist in the SEQ directory. These are common burst lengths and
% are not dynamically generated to improve execution time and reduce write
% cylces to the ESG flash memory
if any(Repetitions == [10 100 1000 10000]),
    SequenceName = sprintf('SEQ:DWPHYLAB%d',Repetitions);
else
    SequenceName = sprintf('SEQ:DWPHYLAB');
    DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:SEQ "SEQ:DWPHYLAB","WFM1:DwPhyLab",%d,ALL',Repetitions);
end
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:WAV "%s"',SequenceName);
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:STATE ON');

% Calculate and set the waveform RMS level to be used for subsequent power
% search operations. Disable ALC and enable automatic power search
DwPhyLab_SendCommand(ESG,':SOURCE:RADIO:ARB:HEADER:RMS "%s", %1.6f',SequenceName,rms);
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH:REFERENCE RMS');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH ON');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:STATE OFF');

% Wait (up to 4 s) for operations to complete. Testing in MN shows a timeout of 2000 ms
% occassionally causes a problem, so the default threshold is doubled to provide margin.
DwPhyLab_SetVisaTimeout(ESG, 4000);
DwPhyLab_SendQuery  (ESG,'*OPC?');
DwPhyLab_CloseESG(ESG); 

% Store the sequence duration. This is used to delay timers in subsequent
% PER measurements
DwPhyLab_Parameters('TsequencePER',      Repetitions*length(x)/80e6);
DwPhyLab_Parameters('PktsPerSequencePER',PktsPerSequence * length(Mbps) * PacketUseRate);
DwPhyLab_Parameters('SequenceNamePER',   SequenceName);
DwPhyLab_Parameters('SequenceRmsPER',    rms);

if nargout == 0, 
    clear data; 
else
    data.x = x;
    data.m = m;
end;
if hBarClose,
    if ishandle(hBar)
        close(hBar);
    else
        data.Result = -1;
    end
end

%% Element - Extract ith elementh from an array
function Xi = Element(i, X)
Xi = X( min(i, length(X)) );

%% REVISIONS
% 2008-07-03: Added 12 MHz filter for packet to reduce off-channel signal
% 2008-10-17: Added option for continuous trigger
% 2008-11-04: Added option to skip waveform load
% 2011-06-23: Added option to apply GainStep
% 2011-06-29: Added Rapp PA and Doppler multipath models