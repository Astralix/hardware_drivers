function data = DwPhyTest_PERvTIFS(varargin)
% data = DwPhyTest_PERvPrdBm(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
fcMHz = 2417;
PrdBm = -50;
PktsPerWaveform = 1;
Broadcast = 0; %#ok<NASGU>
MaxPkts = 10000;
MinFail = 1000;
T_IFS_Range = 60e-6 + (8:34)*1e-6;
DwPhyLab_EvaluateVarargin(varargin);
if isempty(who('PktsPerSequence'))
    PktsPerSequence = max( 1000, 10*PktsPerWaveform);
end

%% Run Test
DwPhyTest_RunTestCommon;
DwPhyLab_WriteRegister(hex2dec('065090A8'),1); % ACK only at 6 Mbps

data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
data.DACHI = DwPhyLab_ReadRegister(256+25,'4:0');
data.DACLO = DwPhyLab_ReadRegister(256+26,'4:0');

hBar = waitbar(0.0,sprintf('%g Mbps',Mbps(1)), 'Name',mfilename, 'WindowStyle','modal');
Npts = length(T_IFS_Range);
data.Npkts = zeros(1, Npts);
data.ReceivedFragmentCount = zeros(1, Npts);
data.ErrCount = zeros(1, Npts);
data.FCSErrorCount = zeros(1, Npts);
DwPhyLab_SetRxPower(PrdBm + data.Ploss);

for i=1:Npts
    data.T_IFS = data.T_IFS_Range(i);

    DwPhyLab_LoadPER(data);
    j = 0;

    while (data.Npkts(i) < MaxPkts) && (data.ErrCount(i) < MinFail) && ishandle(hBar),
    
        j = j+1;
        result = DwPhyLab_GetPER;
        data.ReceivedFragmentCount(i) = data.ReceivedFragmentCount(i) + result.ReceivedFragmentCount;
        data.Npkts(i) = data.Npkts(i) + PktsPerSequence * length(Mbps);
        data.ErrCount(i) = data.Npkts(i) - data.ReceivedFragmentCount(i);
        data.FCSErrorCount(i) = data.FCSErrorCount(i) + result.FCSErrorCount;
        data.PER(i) = data.ErrCount(i) / data.Npkts(i);
        data.RSSI(i) = result.RSSI;
        if result.ReceivedFragmentCount,
            data.LastRxBitRate = result.RxBitRate;
            data.LastLength    = result.Length;
        end
        if ~isempty(who('Verbose')), data.result(i,j) = result; end;
        data.CAPSEL(i) = bitand(31,DwPhyLab_ReadRegister(256+42));

        if ishandle(hBar)
            waitbar( i/Npts, hBar, sprintf('T_IFS = %1.1f usec, PER = %1.2f%%',data.T_IFS*1e6,100*data.PER(i)) );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
    fprintf('T_IFS = %1.1f usec, PER = %1.2f\n',data.T_IFS*1e6,100*data.PER(i));
    if ~ishandle(hBar), break; end
end

DwPhyLab_DisableESG;
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
