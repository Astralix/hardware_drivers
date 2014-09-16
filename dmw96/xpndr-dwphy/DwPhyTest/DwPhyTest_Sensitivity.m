function data = DwPhyTest_Sensitivity(varargin)
% data = DwPhyTest_Sensitivity(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, fcMHzOfs, PrdBm,
%    PktsPerWaveform, MaxPkts, MinFail, MinPER, RxMode, UserReg, and Concise

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
DiversityMode = 0;
LengthPSDU = 1000;
fcMHz = 2412;
PktsPerWaveform = 1;
PrdBm = -99:-60;
Broadcast = 1;
MaxPkts = 10000; %#ok<NASGU>
MinFail = 100; %#ok<NASGU>

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

% Build list of test limits (min sensitivity or max input)
if ~isfield(data,'RequiredSensitivity')
    MbpsOrder = [  1   2 5.5  11   6   9  12  18  24  36  48  54  72 6.5  13 19.5  26  39  52 58.5 65];
    TestLimit = [-83 -80 -79 -76 -82 -81 -79 -77 -74 -70 -66 -65 -59 -82 -79  -77 -74 -70 -66 -65 -64];
    if min(PrdBm) > -50,
        TestLimit = (-30+10*(fcMHz<3000))*ones(size(TestLimit));
    end
    for i=1:length(data.Mbps),
        k = find(data.Mbps(i) == MbpsOrder);
        data.RequiredSensitivity(i) = TestLimit(k);
    end
end

hBar = waitbar(0.0,'', 'Name',mfilename, 'WindowStyle','modal');

for k=1:length(Mbps),

    if ishandle(hBar)
        waitbar( (k-1)/length(Mbps), hBar, {'',sprintf('Running %g Mbps',Mbps(k))} );
        set(hBar,'UserData',[(k-1) 1]/length(Mbps));
    else
        fprintf('*** TEST ABORTED ***\n'); 
        data.Result = -1;
        break;
    end

    if isempty(who('options')), options = {}; end
    if isempty(who('ShortPreamble')), ShortPreamble = 1; end
    if isempty(who('T_IFS')), T_IFS = []; end
    DwPhyLab_LoadPER(Mbps(k), LengthPSDU, PktsPerSequence, PktsPerWaveform, ...
        Broadcast, ShortPreamble, T_IFS, options);
    result = DwPhyTest_SensitivityCore(data, hBar);
    
    data.PER(k,:)      = result.PER';
    data.RSSI(k,:)     = result.RSSI';
    data.Npkts(k,:)    = result.Npkts';
    data.ErrCount(k,:) = result.ErrCount';
    data.result{k}     = result.result;
    data.Sensitivity(k)= result.Sensitivity;
    data.LegendStr{k}  = sprintf('%g Mbps',Mbps(k));

    if data.RequiredSensitivity(k) < -50,
        if data.Sensitivity(k) > data.RequiredSensitivity(k), data.Result = 0; end % min input
    else
        if data.Sensitivity(k) < data.RequiredSensitivity(k), data.Result = 0; end % max input
    end
end
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
