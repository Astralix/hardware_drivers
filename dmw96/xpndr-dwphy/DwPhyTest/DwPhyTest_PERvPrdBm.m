function data = DwPhyTest_PERvPrdBm(varargin)
% data = DwPhyTest_PERvPrdBm(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
fcMHz = 2412;
fcMHzOfs = 0;
PrdBm = -100:1:0;
PktsPerWaveform = 1;
MaxPkts = 1000;
MinFail = 100;
DiversityMode = 0;

DwPhyLab_EvaluateVarargin(varargin);
if isempty(who('PktsPerSequence'))
    PktsPerSequence = max( 1000, 10*PktsPerWaveform); %#ok<NASGU>
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz + fcMHzOfs);

LoadResult = DwPhyLab_LoadPER(data);
PktsPerSequence = DwPhyLab_Parameters('PktsPerSequencePER');
hBar = waitbar(0.0,sprintf('%g Mbps',Mbps(1)), 'Name',mfilename, 'WindowStyle','modal');
if LoadResult.Result == -1, close(hBar); end; % propagate close from DwPhyLab_LoadPER
Npts = length(data.PrdBm);

data.Npkts = zeros(1, Npts);
data.ReceivedFragmentCount = zeros(1, Npts);
data.ErrCount = zeros(1, Npts);
data.FCSErrorCount = zeros(1, Npts);

for i=1:length(PrdBm),
    j = 0;
    DwPhyLab_SetRxPower(PrdBm(i) + data.Ploss);

    while (data.Npkts(i) < MaxPkts) && (data.ErrCount(i) < MinFail) && ishandle(hBar),
    
        j = j+1;
        result = DwPhyLab_GetPER;
        data.ReceivedFragmentCount(i) = data.ReceivedFragmentCount(i) + result.ReceivedFragmentCount;
        data.Npkts(i) = data.Npkts(i) + PktsPerSequence;
        data.ErrCount(i) = data.Npkts(i) - data.ReceivedFragmentCount(i);
        data.FCSErrorCount(i) = data.FCSErrorCount(i) + result.FCSErrorCount;
        data.PER(i) = data.ErrCount(i) / data.Npkts(i);
        data.RSSI(i) = result.RSSI;
        if result.ReceivedFragmentCount,
            data.LastRxBitRate = result.RxBitRate;
            data.LastLength    = result.Length;
        end
        if ~isempty(who('Verbose')), data.result(i,j) = result; end;

        if exist('GetPacketRSSI','var') && GetPacketRSSI == 1,
            DwPhyLab_SendCommand('ESG','*TRG');
            pause(DwPhyLab_Parameters('TsequencePER'));
            X = DwPhyMex('RvClientRxRSSI');
            for k=1:2:length(X),
                [X(k) X(k+1)] = DwPhyMex('DwPhy_ConvertHeaderRSSI',X(k),X(k+1));
            end
            PacketRSSI0{j} = X(1:2:length(X)); %#ok<AGROW>
            PacketRSSI1{j} = X(2:2:length(X)); %#ok<AGROW>
        end
        
        if ishandle(hBar)
            waitbar( i/Npts, hBar, sprintf('Pr = %1.1f dBm, PER = %1.2f%%',data.PrdBm(i),100*data.PER(i)) );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
    
    if exist('GetPacketRSSI','var') && GetPacketRSSI == 1,
        data.PacketRSSI0{i} = PacketRSSI0{1:length(PacketRSSI0)};
        data.PacketRSSI1{i} = PacketRSSI1{1:length(PacketRSSI1)};
        data.MeanPacketRSSI0(i) = GetAverageRSSI(data.PacketRSSI0{i});
        data.MeanPacketRSSI1(i) = GetAverageRSSI(data.PacketRSSI1{i});
        data.MeanPacketRSSI(i) = max([data.MeanPacketRSSI0(i) data.MeanPacketRSSI1(i)]);
    end
    
    if ~ishandle(hBar), break; end
end

% Check RSSI Accuracy
if exist('GetPacketRSSI','var') && GetPacketRSSI == 1,
    if data.fcMHz < 3000,
        k = find((data.PrdBm >= -82) & (data.PrdBm <= -20));
    else
        k = find((data.PrdBm >= -82) & (data.PrdBm <= -30));
    end
    e = data.MeanPacketRSSI(k) - (data.PrdBm(k) + 100);
    data.AccurateRSSI = max(abs(e)) <= 5;
end

DwPhyLab_DisableESG;
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% GetAverageRSSI
function y = GetAverageRSSI(x)
y = mean(x);
% Code below is a work-around for an early driver that returned a few packets w/o RSSI
% if mean(x) > 6,
%     y = mean(x(x>0));
% else
%     y = mean(x);
% end

%% REVISIONS
% 2011-03-16 SM: Added fcMHzOfs to Rx cable loss.
% 2011-04-29 BB: Filter RSSI = 0 readings to avoid problem with DMW96 "Phantom" RX Packets
