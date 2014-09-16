function data = DwPhyTest_IQCalibrationRx(varargin)
% data = DwPhyTest_IQCalibrationRx(...)
%
%    Test DMW96 RX I/Q-Calibration.

% Written by Sheng Mou
% Copyright 2011 DSP Group, Inc., All Rights Reserved.

fcMHz    = 2442;
fToneMHz = 1:3;
Gain     = -1 : 0.1 : 1;
Phase    = -5 : 0.5 : 5;
AGAIN    = 21; 
Verbose  = 0;
DiversityMode = 0; % [110804BB - set to default]

DwPhyLab_EvaluateVarargin(varargin);
DwPhyTest_RunTestCommon;

BasebandID = DwPhyLab_ReadRegister(1);
if ~(BasebandID >= 7 && DwPhyLab_RadioIsRF22)
    error('Unsupported Baseband + RF combination.');
end

%% Retrieve calibration results from RF22
GainStepRF22  = 0.1; % dB
PhaseStepRF22 = 0.5; % deg

IQRxGain  = (DwPhyLab_ReadRegister(140-128+32768)-16)*GainStepRF22;
IQRxPhase = (DwPhyLab_ReadRegister(141-128+32768)-16)*PhaseStepRF22;

data.IQ_dBc        = zeros(length(fToneMHz), length(Gain), length(Phase));
data.CalResult     = zeros(length(fToneMHz), length(Gain), length(Phase));
data.GainEstimate  = zeros(length(fToneMHz), length(Gain), length(Phase));
data.PhaseEstimate = zeros(length(fToneMHz), length(Gain), length(Phase));
data.ArrayIndices  = 'Array(fToneMHz, Gain, Phase)';
data.Limit_dBc     = -40;

%% Test setup
DwPhyLab_Sleep; 
DwPhyLab_WriteRegister(103,     2,     1);   % IdleSDEnB
DwPhyLab_WriteRegister(  4,            1);   % RXMode
DwPhyLab_WriteRegister(144, '2:0',     3);   % DCXShift
DwPhyLab_WriteRegister(  7, '7:4',    10);   % TxIQSrcR/TxIQSrcI
DwPhyLab_WriteRegister(224, '3:2',     1);   % set RF to Standby when BB is in Receive mode   
DwPhyLab_WriteRegister(225,          255);   % TX DACs enabled
DwPhyLab_WriteRegister( 80, '7:6',     3);   % enable fixed RX gain
DwPhyLab_WriteRegister( 65,        AGAIN);   % InitAGain
DwPhyLab_Wake; pause(0.01);

DwPhyLab_WriteRegister(129-128+32768,    8); % IQ_RX = 1
DwPhyLab_WriteRegister(130-128+32768, 3, 1); % LOAD_RX_INIT = 1

%% Set up DMW96 I/Q Calibration
DwPhyLab_WriteRegister(40,   2); % sRxIQ = 2, only RXB valid for RF22 Yarden
DwPhyLab_WriteRegister(52,  80);
DwPhyLab_WriteRegister(53,  58);
DwPhyLab_WriteRegister(54,  17);
DwPhyLab_WriteRegister(55,   3);
DwPhyLab_WriteRegister(60,  26);
DwPhyLab_WriteRegister(61,  40);

hBar = waitbar(0.0, 'Measuring Image Rejection...', 'Name', mfilename, 'WindowStyle','modal');

for i = 1:length(fToneMHz)
    DwPhyLab_WriteRegister(136, mod(160+round(fToneMHz(i)/0.5),160));        % TxToneDiv

    for j = 1:length(Gain)
        valK1 = min(15, max(-16, round((-Gain(j)+IQRxGain)/GainStepRF22)));
        DwPhyLab_WriteRegister(136-128+32768, '4:0', mod(valK1+32, 32));     % K1RX_INIT 
        
        for k = 1:length(Phase)            
            valK2 = min(15, max(-16, round((-Phase(k)+IQRxPhase)/PhaseStepRF22)));
            DwPhyLab_WriteRegister(137-128+32768, '4:0', mod(valK2+32, 32)); % K2RX_INIT
            DwPhyLab_WriteRegister(128-128+32768, 10);                       % DW_CAL_MODE = 1, CALMODE = '010'
            pause(0.5);

            WriteImageDetectorCoefs(-1*fToneMHz(i));
            DwPhyLab_WriteRegister(62,  30);          % init CalGain
            DwPhyLab_WriteRegister(63,  15);          % init CalPhase
            DwPhyLab_WriteRegister(59, 244);          % enable RxIQ calibration            

            waitLimit = 10;
            while DwPhyLab_ReadRegister(58, 7) == 0 && waitLimit ~= 0
                pause(0.01);
                waitLimit = waitLimit - 1;
            end

            data.CalResult(i,j,k)     = DwPhyLab_ReadRegister(58);
            data.GainEstimate(i,j,k)  = (DwPhyLab_ReadRegister(62) - 30)*0.068;
            data.PhaseEstimate(i,j,k) = (DwPhyLab_ReadRegister(63) - 15)*0.448;        

            data.IQ_dBc(i,j,k) = MeasureFromImgPwrDet(fToneMHz(i), 80, Verbose);
            
            DwPhyLab_WriteRegister(59, 7, 0);         % disable RxIQ calibration
            DwPhyLab_WriteRegister(128-128+32768, 0); % clear CALMODE register; this will stop the signal loopback from RF22

            if Verbose,
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
    if any(isfinite(v)), % [110804BB - added else condition to fix crash]
        v = v(isfinite(v));
    else
        v = min(v);
    end
    
    data.MinIMRdB(i) = min(v);
    data.AvgIMRdB(i) = mean(v);    
end

if ~isempty(find(data.IQ_dBc > data.Limit_dBc, 1)), data.Result = 0; end

DwPhyLab_Initialize; % re-initialize to restore settings
if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));



%% Function WriteImageDetectorCoefs
function WriteImageDetectorCoefs(fToneMHz)

fs    = 20; % 20MHz at image detector input
theta = 2*pi*(fToneMHz/fs);
%cr    = cos(theta);
%ci    = sin(theta);

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


%% Function MeasureFromImgPwrDet
function IQ_dBc = MeasureFromImgPwrDet(fToneMHz, N, Verbose)

reg53 = DwPhyLab_ReadRegister(53);

%% measure power of the test tone; assuming full-scale input. 
TestTonePreShift  = 5;
TestTonePostShift = 7;
DwPhyLab_WriteRegister(53, '2:0', TestTonePreShift);
DwPhyLab_WriteRegister(53, '5:3', TestTonePostShift);
WriteImageDetectorCoefs(fToneMHz);
pause(0.1);

DwPhyLab_WriteRegister(53, 7, 1); % enable Image Detector
waitLimit = 10;
while DwPhyLab_ReadRegister(58, 0) == 0 && waitLimit ~= 0
    pause(0.01);
    waitLimit = waitLimit - 1;
end

pwr1 = 256*DwPhyLab_ReadRegister(56) + DwPhyLab_ReadRegister(57);
DwPhyLab_WriteRegister(53, 7, 0); % disable Image Detector

%% measure power of the image tone; assuming at least -36 dBc.
ImageTonePreShift  = 0;
ImageTonePostShift = 6;
DwPhyLab_WriteRegister(53, '2:0', ImageTonePreShift); 
DwPhyLab_WriteRegister(53, '5:3', ImageTonePostShift); 
WriteImageDetectorCoefs(-fToneMHz);
pause(0.1);

DwPhyLab_WriteRegister(53, 7, 1); % enable Image Detector
waitLimit = 10;
while DwPhyLab_ReadRegister(58, 0) == 0 && waitLimit ~= 0
    pause(0.01);
    waitLimit = waitLimit - 1;
end

pwr2 = 256*DwPhyLab_ReadRegister(56) + DwPhyLab_ReadRegister(57);
DwPhyLab_WriteRegister(53, 7, 0); % disable Image Detector

if Verbose, fprintf('pwr1 = %d, pwr2 = %d\n', pwr1, pwr2); end

scale = 2*(TestTonePreShift - ImageTonePreShift) + (TestTonePostShift - ImageTonePostShift);
IQ_dBc = 10*log10( pwr2 / (pwr1*2^scale) );

DwPhyLab_WriteRegister(53, reg53); % restore setting
