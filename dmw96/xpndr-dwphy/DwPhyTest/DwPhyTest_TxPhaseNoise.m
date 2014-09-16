function data = DwPhyTest_TxPhaseNoise(varargin)
% data = DwPhyTest_TxPhaseNoise(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'fcMHz','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 32;
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0, {'Measuring Phase Noise', ...
    sprintf('%d MHz, TXPWRLVL = %d',fcMHz,TXPWRLVL)}, ...
    'Name',mfilename, 'WindowStyle','modal');

for i=1:1, % dummy loop

    %% Run packets to prime ALC
    DwPhyLab_TxBurst(128,6,1000,max(TXPWRLVL),0);
    pause(0.3);
    
    LNAGAIN = floor(TXPWRLVL/32);
    AGAIN   = mod  (TXPWRLVL,32);

    DwPhyLab_WriteRegister(  7,'3:0',10);  % enable +10 MHz tone
    DwPhyLab_WriteRegister( 80,7,1);       % enable fixed RX gain
    DwPhyLab_WriteRegister( 80,6,LNAGAIN); % LNAGAIN = TXPWRLVL[5]
    DwPhyLab_WriteRegister( 65,  AGAIN  ); % AGAIN   = TXPWRLVL[4:0]
    DwPhyLab_WriteRegister(224, 170);      % RF52 in TX mode
    if DwPhyLab_RadioIsRF22                                 ; % use default setting
    else                    DwPhyLab_WriteRegister(228,  15); % T/R switch to TX        
    end
    DwPhyLab_WriteRegister(225, 255);      % TX DACs enabled
            
    % The code below forces the state of the PA enable signals based on the configuration
    % used on Debby A-D.
    BasebandID = DwPhyLab_ReadRegister(1);
    switch BasebandID,
        case {5,6}, % Mojave (DW52/74)
            if fcMHz < 3000, DwPhyLab_WriteRegister(230,'3:2',1); % enable 2.4 GHz PA
            else             DwPhyLab_WriteRegister(230,'1:0',1); % enable 5 GHz PA
            end
        case {7,8}, % Nevada (DMW96)
            if fcMHz < 3000, DwPhyLab_WriteRegister(231,hex2dec('01')); % enable 2.4 GHz PA
            else             DwPhyLab_WriteRegister(231,hex2dec('10')); % enable 5 GHz PA
            end
        otherwise, error('Unsupported Baseband');
    end

    % Setup for phase noise measurement. Save the original timeout value and increase to
    % cover the expected test time so the *OPC? query does not cause an error
    DwPhyTest_ConfigSpectrumAnalyzer('PhaseNoise');
    PSA = DwPhyLab_OpenPSA;
    DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
    Timeout = DwPhyLab_GetVisaTimeout(PSA);
    DwPhyLab_SetVisaTimeout(PSA, 30000);

    if ishandle(hBar),
        waitbar(0.2, hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
    
    DwPhyLab_SendCommand(PSA,':INIT:LPLOT');
    DwPhyLab_SendQuery(PSA,'*OPC?');
    
    if ishandle(hBar),
        waitbar(0.7, hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end

    [status, description, result] = DwPhyLab_SendQuery(PSA,':FETCH:LPLOT?',4096);
    if isempty(result)
        % error recovery: for an unknown reason, the timeout is occassionaly reset to
        % 3000 ms and the measurement times out
        DwPhyLab_SetVisaTimeout(PSA, 30000);
        result = DwPhyLab_SendQuery(PSA,':FETCH:LPLOT?',4096);
    end
    val = str2num(result); %#ok<ST2NM>
    data.PdBm = val(1);
    data.fcHz = val(2);
    data.PhaseNoise_degrms = val(3);
    data.PhaseNoise_dBc = 20*log10(data.PhaseNoise_degrms/180*pi);
    data.CFOppm = (data.fcMHz*1e6 - data.fcHz + 10e6)/data.fcMHz;

    result = DwPhyLab_SendQuery(PSA,':FETCH:LPLOT4?',262144);
    DwPhyLab_ClosePSA(PSA);
    a = str2num(result); %#ok<ST2NM>
    data.PhaseNoiseSpectrum.f   = a(1:2:length(a));
    data.PhaseNoiseSpectrum.dBc = a(2:2:length(a));

    %% TestCode for I/Q Mismatch
    if ishandle(hBar),
        waitbar(0.8, hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end

    DwPhyTest_ConfigSpectrumAnalyzer('SA',PSA);
    PSA = DwPhyLab_OpenPSA;
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:STATE ON');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE POS');
    DwPhyLab_SendQuery(PSA,'*OPC?'); pause(0.5); % make sure the PSA is configured

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X %1.6e MHZ',fcMHz+10);
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:PEAK:SEARCH:MODE MAX');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MAX');
    DwPhyLab_SendCommand(PSA,':CALC:MARKER:MODE DELTA');

    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:COUNT 10');
    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE ON');

    DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS OFF');
    DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -20.0 MHz');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.IQ_dBc = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -10.0 MHz');
    DwPhyLab_SendQuery(PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.LO_dBc = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -40.0 MHz');
    DwPhyLab_SendQuery(PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Neg30_dBc = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -19.0 MHz');
    DwPhyLab_SendQuery(PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Neg9_dBc = str2double(result);

    DwPhyLab_SendCommand(PSA,':CALC:MARKER:X -1.0 MHz');
    DwPhyLab_SendQuery(PSA,'*OPC?');
    result = DwPhyLab_SendQuery(PSA,':CALC:MARKER:Y?');
    data.Pos9_dBc = str2double(result);

    result = DwPhyLab_SendQuery(PSA,':TRACE:DATA? TRACE1',16384);
    data.trace.fMHz = 100*(-300:300)/601;
    data.trace.PdBm = str2num(result); %#ok<ST2NM>
    data.TraceResult = result;

    DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
    DwPhyLab_SendCommand(PSA,':INIT:CONTINUOUS ON');
end

DwPhyLab_Initialize; % re-initialize to stop 10 MHz tone
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE ON');
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% REVISIONS
% 2010-05-18 Add PA enable controls for the Nevada baseband
% 2010-09-27 [SM] Modified for RF22.
