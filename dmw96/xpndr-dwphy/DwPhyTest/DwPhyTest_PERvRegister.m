function data = DwPhyTest_PERvRegister(varargin)
% data = DwPhyTest_PERvRegister(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.
%
%    Specify register to vary with RegAddr, RegRange, and RegField

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
fcMHz = 2412;
PrdBm = -50;
PktsPerWaveform = 1;
MaxPkts = 1000;
MinFail = 100;

DwPhyLab_EvaluateVarargin(varargin);

if isempty(who('RegAddr')) || isempty(who('RegRange')),
    error('Must specify at least RegAddr and RegRange');
end
if isempty(who('RegField')), RegField = '7:0'; end
if isnumeric(RegField), RegField = sprintf('%d:%d',RegField,RegField); end

if isempty(who('PktsPerSequence'))
    PktsPerSequence = max( 1000, 10*PktsPerWaveform);
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);

DwPhyLab_LoadPER(data);
DwPhyLab_SetRxPower(PrdBm + data.Ploss);
hBar = waitbar(0.0,sprintf('%g Mbps',Mbps), 'Name',mfilename, 'WindowStyle','modal');

Npts = length(RegRange);
data.Npkts = zeros(1, Npts);
data.ReceivedFragmentCount = zeros(1, Npts);
data.ErrCount = zeros(1, Npts);
data.FCSErrorCount = zeros(1, Npts);

for i = 1:Npts,
    
    DwPhyLab_WriteRegister(RegAddr, RegField, RegRange(i));
    j = 0;
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

        if ishandle(hBar)
            waitbar( i/Npts, hBar, sprintf('Reg(0x%s) = %d (0x%s), PER = %1.2f%%', ...
                dec2hex(RegAddr,3), RegRange(i), dec2hex(RegRange(i),2), 100*data.PER(i)) );
        end
    end
    if ~ishandle(hBar), 
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break; 
    end
    
end    

DwPhyLab_DisableESG;
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
