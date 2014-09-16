function data = DwPhyTest_PERvChanl(varargin)
% data = DwPhyTest_PERvChanl(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.
%
%    If InterferenceType = {'ACI-OFDM','ACI-CCK',...} is a parameter, this function
%    will load the specified interference waveform and evaluate it at fiMHz (either a
%    single point or an array with length matching fcMHz) and PidBm.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
fcMHz = DwPhyTest_ChannelList;
fcMHzOfs = 0;
PrdBm = -65;
PktsPerWaveform = 1;
MaxPkts = 1000;
MinFail = 100;
SleepWakeDelay = 0;

DwPhyLab_EvaluateVarargin(varargin);
PktsPerSequence = max( 1000, 10*PktsPerWaveform);

if ~isempty(who('InterferenceType')),
    if isempty(who('fiMHz','PidBm'))
        error('Must specify both fiMHz and PidBm if InterferenceType is specified');
    end
end

if length(fcMHzOfs) == 1,
    fcMHzOfs = fcMHzOfs * ones(size(fcMHz));
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz + fcMHzOfs);

if SleepWakeDelay, DwPhyLab_Sleep; end

if ~isempty(who('InterferenceType'))
    % Load the interference waveform into the AWG520 and configure the second ESG to
    % use this signal. Assume the input waveform has a high duty cycle: enable ALC.
    DwPhyTest_LoadAWG(InterferenceType,1,1);
    ESG2 = DwPhyLab_OpenESG(2);
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:STATE ON');
    DwPhyLab_SendCommand(ESG2,':OUTPUT:MODULATION:STATE ON');
    DwPhyLab_SendCommand(ESG2,':OUTPUT:STATE ON');
    DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:STATE ON');
    data.Piloss = DwPhyLab_RxCableLoss(fiMHz);
end

DwPhyLab_LoadPER(data);
hBar = waitbar(0.0, sprintf('%g Mbps, Pr = %1.1f dBm',Mbps,PrdBm), ... 
    'Name',mfilename, 'WindowStyle','modal');
Npts = length(fcMHz);

data.Npkts = zeros(1, Npts);
data.ReceivedFragmentCount = zeros(1, Npts);
data.ErrCount = zeros(1, Npts);
data.FCSErrorCount = zeros(1, Npts);

for i=1:length(fcMHz),
    pause(SleepWakeDelay);
    DwPhyLab_SetChannelFreq(fcMHz(i));
    DwPhyLab_Wake;
    DwPhyLab_SetRxFreqESG(fcMHz(i)+fcMHzOfs(i));
    DwPhyLab_SetRxPower(PrdBm + data.Ploss(i));

    if ~isempty(who('InterferenceType'))
        DwPhyLab_SetRxFreqESG(2, fiMHz(min(i, length(fiMHz))) );
        DwPhyLab_SetRxPower(2, PidBm+data.Piloss(i));
        pause(0.02);
    end

    while (data.Npkts(i) < MaxPkts) && (data.ErrCount(i) < MinFail) && ishandle(hBar),
    
        result = DwPhyLab_GetPER;
        data.ReceivedFragmentCount(i) = data.ReceivedFragmentCount(i) + result.ReceivedFragmentCount;
        data.Npkts(i) = data.Npkts(i) + PktsPerSequence;
        data.ErrCount(i) = data.Npkts(i) - data.ReceivedFragmentCount(i);
        data.FCSErrorCount(i) = data.FCSErrorCount(i) + result.FCSErrorCount;
        data.PER(i) = data.ErrCount(i) / data.Npkts(i);
        data.RSSI(i) = result.RSSI;

        if result.ReceivedFragmentCount,
            data.LastRxBitRate(i) = result.RxBitRate;
            data.LastLength(i)    = result.Length;
        end
        if ~isempty(who('Verbose')), data.result(i) = result; end;

        if ishandle(hBar)
            waitbar( i/Npts, hBar, { sprintf('%g Mbps, Pr = %1.1f dBm',Mbps,PrdBm),...
                sprintf('fcMHz = %g, PER = %1.2f%%',data.fcMHz(i),100*data.PER(i)) } );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
    if ~ishandle(hBar), break; end
end

DwPhyLab_DisableESG;
try DwPhyLab_SetRxPower(2, -136); catch end

if isfield(data,'PERLimit')
    if any(data.PER > data.PERLimit), data.Result = 0; end
else
    if any(data.PER > 0.1), data.Result = 0; end
end
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% REVISIONS
% 2008-05-09 Allow sleep-dwell-wake between channels