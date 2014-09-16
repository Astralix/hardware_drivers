function data = DwPhyTest_RxIMR_Scope(varargin)
% data = DwPhyTest_RxIMR_Scope(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz      = 2412;   % channel frequency
fToneMHz   = 5.556;  % tone frequency relative to fcMHz
UseArbTone = 1;      % use arb tone instead of ESG LO
AdB        = 0.0;    % gain mismatch (dB) for arb tone
phi_deg    = 0.0;    % phase mismatch (deg) for arb tone
NegToneAdB = NaN;    % amplitude on negative image tone
NegToneDeg = 0.0;    % relative phase for negative tone
mVrms      = 288;    % gain target for ADC input tone
path       =   1;
ScopeCapture = 1;

DwPhyLab_EvaluateVarargin(varargin, {'fcMHz'});

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
data.CalSIRdB = GetCalSIRdB(fToneMHz);

% Configure the signal generator
ESG = DwPhyLab_OpenESG;
DwPhyLab_SendCommand(ESG,':OUTPUT:STATE ON');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:STATE ON');
if ~UseArbTone
    DwPhyLab_SendCommand(ESG,':SOURCE:DM:STATE OFF');
    DwPhyLab_SendCommand(ESG,':OUTPUT:MODULATION:STATE OFF');
else
    L = round(100 * 100 * fToneMHz);
    k = 0:(L-1);
    phi = pi*phi_deg/180;
    yI = 10^(AdB/20) * cos(2*pi*fToneMHz*k/100      ) / sqrt(2);
    yQ =               sin(2*pi*fToneMHz*k/100 + phi) / sqrt(2);
    y = yI + j*yQ;
    if ~isnan(NegToneAdB),
        A = 10^(NegToneAdB/20);
        phi = pi*NegToneDeg/180;
        zI = A * cos(2*pi*fToneMHz*k/100 + phi) / sqrt(2);
        zQ = A * sin(2*pi*fToneMHz*k/100 + phi) / sqrt(2);
        y = y + (zI-j*zQ);
    end
    m = ones(length(y),2);
    m(90:length(y),1) = 0;
    
    DwPhyLab_CloseESG(ESG);
    DwPhyLab_LoadWaveform(1, 100e6, y, m, 'play', 'RxIMR');
    fToneMHz = 0; % clear offset so it doesn't get added below
    ESG = DwPhyLab_OpenESG;
    DwPhyLab_SendQuery(ESG,'*OPC?');
end

for i=1:length(fcMHz),
    
    DwPhyLab_SetChannelFreq(fcMHz(i));
    DwPhyLab_Wake;
    
    % Adjust tone so it occurs at 15.556 MHz with a +10MHz upmix in the receiver. The
    % Mojave baseband provides > 70 dB rejection for a +/- 5 kHz notch around this freq.
    fcMHzOfs = -CFOppm * 1e-6 * fcMHz(i);
    f = fcMHz(i) + fcMHzOfs + fToneMHz;
    DwPhyLab_SetRxFreqESG(f);

    DwPhyLab_WriteRegister( 77,  0);
    DwPhyLab_WriteRegister( 80,'7:6',3);
    DwPhyLab_WriteRegister(144,'2:0',1); % 800 kHz HPF
    DwPhyLab_WriteRegister(197,4,0); 

    PrdBm = -65 + 20*log10(mVrms/80) + data.Ploss(i); % starting point for level adjustment
    AbsPwrH = log2(mVrms)*8 + 4;                      % gain target
    data.AbsPwrH = AbsPwrH;

    for k=path

        DwPhyLab_WriteRegister(6, 0, 1); % Disable baseband mixer
        PrdBm = AdjustPower(AbsPwrH, PrdBm, k);

        DwPhyLab_WriteRegister(6, bin2dec('00100101')); % no baseband mixer
        data.SdB(i,k) = AverageRSSI(k, 1);

        DwPhyLab_WriteRegister(6, bin2dec('00100100')); % +10 MHz mixer
        data.IdB(i,k) = AverageRSSI(k, 2);

        % Capture Instantaneous Error
        X = DwPhyMex('RvClientRxRSSIValues');
        data.I0 = X(1:2:length(X)); % Path 0 (RXA)
        data.I1 = X(2:2:length(X)); % Path 1 (RXB)

        % Capture Data using the scope
        if ScopeCapture,
            % Diagnostic code. Not supported for normal use
            DwPhyLab_SendCommand('Scope', 'TRIG_MODE AUTO'); pause(0.1);
            DwPhyLab_SendCommand('Scope', 'TRIG_MODE STOP');
            [C1]   = DwPhyLab_GetScopeWaveform('C1');
            [C2,T] = DwPhyLab_GetScopeWaveform('C2');
            DwPhyLab_SendCommand('Scope', 'TRIG_MODE AUTO');
            data.ScopeCapture.x = C1+j*C2;
            data.ScopeCapture.FsMHz = 1/T/1e6;
        else
            data.ScopeCapture = [];
        end

        DwPhyLab_SetRxPower(-136);
        data.NdB(i,k) = AverageRSSI(k, 2);

        if ~ishandle(hBar), continue; end;
    end

    % Remove noise power from image calculation
    data.IdBadj = 10*log10(10.^(data.IdB/10) - 10.^(data.NdB/10));
    data.IMRdB = data.SdB - data.IdBadj - data.CalSIRdB;
    
    % Remove image (as amplitude) from signal power in IMRdB
    data.IMRdB = 20*log10( 10.^(data.IMRdB/20) - 1 );
    
    if ishandle(hBar),
        waitbar(i/length(fcMHz), hBar,sprintf('%d MHz, IMR = %1.1f, %1.1f dB',fcMHz(i),data.IMRdB(i,:)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end
data.MinIMRdB = min(min(data.IMRdB));
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
        
    if abs(AbsPwr - AbsPwrH) > 4,
        dB = 2.0;
    elseif abs(AbsPwr - AbsPwrH) > 2,
        dB = 1.0;
    else
        return;
    end
    
    if AbsPwr > AbsPwrH,
        PrdBm = PrdBm - dB;
    else
        PrdBm = PrdBm + dB;
    end
    DwPhyLab_SetRxPower(PrdBm);
%    fprintf('AbsPwr = %1.1f (%1.1f), PrdBm -> %1.1f dBm\n',AbsPwr,AbsPwrH,PrdBm);
end
warning('DwPhyTest_RxIMR:GainSteps','Aborting power adjustment after 20 steps');

%% Function AverageRSSI
function RSSI = AverageRSSI(k, n)
if nargin < 2, n = 1; end

for i=1:n,
    RSSI(i,:) = DwPhyLab_AverageRSSI; %#ok<AGROW>
end
RSSI = 10*log10( mean(10.^(RSSI/10), 1) );
        
if nargin>0,
    RSSI = RSSI(k);
end

%% GetCalSIRdB(fMHz)
%  Calibration for filter differences between fMHz and -fMHz + 10 MHz
function CalSIRdB = GetCalSIRdB(fMHz)
b = [3, 0, -7, 0, 20, 32, 20, 0, -7, 0, 3];
f = [fMHz -fMHz+10];
H = abs(freqz(b,64,f,40));
CalSIRdB = 20*log10(H(1)/H(2));
