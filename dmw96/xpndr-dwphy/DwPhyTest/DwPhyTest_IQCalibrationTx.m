function data = DwPhyTest_IQCalibrationTx(varargin)
% data = DwPhyTest_IQCalibrationTx(...)
%
%    Test DMW96 TX I/Q-Calibration.

% Written by Sheng Mou
% Copyright 2011 DSP Group, Inc., All Rights Reserved.

fcMHz    = 2442;
fToneMHz = 1:3; 
Gain     = -1 : 0.1 : 1;
Phase    = -5 : 0.5 : 5;
TXPWRLVL = 43;
Verbose  = 0;

DwPhyLab_EvaluateVarargin(varargin);
DwPhyTest_RunTestCommon;

BasebandID = DwPhyLab_ReadRegister(1);
if ~(BasebandID >= 7 && DwPhyLab_RadioIsRF22)
    error('Unsupported Baseband + RF combination.');
end

%% Retrieve calibration results from RF22
GainStepRF22  = 0.1; % dB
PhaseStepRF22 = 0.5; % deg

IQTxGain  = (DwPhyLab_ReadRegister(138-128+32768)-16)*GainStepRF22;
IQTxPhase = (DwPhyLab_ReadRegister(139-128+32768)-16)*PhaseStepRF22;

data.IQ_dBc        = zeros(length(fToneMHz), length(Gain), length(Phase));
data.CalResult     = zeros(length(fToneMHz), length(Gain), length(Phase));
data.GainEstimate  = zeros(length(fToneMHz), length(Gain), length(Phase));
data.PhaseEstimate = zeros(length(fToneMHz), length(Gain), length(Phase));
data.ArrayIndices  = 'Array(fToneMHz, Gain, Phase)';
data.Limit_dBc     = -40;

%% Test setup
DwPhyLab_Sleep;
DwPhyLab_WriteRegister(103,     2,   1);           % IdleSDEnB
DwPhyLab_WriteRegister(  4,          1);           % RXMode
DwPhyLab_WriteRegister(144, '2:0',   3);           % DCXShift
DwPhyLab_WriteRegister(  7, '7:4',  10);           % TxIQSrcR/TxIQSrcI
DwPhyLab_WriteRegister(224, '3:2',   1);           % set RF to Standby when BB is in Receive mode   
DwPhyLab_WriteRegister(225,        255);           % TX DACs enabled

DwPhyLab_WriteRegister(       256+85, '1:0',   1); % ENVDETTH     = 1
DwPhyLab_WriteRegister(129-128+32768,        128); % IQ_TX        = 1 only
DwPhyLab_WriteRegister(129-128+32768,     5,   1); % TX_AGC_PRE   = 1
DwPhyLab_WriteRegister(130-128+32768,     2,   1); % LOAD_TX_INIT = 1

LNAGAIN = floor(TXPWRLVL/32);
AGAIN   = mod  (TXPWRLVL,32);
DwPhyLab_WriteRegister(80, 7,       1);            % enable fixed RX gain
DwPhyLab_WriteRegister(80, 6, LNAGAIN);            % LNAGAIN = TXPWRLVL[5]
DwPhyLab_WriteRegister(65,      AGAIN);            % AGAIN   = TXPWRLVL[4:0]

%% Set up PA
if fcMHz < 3000, DwPhyLab_WriteRegister(231,hex2dec('01')); % enable 2.4 GHz PA
else             DwPhyLab_WriteRegister(231,hex2dec('10')); % enable 5 GHz PA
end

DwPhyLab_Wake; pause(0.01);

%% Set up PSA
DwPhyTest_ConfigSpectrumAnalyzer('SA');

PSA = DwPhyLab_OpenPSA;
Timeout = DwPhyLab_GetVisaTimeout(PSA);
DwPhyLab_SendCommand(PSA,':CALC:MARKER:STATE ON');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE POS');
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 20.0e6'); 
DwPhyLab_SendQuery(PSA,'*OPC?'); pause(0.5); % make sure the PSA is configured

%% Set up DMW96 I/Q Calibration
DwPhyLab_WriteRegister(40,   0); % sRxIQ = 0
DwPhyLab_WriteRegister(52,  80);
DwPhyLab_WriteRegister(53,  53);
DwPhyLab_WriteRegister(54,  17);
DwPhyLab_WriteRegister(55,   3);
DwPhyLab_WriteRegister(60,  26);
DwPhyLab_WriteRegister(61,  40);

hBar = waitbar(0.0, 'Measuring Image Rejection...', 'Name', mfilename, 'WindowStyle','modal');

for i = 1:length(fToneMHz)
    DwPhyLab_WriteRegister(136, mod(160+round(fToneMHz(i)/0.5),160));        % TxToneDiv
    WriteImageDetectorCoefs(2*abs(fToneMHz(i)));
    
    for j = 1:length(Gain)
        valK1 = min(15, max(-16, round((Gain(j)+IQTxGain)/GainStepRF22)));
        DwPhyLab_WriteRegister(134-128+32768, '4:0', mod(valK1+32, 32));     % K1TX_INIT 

        for k = 1:length(Phase)            
            valK2 = min(15, max(-16, round((Phase(k)+IQTxPhase)/PhaseStepRF22)));
            DwPhyLab_WriteRegister(135-128+32768, '4:0', mod(valK2+32, 32)); % K2TX_INIT
            DwPhyLab_WriteRegister(36,  0);                                  % sTxIQ = 0
            DwPhyLab_WriteRegister(62, 30);                                  % init CalGain
            DwPhyLab_WriteRegister(63, 15);                                  % init CalPhase
            DwPhyLab_WriteRegister(128-128+32768, 10);                       % DW_CAL_MODE = 1, CALMODE = '010'
            pause(0.5);
            
            % wait for TX_AGC_PRE_DONE
            waitLimit = 10;
            while DwPhyLab_ReadRegister(131-128+32768, 0) == 0 && waitLimit ~= 0
                pause(0.01);
                waitLimit = waitLimit - 1;
            end
            if waitLimit == 0  data.Result = -1; return; end
            
            DwPhyLab_WriteRegister(36,   1);                   % sTxIQ = 1
            DwPhyLab_WriteRegister(59, 212);                   % enable TxIQ calibration

            waitLimit = 10;
            while DwPhyLab_ReadRegister(58, 7) == 0 && waitLimit ~= 0
                pause(0.01);
                waitLimit = waitLimit - 1;
            end

            data.CalResult(i,j,k)     = DwPhyLab_ReadRegister(58);
            data.GainEstimate(i,j,k)  = (DwPhyLab_ReadRegister(62) - 30)*0.068;
            data.PhaseEstimate(i,j,k) = (DwPhyLab_ReadRegister(63) - 15)*0.448;        
            
            DwPhyLab_WriteRegister(59, 7, 0);                  % disable TxIQ calibration
            DwPhyLab_WriteRegister(128-128+32768, 0);          % clear CALMODE register
            
            % measure resulted image rejection 
            DwPhyLab_WriteRegister(224, '3:2', 2); pause(0.1); % set RF to TX         
            data.IQ_dBc(i,j,k) = MeasureFromPSA(PSA, fcMHz, fToneMHz(i));        
            DwPhyLab_WriteRegister(224, '3:2', 1); pause(0.1); % set RF to Standby    

            if Verbose
                fprintf('Reg58 = %s\n', dec2bin(data.CalResult(i,j,k),8));
                fprintf('Gain = %.2f(%.2f) Phase = %.2f(%.2f) IQ_dBc = %.2f\n\n', Gain(j), data.GainEstimate(i,j,k),  ...
                        Phase(k), data.PhaseEstimate(i,j,k), data.IQ_dBc(i,j,k));
                
                if data.IQ_dBc(i,j,k) > data.Limit_dBc
                    close(hBar);
                    break;
                end            
            end
            
            if ishandle(hBar),
                waitbar(((i-1)*length(Gain)*length(Phase)+(j-1)*length(Phase)+k)/(length(fToneMHz)*length(Gain)*length(Phase)), ...
                        hBar, sprintf('%d MHz Tone, %1.1f dB, %1.1f deg, IMR = %1.1f dB', ...
                                      fToneMHz(i), Gain(j), Phase(k), -data.IQ_dBc(i,j,k)));            
            else
                fprintf('*** TEST ABORTED ***\n');
                data.Result = -1;
                break;
            end            
        end
        
        if ~ishandle(hBar), break; end
    end
    
    if ~ishandle(hBar), break; end

    v = -1 * reshape(data.IQ_dBc(i,:,:), 1, length(Gain)*length(Phase));
    v = v(isfinite(v));
    data.MinIMRdB(i) = min(v);
    data.AvgIMRdB(i) = mean(v);
end

if ~isempty(find(data.IQ_dBc > data.Limit_dBc)) data.Result = 0; end
    
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
DwPhyLab_Initialize; % re-initialize to restore settings
if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));



%% Function WriteImageDetectorCoefs
function WriteImageDetectorCoefs(fToneMHz)

fs    = 20; % 20MHz at image detector input
theta = 2*pi*(fToneMHz/fs);
cr    = cos(theta);
ci    = sin(theta);

cr = round(cos(theta)*2^11);
cr = min(max(cr, -2^11+1), 2^11-1);
ci = round(sin(theta)*2^11);
ci = min(max(ci, -2^11+1), 2^11-1);

highPart = floor(cr/256);
lowPart  = cr - highPart*256;
DwPhyLab_WriteRegister(48, bitand(highPart+16,  15));
DwPhyLab_WriteRegister(49, bitand(lowPart+256, 255));

highPart = floor(ci/256);
lowPart  = ci - highPart*256;
DwPhyLab_WriteRegister(50, bitand(highPart+16,  15));
DwPhyLab_WriteRegister(51, bitand(lowPart+256, 255));


%% Function MeasureFromPSA
function IQ_dBc = MeasureFromPSA(PSA, fcMHz, fToneMHz)

DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:COUNT 10');
DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE ON');
DwPhyLab_SendQuery(PSA,'*OPC?'); % wait for the completion of averaging        

DwPhyLab_SendCommand(PSA,':CALC:MARKER:X %1.6e MHZ', fcMHz + fToneMHz);
DwPhyLab_SendCommand(PSA,':CALC:MARKER:PEAK:SEARCH:MODE MAX');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MAX');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE DELTA');

DwPhyLab_SendCommand(PSA, sprintf(':CALC:MARKER:X %d MHz', -2*fToneMHz));
result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');

if isempty(result)
    % error recovery: for an unknown reason, the timeout is occassionaly reset to
    % 5000 ms and the measurement times out
    DwPhyLab_SetVisaTimeout(PSA, 30000);
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
end

DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
IQ_dBc = str2double(result);

