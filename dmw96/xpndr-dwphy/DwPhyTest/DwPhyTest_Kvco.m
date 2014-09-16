function data = DwPhyTest_Kvco(varargin)
% data = DwPhyTest_Kvco(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
SweepByBand = 1;
TestAllCAPSEL = 0;
VCOREFDAC = 0:31;
Use10MHzTone = 1;
fcMHz = 2412;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'fcMHz','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 33;
end
if ~(SweepByBand && TestAllCAPSEL) && isempty(who('dCAPSEL')),
    dCAPSEL = -1:1;
end

%% Run Test
DwPhyTest_ConfigSpectrumAnalyzer('SA');
PSA = DwPhyLab_OpenPSA;
DwPhyLab_SendCommand(PSA, ':SENSE:FREQUENCY:CENTER %5.3fMHz',fcMHz);
DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:RESOLUTION 1 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 3 GHz'); 
DwPhyLab_SendCommand(PSA,':CALC:MARKER:STATE ON');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE POS');
DwPhyLab_SendQuery  (PSA,'*OPC?'); pause(0.5); % make sure the PSA is configured
DwPhyLab_ClosePSA(PSA);

DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

% Select CAPSEL range based on channel/band and test type
if SweepByBand,
    if TestAllCAPSEL,
        CAPSEL = 0:31;
    else
        if fcMHz < 3000, fcMHzList = [2412 2484];
        else             fcMHzList = [4920 5805];
        end
        for i=1:length(fcMHzList),
            DwPhyLab_SetChannelFreq(fcMHzList(i)); 
            DwPhyLab_Wake;
            data.ReadCAPSEL(i) = DwPhyLab_ReadRegister(256+42,'4:0');
        end
        CAPSEL = (min(data.ReadCAPSEL)+min(dCAPSEL)) : (max(data.ReadCAPSEL)+max(dCAPSEL));
    end
else
    data.ReadCAPSEL = DwPhyLab_ReadRegister(256+42,'4:0');
    CAPSEL = data.ReadCAPSEL + dCAPSEL;
end
data.CAPSEL = CAPSEL;

hBar = waitbar(0.0, {'Kvco', sprintf('%d MHz, TXPWRLVL = %d',fcMHz,TXPWRLVL)}, ...
    'Name',mfilename, 'WindowStyle','modal');

LNAGAIN = floor(TXPWRLVL/32);
AGAIN   = mod  (TXPWRLVL,32);

if Use10MHzTone, DwPhyLab_WriteRegister(7,'3:0',10); % enable +10 MHz tone
else             DwPhyLab_WriteRegister(7,'3:0',15); % DAC input = 512 (mid-scale)
end
DwPhyLab_WriteRegister( 80,7,1);       % enable fixed RX gain
DwPhyLab_WriteRegister( 80,6,LNAGAIN); % LNAGAIN = TXPWRLVL[5]
DwPhyLab_WriteRegister( 65,  AGAIN  ); % AGAIN   = TXPWRLVL[4:0]
DwPhyLab_WriteRegister(224, 170);      % RF52 in TX mode
DwPhyLab_WriteRegister(228,  15);      % T/R switch to TX
DwPhyLab_WriteRegister(225, 255);      % TX DACs enabled

% The code below forces the state of the PA enable signals based on the configuration
% used on Debby A-D.
if fcMHz < 3000, DwPhyLab_WriteRegister(230,'3:2',1); % enable 2.4 GHz PA
else             DwPhyLab_WriteRegister(230,'1:0',1); % enable 5 GHz PA
end

PSA = DwPhyLab_OpenPSA;

%%% SWEEP OVER CAPSEL
for i=1:length(CAPSEL),

    DwPhyLab_WriteRegister(256+35,bin2dec('00100000') + CAPSEL(i));
    DwPhyLab_WriteRegister(256+36,0,1);
    DwPhyLab_WriteRegister(256+36,0,0);

    DwPhyLab_WriteRegister(256+24, 1+2*VCOREFDAC(1));

    DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:CENTER %5.3fMHz',fcMHz);
    DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:RESOLUTION 100 kHz');
    if i == 1
        if fcMHz < 3000, DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 2 GHz'); 
        else             DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 3 GHz'); 
        end
    else
        DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 200 MHz');
    end
    DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');
    f0 = MeasurePeak(PSA);
    fcMHz = f0 / 1e6;  % update center frequency based on current CAPSEL
    DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:CENTER %5.3fGHz',f0/1e9);
    DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 30 MHz');
    DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:RESOLUTION 50 kHz');
    DwPhyLab_SendQuery  (PSA,'*OPC?'); 
    pause(1); MeasurePeak(PSA); % delay + dummy measurement to allow for settling
    
    for k = 1:length(VCOREFDAC),
    
        DwPhyLab_WriteRegister(256+24, 1+2*VCOREFDAC(k));
        [data.f(i,k), data.P(i,k)] = MeasurePeak(PSA);
        
        % Recenter spectrum anaylzer
        if (data.f(i,k) - f0) > 5e6,
            f0 = data.f(i,k) + 12e6;
            DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:CENTER %5.3fGHz',(f0)/1e9);
        end

        if ishandle(hBar),
            x = ((i-1)*length(VCOREFDAC) + k) / (length(VCOREFDAC)*length(CAPSEL));
            waitbar(x, hBar, sprintf('CAPSEL=%d, VCOREFDAC=%2d, f = %1.1f MHz\n', ...
                CAPSEL(i), VCOREFDAC(k), data.f(i,k)/1e6) );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
        
    end

    data.xReadCAPSEL(i) = DwPhyLab_ReadRegister(256+42,'4:0'); % read-back as sanity check
    DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS ON');
end

% Adjust measured tone for IF offset
if Use10MHzTone,
   data.f = data.f - 10e6;
end

%DwPhyLab_Initialize; % re-initialize to stop 10 MHz tone
DwPhyLab_ClosePSA(PSA);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% MeasurePeak
function [F,P] = MeasurePeak(PSA)

DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS OFF');
DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');
DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
DwPhyLab_SendQuery  (PSA,'*OPC?'); 

DwPhyLab_SendCommand(PSA,':CALC:MARKER:PEAK:SEARCH:MODE MAX');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MAX');
DwPhyLab_SendQuery  (PSA,'*OPC?'); 

DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');
DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
DwPhyLab_SendQuery  (PSA,'*OPC?'); 

DwPhyLab_SendCommand(PSA,':CALC:MARKER:PEAK:SEARCH:MODE MAX');
DwPhyLab_SendCommand(PSA,':CALC:MARKER:MAX');
DwPhyLab_SendQuery  (PSA,'*OPC?'); 

[status,description,result] = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
if status ~= 0, error(description); end
P = str2double(result);

[status,description,result] = DwPhyLab_SendQuery(PSA,':CALC:MARKER:X?');
if status ~= 0, error(description); end
F = str2double(result);

