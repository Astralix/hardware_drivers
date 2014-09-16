function data = DwPhyTest_TxIMR(varargin)
% data = DwPhyTest_TxIMR(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;

DwPhyLab_EvaluateVarargin(varargin, {'fcMHz','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 32;
end

%% Run Testf
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0, sprintf('Measuring Tx IMR (dB) at TXPWRLVL = %d',TXPWRLVL), ...
    'Name',mfilename, 'WindowStyle','modal');

% Setup for phase noise measurement. Save the original timeout value and increase to
% cover the expected test time so the *OPC? query does not cause an error
DwPhyTest_ConfigSpectrumAnalyzer('SA');
PSA = DwPhyLab_OpenPSA;
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
Timeout = DwPhyLab_GetVisaTimeout(PSA);

for k=1:length(data.fcMHz),

    DwPhyLab_SetChannelFreq(fcMHz(k));
    DwPhyLab_SetTxFreqPSA  (PSA, fcMHz(k));
    DwPhyLab_Wake;
    
    LNAGAIN = floor(TXPWRLVL/32);
    AGAIN   = mod  (TXPWRLVL,32);

    DwPhyLab_WriteRegister(  7,'3:0',10);  % enable +10 MHz tone
    DwPhyLab_WriteRegister( 80,7,1);       % enable fixed RX gain
    DwPhyLab_WriteRegister( 80,6,LNAGAIN); % LNAGAIN = TXPWRLVL[5]
    DwPhyLab_WriteRegister( 65,  AGAIN  ); % AGAIN   = TXPWRLVL[4:0]
    DwPhyLab_WriteRegister(224, 170);      % RF52 in TX mode
    DwPhyLab_WriteRegister(228,  15);      % T/R switch to TX
    DwPhyLab_WriteRegister(225, 255);      % TX DACs enabled

    % The code below forces the state of the PA enable signals based on the configuration
    % used on Debby A-D.
    if fcMHz(k) < 3000, DwPhyLab_WriteRegister(230,'3:2',1); % enable 2.4 GHz PA
    else                DwPhyLab_WriteRegister(230,'1:0',1); % enable 5 GHz PA
    end

    DwPhyLab_SetVisaTimeout(PSA, 30000);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:STATE ON');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE POS');
    DwPhyLab_SendQuery  (PSA,'*OPC?'); pause(0.5); % make sure the PSA is configured

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X %1.6e MHZ',fcMHz(k)+10);
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:PEAK:SEARCH:MODE MAX');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MAX');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE DELTA');

    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:COUNT 10');
    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE ON');

    DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS OFF');
    DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -20.0 MHz');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.IQ_dBc(k) = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -10.0 MHz');
    DwPhyLab_SendQuery  (PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.LO_dBc(k) = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -40.0 MHz');
    DwPhyLab_SendQuery  (PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Neg30_dBc(k) = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -19.0 MHz');
    DwPhyLab_SendQuery  (PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Neg9_dBc(k) = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -1.0 MHz');
    DwPhyLab_SendQuery  (PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Pos9_dBc(k) = str2double(result);

    result = DwPhyLab_SendQuery(PSA,':TRACE:DATA? TRACE1', 10000);
    data.trace.fMHz = 100*(-300:300)/601;
    data.trace.PdBm{k} = str2num(result); %#ok<ST2NM>

    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
    DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS ON');

    if ishandle(hBar),
        waitbar(k/length(fcMHz), hBar,sprintf('%d MHz, IMR = %1.1f dBc',fcMHz(k),data.IQ_dBc(k)));
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
   
end

data.MaxIQ_dBc = max(data.IQ_dBc);

DwPhyLab_Initialize; % re-initialize to stop 10 MHz tone
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE ON');
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
