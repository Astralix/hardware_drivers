function data = DwPhyTest_SensitivityCore(varargin)
% data = DwPhyTest_SensitivityCore(...)
%
%    This function performs a basic sensitivity measurement using a waveform already
%    loaded into the signal generator. It performs no register programming and is
%    typically used as part of an overall test routine.
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Parameters include PrdBm, MaxPkts, MinFail, MinPER, SensitivityLevel, and hBar

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

data.Description = mfilename;
data.Timestamp = datestr(now);
data.Result = 1;

%% Default and User Parameters
PrdBm = -99:50;
MaxPkts = 10000;
MinFail = 100;
MinPER  = 1e-2;
SensitivityLevel = 0.1;
hBar = [];
UseWaitBar = 1;
hBarClose = 1;

DwPhyLab_EvaluateVarargin(varargin, {'PrdBm','MaxPkts','MinFail','MinPER','SensitivityLevel'});
DwPhyLab_RecordParameters;

if isempty(who('Ploss'))
    data.Ploss = DwPhyLab_RxCableLoss(DwPhyLab_Parameters('fcMHz'));
end

%% Get Waitbar Data or Create a New One
if UseWaitBar

    hBarOfs = 0;
    hBarLen = 1;
    hBarStr = {};

    if isempty(hBar),
        hBar = waitbar(0.0,'Running...','Name',mfilename,'WindowStyle','modal');
    else
        if ishandle(hBar),
            hBarClose = 0;
            UserData = get(hBar,'UserData');
            if length(UserData) == 2,
                hBarOfs = UserData(1);
                hBarLen = UserData(2);
            end
            hBarStr = get(get(get(hBar,'Children'),'Title'),'String');
            if ischar(hBarStr)
                clear hBarStr;
                hBarStr{1} = get(get(get(hBar,'Children'),'Title'),'String');
            end
        end
    end
    hBarStrIndex = length(hBarStr) + 1;
end

%% Run Test
% Initialize results arrays to all zeros
Z = zeros(1, length(PrdBm));
data.PER   = Z; data.RSSI     = Z;
data.Npkts = Z; data.ErrCount = Z; data.FCSErrorCount = Z;

DwPhyLab_SetRxPower(PrdBm(1) + data.Ploss); pause(0.1);
StepPrdBm = mean(diff(PrdBm));

for i=1:length(PrdBm),
    
%     % For small steps, first set the power back. Observed measurements show PERvPrdBm
%     % curves can be non-monotonic when taking small steps. It is not clear why, but may 
%     % be due to the ESG. Because PER curves are typically repeatable, move back so the 
%     % step is larger to see if this helps (Added 2008-06-09)
%     if StepPrdBm < 1,
%         DwPhyLab_SetRxPower(PrdBm(i)+data.Ploss-2); 
%     end;
    
    DwPhyLab_SetRxPower(PrdBm(i) + data.Ploss);
    [data.PER(i),data.RSSI(i),data.Npkts(i),data.ErrCount(i), data.FCSErrorCount(i), ...
        data.result{i}] = GetPER(hBar, MaxPkts, MinFail);

    if UseWaitBar,
        if ishandle(hBar)
            w = hBarOfs + hBarLen * (i/length(PrdBm));
            hBarStr{hBarStrIndex} = sprintf('Pr = %1.1f dBm, PER = %1.2f%%',PrdBm(i),100*data.PER(i));
            waitbar(w,hBar,hBarStr);
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            data.Sensitivity = NaN;
            return ;
        end
    end

    if data.PER(i) < MinPER,
        break;
    end
end

% Compute Sensitivity
data.Sensitivity = DwPhyTest_CalcSensitivity(PrdBm, data.PER, SensitivityLevel);
if ~isempty(who('Concise'))
    if Concise,
        data = rmfield(data, 'result');
    end
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar) && hBarClose, close(hBar); end;

%% FUNCTION GetPER
function [PER, RSSI, Npkts, ErrCount, FCSErrorCount, result] = GetPER(hBar, MaxPkts, MinFail)
Npkts = 0;
ErrCount = 0;
FCSErrorCount = 0;
PER = 1;
PktsPerSequence = DwPhyLab_Parameters('PktsPerSequencePER');

while (Npkts < MaxPkts) && (ErrCount < MinFail),
    
    result = DwPhyLab_GetPER;
    ErrCount = ErrCount + (PktsPerSequence - result.ReceivedFragmentCount);
    FCSErrorCount = FCSErrorCount + result.FCSErrorCount;
    Npkts = Npkts + PktsPerSequence;
    PER = ErrCount / Npkts;
    RSSI = result.RSSI;

    if ~isempty(hBar) && ~ishandle(hBar), return; end % user abort
end

%% REVISIONS
% 2008-06-09 Added step-back before forward when stepping power < 1 dB [removed]
% 2008-11-06 Increased default MinPER from 0.001 to 0.01 to speed up tests