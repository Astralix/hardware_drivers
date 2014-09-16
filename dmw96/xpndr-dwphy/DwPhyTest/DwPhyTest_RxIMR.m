function data = DwPhyTest_RxIMR(varargin)
% data = DwPhyTest_RxIMR(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fToneMHz   = [-5.556 -2.257 2.257 5.556];  % tone frequency relative to fcMHz
UseArbTone = 0;      % use arb tone instead of ESG LO
AdB        = 0.0;    % gain mismatch (dB) for arb tone
phi_deg    = 0.0;    % phase mismatch (deg) for arb tone
NegToneAdB = NaN;    % amplitude on negative image tone
NegToneDeg = 0.0;    % relative phase for negative tone
mVrms      = 288;    % gain target for ADC input tone
path       = 1:2;    % which antenna paths to measured {1,2}

DwPhyLab_EvaluateVarargin(varargin, {'fcMHz'});
if ~exist('fcMHz','var'), fcMHz = 2484; end

if UseArbTone && any(diff(abs(fToneMHz))) > 0,
    error('abs(fToneMHz) must be constant when using the DualARB (UseArbTone=1)');
end

hBar = waitbar(0.0, 'Measuring CFO', 'Name',mfilename, 'WindowStyle','modal');

%% Run Test (CFO Estimation)
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
if isempty(who('CFOppm'))
    data.MeasureRxCFO = DwPhyTest_MeasureRxCFO(sprintf('fcMHz = %g; RunIQCal = %d;', fcMHz(1), data.RunIQCal));
    CFOppm = data.MeasureRxCFO.CFOppm;
end
if ishandle(hBar), waitbar(0, hBar, 'Measuring Image Rejection...'); end

%% Run Test (Image Rejection)
DwPhyTest_RunTestCommon;

data.CalSIRdB = GetCalSIRdB(fToneMHz); % baseband filter gain
data.AbsPwrH = log2(mVrms)*8 + 4;      % gain target
data.ArrayIndices = 'Array(fcMHz, fToneMHz, AntennaPath)';

if DwPhyLab_RadioIsRF22
    RadioIsRF22 = 1;
    data.DefaultRFSW1 = DwPhyLab_ReadRegister(228);
else
    RadioIsRF22 = 0; 
end

% Configure the signal generator
ESG = DwPhyLab_OpenESG;
DwPhyLab_SendCommand(ESG,':OUTPUT:STATE ON');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:STATE ON');
if ~UseArbTone
    DwPhyLab_SendCommand(ESG,':SOURCE:DM:STATE OFF');
    DwPhyLab_SendCommand(ESG,':OUTPUT:MODULATION:STATE OFF');
else
    j = sqrt(-1);
    f = abs(fToneMHz(1));
    L = round(100 * 100 * f);
    k = 0:(L-1);
    phi = pi*phi_deg/180;
    yI = 10^(AdB/20) * cos(2*pi*f*k/100      ) / sqrt(2);
    yQ =               sin(2*pi*f*k/100 + phi) / sqrt(2);
    y = yI + j*yQ;
    if ~isnan(NegToneAdB),
        A = 10^(NegToneAdB/20);
        phi = pi*NegToneDeg/180;
        zI = A * cos(2*pi*f*k/100 + phi) / sqrt(2);
        zQ = A * sin(2*pi*f*k/100 + phi) / sqrt(2);
        y = y + (zI-j*zQ);
    end
    m = ones(length(y),2);
    m(90:length(y),1) = 0;
    
    DwPhyLab_LoadWaveform(1, 100e6, y, m, 'play', 'RxIMR');
    DwPhyLab_SendQuery(ESG,'*OPC?');
end
DwPhyLab_CloseESG(ESG);

%% Generate calculation arrays
Z = zeros( length(fcMHz), length(fToneMHz), length(path) );
data.SdB = Z;
data.IdB = Z;
data.NdB = Z;
data.IdBadj = Z;
data.SdBadj = Z;
data.IMRdB = Z;

%% Loop fcMHz x fToneMHz x Path
for i=1:length(fcMHz),

    DwPhyLab_Sleep;
    DwPhyLab_SetChannelFreq(fcMHz(i));
    if exist('UserReg','var'), DwPhyLab_WriteUserReg(UserReg); end
    DwPhyLab_Wake;

    PrdBm = -65 + 20*log10(mVrms/80) + data.Ploss(i); % starting point for level adjustment
    PrdBm = PrdBm * ones(size(path));

    for j=1:length(fToneMHz),
        % Adjust tone so it occurs at 15.556 MHz with a +10MHz upmix in the receiver. The
        % Mojave baseband provides > 70 dB rejection for a +/- 5 kHz notch around this freq.
        fcMHzOfs = -CFOppm * 1e-6 * fcMHz(i);
        if ~UseArbTone,
            f = fcMHz(i) + fcMHzOfs + fToneMHz(j);
        else
            f = fcMHz(i) + fcMHzOfs + abs(fToneMHz(j))*(sign(fToneMHz(j))-1);
        end
        DwPhyLab_SetRxFreqESG(f);

        DwPhyLab_WriteRegister( 77,  0);     % Pwr100dBm=0 to avoid clipping noise measure
        DwPhyLab_WriteRegister( 80,'7:6',3); % Fixed gain with LNAGAIN=1
%  DwPhyLab_WriteRegister( 74, 0);
%  DwPhyLab_WriteRegister( 80,'7:6',2); % Fixed gain with LNAGAIN=0
        DwPhyLab_WriteRegister(144,'2:0',1); % 800 kHz HPF
        DwPhyLab_WriteRegister(197,4,0);     % RSSI in 3/4 dB units for better accuracy

        for k=path
            if RadioIsRF22
                if k == 1 DwPhyLab_WriteRegister(228, data.DefaultRFSW1);
                else      DwPhyLab_WriteRegister(228, bitxor(data.DefaultRFSW1, 255));
                end
            end
            
            n = sub2ind(size(Z),i,j,k);

            SetBasebandMixer(0);
            
            % Set Signal Amplitude
            if j==1, PrdBm(k) = AdjustPower(data.AbsPwrH, PrdBm(k), k);
            else     DwPhyLab_SetRxPower(PrdBm(k));
            end

            data.SdB(n) = AverageRSSI(k, 1);         % measure signal power
            
            SetBasebandMixer(10*sign(fToneMHz(j)));
            data.IdB(n) = AverageRSSI(k, 2);         % measure image power
            
            DwPhyLab_SetRxPower(-136);
            data.NdB(n) = AverageRSSI(k, 2);         % measure noise power

            
            data.IdBadj(n) = 10*log10(10.^(data.IdB(n)/10) - 10.^(data.NdB(n)/10)) + data.CalSIRdB(j);
            data.SdBadj(n) = 10*log10(10.^(data.SdB(n)/10) - 10.^(data.IdBadj(n)/10));
            data.IMRdB(n)  = data.SdBadj(n) - data.IdBadj(n);
            
            if exist('Verbose','var') && Verbose,
                pathstr = {'RXA','RXB'};
                if j==1 && k==1, fprintf('   fcMHz = %4d, ',fcMHz(i));
                else             fprintf('                 ');
                end
                if k==1, fprintf('fToneMHz =%7.3f, ',fToneMHz(j));
                else     fprintf('                   ');
                end
                fprintf('%s, IMRdB = %4.1f\n',pathstr{k},data.IMRdB(n));
            end
            
            if ~ishandle(hBar), continue; end;
        end
        
        if RadioIsRF22 DwPhyLab_WriteRegister(228, data.DefaultRFSW1); end
    end

%     % Remove noise power from image calculation
%     data.IdBadj = 10*log10(10.^(data.IdB/10) - 10.^(data.NdB/10)) + data.CalSIRdB;
%     data.SdBadj = 10*log10(10.^(data.SdB/10) - 10.^(data.IdBadj/10));
%     data.IMRdB = data.SdBadj - data.IdBadj;
    
    if ishandle(hBar),
        waitbar(i/length(fcMHz), hBar,sprintf('%d MHz, IMR = %1.1f, %1.1f dB',fcMHz(i),data.IMRdB(i,j,:)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end
data.MinIMRdB = min(min(min(data.IMRdB)));
if data.MinIMRdB < 30, data.Result = 0; end

ESG = DwPhyLab_OpenESG;
DwPhyLab_SendCommand(ESG,':SOURCE:DM:STATE ON');
DwPhyLab_SendCommand(ESG,':OUTPUT:MODULATION:STATE ON');
DwPhyLab_CloseESG(ESG);
if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));


%% AdjustPower
function PrdBm = AdjustPower(AbsPwrH, PrdBm, k)
DwPhyLab_SetRxPower(PrdBm);

for i = 1:20,
    pause(0.1);
    AbsPwr = 4/3 * AverageRSSI(k);
        
    if     abs(AbsPwr - AbsPwrH) > 4, dB = 2.0;
    elseif abs(AbsPwr - AbsPwrH) > 2, dB = 1.0;
    else   return; % done...error is small enough
    end
    
    if AbsPwr > AbsPwrH, PrdBm = PrdBm - dB;
    else                 PrdBm = PrdBm + dB;
    end
    DwPhyLab_SetRxPower(PrdBm);
%    fprintf('AbsPwr = %1.1f (%1.1f), PrdBm -> %1.1f dBm\n',AbsPwr,AbsPwrH,PrdBm);
end
warning('DwPhyTest_RxIMR:GainSteps','Aborting power adjustment after 20 steps');

%% Function AverageRSSI
function RSSI = AverageRSSI(k, n)
if nargin < 2, n = 1; end
if DwPhyLab_RadioIsRF22 k = 2; end % always read RSSI from baseband path 1

for i=1:n,
    RSSI(i,:) = DwPhyLab_AverageRSSI(1); %#ok<AGROW>
end
RSSI = 10*log10( mean(10.^(RSSI/10), 1) );
        
if nargin>0, RSSI = RSSI(k); end

%% GetCalSIRdB(fMHz)
%  Calibration for filter differences between fMHz and -fMHz + 10 MHz
function CalSIRdB = GetCalSIRdB(fMHz)
fMHz = abs(fMHz); % filter is real, i.e., symmetrical around DC
N = length(fMHz);
CalSIRdB = zeros(1, N);
for i = 1:N,
    f = [fMHz(i) -fMHz(i)+10];
    H = freqz([3, 0, -7, 0, 20, 32, 20, 0, -7, 0, 3],64,f,40);
    CalSIRdB(i) = 20*log10(abs(H(1)/abs(H(2))));
end

%% SetBasebandMixer
function SetBasebandMixer(fMHz)
switch fMHz,
    case -10, DwPhyLab_WriteRegister(6, bin2dec('00100000')); % -10 MHz mixer (+10MHz IF)
    case   0, DwPhyLab_WriteRegister(6, bin2dec('00100011')); % no baseband mixer
    case +10, DwPhyLab_WriteRegister(6, bin2dec('00101100')); % +10 MHz mixer (-10MHz IF)
    otherwise, error('Mixer frequency must be -10, 0, +10');
end

%% REVISION HISTORY
% 2008-03-30 Corrected IMR for
%              - Baseband filter response at f and  -f + 10 HMz
%              - Image amplitude during measurement of signal power
% 2008-04-07 Remove image from signal as power, not amplitude
% 2008-06-07 Added capability to measure multiple and negative fToneMHz IMR
% 2008-06-10 Fixed GetCalSIRdB...previously only calibrated to first frequency
% 2011-01-02 [SM]: Added changes to support RF22 T/R switch design.
% 2011-04-12 [SM]: Make sure the same RunIQCal is used for CFO measuring 