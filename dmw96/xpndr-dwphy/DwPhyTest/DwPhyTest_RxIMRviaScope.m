function data = DwPhyTest_RxIMRviaScope(varargin)
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
mVrms      = 160;    % gain target for ADC input tone
path = 1;

DwPhyLab_EvaluateVarargin(varargin, {'fcMHz'});

if UseArbTone && any(diff(abs(fToneMHz))) > 0,
    error('abs(fToneMHz) must be constant when using the DualARB (UseArbTone=1)');
end

hBar = waitbar(0.0, 'Measuring CFO', 'Name',mfilename, 'WindowStyle','modal');

%% Run Test (CFO Estimation)
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
if isempty(who('CFOppm'))
    data.MeasureRxCFO = DwPhyTest_MeasureRxCFO(sprintf('fcMHz = %g',fcMHz(1)));
    CFOppm = data.MeasureRxCFO.CFOppm;
end
if ishandle(hBar), waitbar(0, hBar, 'Measuring Image Rejection...'); end

%% Run Test (Image Rejection)
DwPhyTest_RunTestCommon;

data.CalSIRdB = GetCalSIRdB(fToneMHz); % baseband filter gain
data.AbsPwrH = log2(mVrms)*8 + 4;      % gain target
data.ArrayIndices = 'Array(fcMHz, fToneMHz, AntennaPath)';

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
    
    DwPhyLab_CloseESG(ESG);
    DwPhyLab_LoadWaveform(1, 100e6, y, m, 'play', 'RxIMR');
    ESG = DwPhyLab_OpenESG;
    DwPhyLab_SendQuery(ESG,'*OPC?');
end

%% Generate calculation arrays
Z = zeros( length(fcMHz), length(fToneMHz), length(path) );
data.SdB = Z;
data.IdB = Z;
data.NdB = Z;
data.IdBadj = Z;
data.SdBadj = Z;
data.IMRdB = Z;

FsMHz = 50; data.FsMHz = FsMHz;

%% Configure Oscilloscope
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'C1:TRACE ON');
DwPhyLab_SendCommand(scope, 'C2:TRACE ON');
DwPhyLab_SendCommand(scope, 'C3:TRACE OFF');
DwPhyLab_SendCommand(scope, 'C4:TRACE OFF');
DwPhyLab_SendCommand(scope, 'C1:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C2:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C1:BWL 20MHz');
DwPhyLab_SendCommand(scope, 'C2:BWL 20MHz');

DwPhyLab_SendCommand(scope, 'TIME_DIV 200E-6');
DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 1M');

DwPhyLab_SendCommand(scope,'TRIG_MODE AUTO');
DwPhyLab_CloseScope(scope);
            
%% Loop fcMHz x fToneMHz
for i=1:length(fcMHz),

    DwPhyLab_Sleep;
    DwPhyLab_SetChannelFreq(fcMHz(i));
    if exist('UserReg','var'), DwPhyLab_WriteUserReg(UserReg); end
    DwPhyLab_Wake;
    
    InitAGain = DwPhyLab_ReadRegister(65);
    DwPhyLab_WriteRegister(65, InitAGain - 10); % reduce gain by 15 dB

    PrdBm = -65 + 20*log10(mVrms/80) + data.Ploss(i); % starting point for level adjustment

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
% DwPhyLab_WriteRegister( 74, 0);
% DwPhyLab_WriteRegister( 80,'7:6',2); % Fixed gain with LNAGAIN=0
        DwPhyLab_WriteRegister(144,'2:0',1); % 800 kHz HPF
        DwPhyLab_WriteRegister(197,4,0);     % RSSI in 3/4 dB units for better accuracy

        for k=1,
            n = sub2ind(size(Z),i,j,k);

            SetBasebandMixer(0);
            
            % Set Signal Amplitude
            if j==1, 
                DwPhyLab_SetRxFreqESG(fcMHz(i) + fcMHzOfs + 5); % adjust amplitude at 5 MHz
                PrdBm(k) = AdjustPower(data.AbsPwrH, PrdBm, path);
                DwPhyLab_SetRxFreqESG(f);
            else
                DwPhyLab_SetRxPower(PrdBm(k));
            end

            scope = DwPhyLab_OpenScope;
            DwPhyLab_SendCommand(scope,'TRIG_MODE SINGLE');
            DwPhyLab_CloseScope(scope);
            pause(0.2);
            
            [C1,T] = DwPhyLab_GetScopeWaveform('C1');
            [C2,T] = DwPhyLab_GetScopeWaveform('C2');
            r = downsample(C1+sqrt(-1)*C2, round(1/(T*FsMHz*1e6)));
            r = r-mean(r);
            scope = DwPhyLab_OpenScope;
            DwPhyLab_SendCommand(scope,'TRIG_MODE AUTO');
            DwPhyLab_CloseScope(scope);
            
            data.r{i,j,k} = r;
            
            kT=(1:length(r))/FsMHz; 
            kT=reshape(kT,size(r));
            w = exp(-sqrt(-1)*2*pi*fToneMHz(j)*kT);
            [Bz,Az] = butter(7,1/FsMHz);

            x = conj(w).*filtfilt(Bz,Az,w.*r);
            y = w.*filtfilt(Bz,Az,conj(w).*r);
            
            % discard 100T at both ends
            k1 = 100:(length(x)-100);
            x=x(k1);
            y=y(k1);

            Kxx = mean(x.*conj(x));
            Kyy = mean(y.*conj(y));
            Kxy = mean(x.*y);

            g1 = sqrt(Kxx+Kyy+2*real(Kxy));
            g2 = sqrt(Kxx+Kyy-2*real(Kxy));

            Ahat = g2/g1;
            Phihat = asin(2*imag(Kxy)/(g1*g2));

            a = abs(Kxy)/Kxx;
            theta = angle(Kxy);
            
            data.IMRdB(i,j,k) = -20*log10(a);
            data.a(i,j,k) = a;
            data.theta(i,j,k) = theta;
            data.A_dB(i,j,k) = 20*log10(Ahat);
            data.Phi_deg(i,j,k) = Phihat*180/pi;
            
            if exist('Verbose','var') && Verbose,
                pathstr = {'RXA','RXB'};
                if j==1 && k==1, fprintf('   fcMHz = %4d, ',fcMHz(i));
                else             fprintf('                 ');
                end
                if k==1, fprintf('fToneMHz =%7.3f, ',fToneMHz(j));
                else     fprintf('                   ');
                end
                fprintf('%s, IMRdB = %4.1f\n',pathstr{path},data.IMRdB(n));
            end
            
            if ~ishandle(hBar), continue; end;
        end
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

DwPhyLab_SendCommand(ESG,':SOURCE:DM:STATE ON');
DwPhyLab_SendCommand(ESG,':OUTPUT:MODULATION:STATE ON');
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

for i=1:n,
    RSSI(i,:) = DwPhyLab_AverageRSSI(1);
end
RSSI = 10*log10( mean(10.^(RSSI/10), 1) );
        
if nargin>0, RSSI = RSSI(k); end

%% GetCalSIRdB(fMHz)
%  Calibration for filter differences between fMHz and -fMHz + 10 MHz
function CalSIRdB = GetCalSIRdB(fMHz)
fMHz = abs(fMHz); % filter is real, i.e., symmetrical around DC
for i=1:length(fMHz),
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
