function data = DwPhyTest_IQCalibration(varargin)
% data = DwPhyTest_IQCalibration(...)
%
%    Test DMW96 I/Q-Calibration.

% Written by Sheng Mou
% Copyright 2011 DSP Group, Inc., All Rights Reserved.

[fcMHz, Band] = DwPhyTest_ChannelList;
fToneMHz = [-3:0.5:-1 1:0.5:3];
AGAIN    = 21; 
Verbose  = 0;
RunIQCal = 1;

DwPhyLab_EvaluateVarargin(varargin);
DwPhyTest_RunTestCommon;

BasebandID = DwPhyLab_ReadRegister(1);
if ~(BasebandID >= 7 && DwPhyLab_RadioIsRF22)
    error('Unsupported Baseband + RF combination.');
end

% Due to the asymmetric TX spectrum flatness of RF22, measurements from higher frequencies
% will generate biased results. Limit the range of fToneMHz to where the spectrum is relatively 
% symmetric around DC.
if max(abs(fToneMHz)) > 3
    error('Test tone frequency out of range.');
end

data.IQ_dBc    = zeros(length(fToneMHz), length(fcMHz));
data.Limit_dBc = -40;

hBar = waitbar(0.0, 'Measuring Image Rejection...', 'Name', mfilename, 'WindowStyle','modal');

for k = 1:length(fcMHz)
    DwPhyLab_Sleep;
    DwPhyLab_SetChannelFreq(fcMHz(k));

    % utilize RF22 Rx-IQ loopback mode for testing
    DwPhyLab_WriteRegister(103,     2,     1); % IdleSDEnB
    DwPhyLab_WriteRegister(  4,            1); % RXMode
    DwPhyLab_WriteRegister(144, '2:0',     3); % DCXShift
    DwPhyLab_WriteRegister(  7, '7:4',    10); % TxIQSrcR/TxIQSrcI to tone generator
    DwPhyLab_WriteRegister(224,           85); % set radio to Standby 
    DwPhyLab_WriteRegister(225,          255); % TX DACs enabled
    DwPhyLab_WriteRegister( 80, '7:6',     3); % enable fixed RX gain
    DwPhyLab_WriteRegister( 65,        AGAIN); % InitAGain
    DwPhyLab_WriteRegister(129-128+32768,  8); % IQ_RX = 1
    DwPhyLab_WriteRegister(128-128+32768, 10); % DW_CAL_MODE = 1, CALMODE = '010'
    
    DwPhyLab_Wake; pause(0.01);

    for i = 1:length(fToneMHz)
        DwPhyLab_WriteRegister(136, mod(160+round(fToneMHz(i)/0.5),160)); % TxToneDiv
        pause(0.1);
        
        data.IQ_dBc(i,k) = MeasureFromImgPwrDet(fToneMHz(i), 80, Verbose);

        if Verbose
            fprintf('fcMHz = %d  fToneMHz = %.1f  IQ_dBc = %.2f\n\n', fcMHz(k), fToneMHz(i), data.IQ_dBc(i,k));
        end
        
        if ishandle(hBar),
            waitbar(((k-1)*length(fToneMHz) + i)/(length(fToneMHz)*length(fcMHz)), ...
                    hBar, sprintf('fcMHz = %d MHz, %.1f MHz Tone, IMR = %1.1f dB', fcMHz(k), fToneMHz(i), -data.IQ_dBc(i,k)));
        else
            break;
        end
    end
    
    if ~ishandle(hBar)
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;        
    end

    v = -1*data.IQ_dBc(:,k);
    v = v(isfinite(v));
    data.MinIMRdB(k) = min(v);
    data.AvgIMRdB(k) = mean(v);
end

if ~isempty(find(data.IQ_dBc > data.Limit_dBc)) data.Result = 0; end

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


%% Function MeasureFromImgPwrDet
function IQ_dBc = MeasureFromImgPwrDet(fToneMHz, N, Verbose)

DwPhyLab_WriteRegister(54,  17);
DwPhyLab_WriteRegister(55,   4);

%% The following number M has to cover integer number of cycles in order to 
%% align 0.5 MHz resolution tones at bin center,
%%
%%     M = 256*ImgDetLengthH + ImgDetLength + ImgDetDelay + 3
DwPhyLab_WriteRegister(52,  60);

%% measure power of the test tone; assuming full-scale input. 
TestTonePreShift  = 5;
TestTonePostShift = 6;
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
ImageTonePostShift = 5;
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

if Verbose fprintf('TonePwr = %d, ImagePwr = %d\n', pwr1, pwr2); end

scale = 2*(TestTonePreShift - ImageTonePreShift) + (TestTonePostShift - ImageTonePostShift);
IQ_dBc = 10*log10( pwr2 / (pwr1*2^scale) );


