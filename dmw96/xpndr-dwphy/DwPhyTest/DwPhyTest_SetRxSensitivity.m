function data = DwPhyTest_SetRxSensitivity(varargin)
% data = DwPhyTest_SetRxSensitivity(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
LengthPSDU = 1000;
fcMHz = 2484;
PktsPerWaveform = 1;
Broadcast = 1;
MinPrdBm = -101;
MinPER  = 1e-2;
StepPrdBm = 1;
MaxPkts = 2000; %#ok<NASGU>
MinFail =  200; %#ok<NASGU>
SetRxSensitivity = -97:3:-55;

DwPhyLab_EvaluateVarargin(varargin);
PktsPerSequence = max(1000, 10*PktsPerWaveform);

if isempty(who('Mbps')),
    if isempty(who('RxMode'))
        RxMode = 1 + 2*(fcMHz < 3000);
    end
    switch RxMode,
        case 1, Mbps = [6 9 12 18 24 36 48 54];
        case 2, Mbps = [1 2 5.5 11];
        case 3, Mbps = [1 2 5.5 11 6 9 12 18 24 36 48 54];
        otherwise, error('Invalid RxMode');
    end
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);

hBar = waitbar(0.0,'', 'Name',mfilename, 'WindowStyle','modal');

for i=1:length(Mbps),
    if ishandle(hBar)
        waitbar( i/length(Mbps), hBar, {'',sprintf('Running %g Mbps',Mbps(i))} );
        set(hBar,'UserData',[(i-1) 1]/length(Mbps));
    else
        fprintf('*** TEST ABORTED ***\n'); 
        data.Result = -1;
        break;
    end
    if isempty(who('options')), options = {}; end
    if isempty(who('ShortPreamble')), ShortPreamble = 1; end
    if isempty(who('T_IFS')), T_IFS = []; end

    DwPhyLab_LoadPER(Mbps(i), LengthPSDU, PktsPerSequence, PktsPerWaveform, ...
        Broadcast, ShortPreamble, T_IFS, options);

    for j=1:length(SetRxSensitivity),
        if ishandle(hBar)
            N = length(Mbps) * length(SetRxSensitivity);
            k = ((i-1)*length(SetRxSensitivity) + j);
            waitbar( k/N, hBar, {'',sprintf('Running %g Mbps at RxSensitivity = %d',...
                Mbps(i),SetRxSensitivity(j))} );
            set(hBar,'UserData',[(k-1) 1]/N);
        else
            fprintf('*** TEST ABORTED ***\n'); 
            data.Result = -1;
            break;
        end
        DwPhyLab_Sleep;
        DwPhyLab_SetRxSensitivity(SetRxSensitivity(j));
        if ~isempty(who('UserReg')), DwPhyLab_WriteUserReg(UserReg); end
        DwPhyLab_Wake;
        data.GetRxSensitivity(i,j) = DwPhyLab_GetRxSensitivity;
        PrdBm{i,j} = max(MinPrdBm,(data.GetRxSensitivity(i,j) - 6)) : StepPrdBm : -30;
        data.PrdBm = PrdBm{i,j};
        result = DwPhyTest_SensitivityCore(data, hBar);

        data.PER{i,j}      = result.PER;
        data.RSSI{i,j}     = result.RSSI;
        data.Npkts{i,j}    = result.Npkts;
        data.ErrCount{i,j} = result.ErrCount;
        data.result{i,j}   = result.result;
        data.Sensitivity(i,j)= result.Sensitivity;
        
        if ~isempty(who('Verbose')) && Verbose,
            fprintf('   %2d Mbps, Set/Measured Sensitivity = %d, %1.1f dBm\n',...
                data.Mbps(i), data.GetRxSensitivity(i,j), data.Sensitivity(i,j) );
        end
        
    end
    data.LegendStr{i}  = sprintf('%g Mbps',Mbps(i));
end

data.PrdBm = PrdBm;

DwPhyLab_SetRxPower(-136);
DwPhyLab_DisableESG;

%% Reduce size of data structure
if ~isempty(who('Concise'))
    if Concise,
        data = rmfield(data,'result');
        data = rmfield(data,'RegList');
    end
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
