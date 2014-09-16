function data = DwPhyTest_TxSpectrumMask(varargin)
% data = DwPhyTest_TxspectrumMask(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 1500;
fcMHz = 2412;
PacketType = 0;

DwPhyLab_EvaluateVarargin(varargin);

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
    if isempty(TXPWRLVL), TXPWRLVL = 63; end;
end
if isempty(who('Standard'))
    if any(Mbps == [1 2 5.5 11]),
        Standard = '802.11b';
    elseif any(Mbps == [6 9 12 18 24 36 48 54]),
        Standard = '802.11a';
    else
        Standard = '802.11n';
    end
end
        

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0,'Configuring Test...', 'Name',mfilename, 'WindowStyle','modal');

for i=1:1, % dummy loop for future use and so break from hBar works

    DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, TXPWRLVL, PacketType)

    PSA = DwPhyLab_OpenPSA;
    DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
    DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
    DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
    DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO:RULES ACCURACY');

    switch upper(Standard),
        case '802.11A', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11A');
        case '802.11B', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11B');
        case '802.11G', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11G');
        case '802.11N', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11N');
        otherwise, error('Unknown Standard "%s"',Standard);
    end

    DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:EAMEAS YES');
    DwPhyLab_SendCommand(PSA,':CONFIGURE:SEMASK');
    DwPhyLab_SendCommand(PSA,':SENSE:SEMASK:AVERAGE:STATE OFF');

    % save the original timeout value and increase to cover the expected test time so the
    % *OPC? query does not cause an error message
    [status, description, Timeout] = DwPhyLab_GetVisaTimeout(PSA);
    if status ~= 0, error(description); end
    DwPhyLab_SetVisaTimeout(PSA, 30000);

    DwPhyLab_SendCommand(PSA,':INITIATE:SEMASK');
    DwPhyLab_SendCommand(PSA,':SENSE:POWER:RANGE:AUTO ONCE');
    DwPhyLab_SendQuery  (PSA,'*OPC?');

    DwPhyLab_SendCommand(PSA,':SENSE:SEMASK:AVERAGE:STATE ON');
    DwPhyLab_SendCommand(PSA,':SENSE:SEMASK:TYPE PSDRef');

    if ishandle(hBar)
        waitbar( 0.2, hBar, sprintf('Measuring at %d MHz, TXPWRLVL = %d',fcMHz,TXPWRLVL) );
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
    end

    DwPhyLab_SendCommand(PSA,':INITIATE:SEMASK');
    DwPhyLab_SendQuery  (PSA,'*OPC?');

    if ishandle(hBar)
        waitbar(0.9, hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
    end

    [status, description, result] = DwPhyLab_SendQuery(PSA,':FETCH:SEMASK2?', 20000);
    if isempty(result)
        % error recovery: for an unknown reason, the timeout is occassionaly reset to
        % 3000 ms and the measurement times out
        DwPhyLab_SetVisaTimeout(PSA, 30000);
        result = DwPhyLab_SendQuery(PSA,':FETCH:SEMASK2?', 20000);
    end
    data.PSDdBr = str2num(result); %#ok<ST2NM>

    result = DwPhyLab_SendQuery(PSA,':FETCH:SEMASK4?', 20000);
    data.Limit = str2num(result); %#ok<ST2NM>

    result = DwPhyLab_SendQuery(PSA,':FETCH:SEMASK12?');
    data.Pout_dBm = str2double(result) + data.Ploss;

    data.fMHz = -50:(100/(length(data.PSDdBr)-1)):50;
    data.Result = all(data.PSDdBr <= data.Limit);

    data.RefPSD = max(data.PSDdBr);
    data.PSDdBr = data.PSDdBr - data.RefPSD;
    data.Limit  = data.Limit  - data.RefPSD;

end    
    
%DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON'); % continuous sweep
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE ON');
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
DwPhyLab_TxBurst(0);

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% REVISIONS
% 2008-08-12 Added one-shot error recovery for timeout in mask measurement
