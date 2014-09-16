function data = DwPhyTest_PERFloor(varargin)
% data = DwPhyTest_PERFloor(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, PrdBm, fcMHz, PaxPkts, PktsPerWaveform

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

DwPhyLab_Setup;
data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
PrdBm = -50;
fcMHz = 2412;
fcMHzOfs = 0.0;
Broadcast = 1;
MaxPkts = 1e6;
PktsPerWaveform = 10;
UserReg = {};

DwPhyLab_EvaluateVarargin(varargin);

data.Mbps = Mbps;
data.PrdBm = PrdBm;
data.fcMHz = fcMHz;
data.fcMHzOfs = fcMHzOfs;
data.MaxPkts = MaxPkts;
data.PktsPerWaveform = PktsPerWaveform;
data.UserReg = UserReg;

if data.Mbps > 50,
    data.PktsPerSequence = max(10000,100*data.PktsPerWaveform);
else
    data.PktsPerSequence = max( 1000, 10*data.PktsPerWaveform);
end

%% Run Test
DwPhyLab_Initialize;
DwPhyLab_SetRxFreqESG(data.fcMHz + fcMHzOfs);
DwPhyLab_SetChannelFreq(data.fcMHz);
DwPhyLab_WriteUserReg(UserReg);
DwPhyLab_Wake;

data.Ploss = DwPhyLab_RxCableLoss(data.fcMHz);

DwPhyLab_SetRxPower(data.PrdBm + data.Ploss);
if isempty(who('options')), options = {}; end
if isempty(who('ShortPreamble')), ShortPreamble = 1; end
if isempty(who('T_IFS')), T_IFS = []; end
DwPhyLab_LoadPER(Mbps, LengthPSDU, data.PktsPerSequence, PktsPerWaveform, Broadcast, ...
    ShortPreamble, T_IFS, options);

data.RegList = DwPhyLab_RegList;
data.ReceivedFragmentCount = [];
data.Npkts = 0;
k = 0;

hBar = waitbar(0.0,'Loading Waveform...','Name',mfilename,'WindowStyle','modal');

while data.Npkts < data.MaxPkts,

    k = k+1;
    result = DwPhyLab_GetPER;
    data.ReceivedFragmentCount(k) = result.ReceivedFragmentCount;
    data.Npkts = k * data.PktsPerSequence;
    data.RSSI(k) = result.RSSI;
    data.ErrCount = (data.Npkts - sum(data.ReceivedFragmentCount));
    data.PER =  data.ErrCount / data.Npkts;

    if data.ErrCount > 100,
        break;
    end

    if ishandle(hBar)
        waitbar(data.Npkts/data.MaxPkts,hBar,sprintf('PER = %d / %g = %1.4f%%',data.ErrCount,data.Npkts,100*data.PER));
    else
        fprintf('*** TEST ABORTED ***\n');
        break;
    end
end

DwPhyLab_DisableESG;
data.Duration_s = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
