function data = DwPhyTest_InterferenceDwell(varargin)
% data = DwPhyTest_InterferenceDwell(...)
%
%   ? What do I use this test for?
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
fcMHz = 2412;
fcMHzOfs = 0.0;
fiMHz = [];
PrdBm = -62;
PidBm = -50;
PktsPerWaveform = 1;
MaxPkts = 2000;
MinFail = 100;
NumPoints = 100;
RxMode = 1 + 2*(fcMHz<3000); % 3(802.11g) in 2.4 GHz band, 1(802.11a) in 5 GHz band

DwPhyLab_EvaluateVarargin(varargin);

if isempty(fiMHz),
    fiMHz = fcMHz + 40;
end

data.Mbps = Mbps;
data.LengthPSDU = LengthPSDU;
data.fcMHz  = fcMHz;
data.fcMHzOfs = fcMHzOfs;
data.fiMHz  = fiMHz;
data.RxMode = RxMode;
data.PrdBm  = PrdBm;
data.PidBm  = PidBm;
data.MaxPkts= MaxPkts;
data.MinFail= MinFail;
data.PktsPerWaveform = PktsPerWaveform;
data.PktsPerSequence = max( 1000, 10*data.PktsPerWaveform);
data.varargin = varargin;

data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
data.Piloss = DwPhyLab_RxCableLoss(fiMHz);

%% Run Test
DwPhyLab_Initialize;
DwPhyLab_SetRxFreqESG(fcMHz + fcMHzOfs);
DwPhyLab_SetChannelFreq(fcMHz);
DwPhyLab_SetRxMode(RxMode);
DwPhyLab_Wake;

data.RegList = DwPhyLab_RegList;

DwPhyTest_LoadAWG('ACI-OFDM',1,1);

DwPhyLab_LoadPER(data.Mbps, data.LengthPSDU, data.PktsPerSequence, data.PktsPerWaveform);
DwPhyLab_SetRxPower(data.PrdBm + data.Ploss);

Z = zeros(length(data.fiMHz), length(data.PidBm));
data.ReceivedFragmentCount = Z;
data.Npkts = Z;
data.ErrCount = Z;
data.PER = Z;
data.RSSI = Z;

hBar = waitbar(0.0,'Running...','Name',mfilename,'WindowStyle','modal');

DwPhyLab_SetRxFreqESG(2, fiMHz);
DwPhyLab_SetRxPower(2, PidBm+data.Piloss);

    DwPhyLab_SetChannelFreq(fcMHz);

for i=1:NumPoints,

%    DwPhyLab_SetChannelFreq(fcMHz);
%    DwPhyLab_SetRxMode(RxMode);
    pause(0.01);
    DwPhyLab_Sleep; pause(0.01);
    DwPhyLab_Wake; pause(0.01);

    data.Npkts(i) = 0;
    data.ErrCount(i) = 0;
    data.ReceivedFragmentCount(i) = 0;

    while (data.Npkts(i) < data.MaxPkts) && (data.ErrCount(i) < data.MinFail),

        result = DwPhyLab_GetPER;
        data.ReceivedFragmentCount(i) = data.ReceivedFragmentCount(i) + result.ReceivedFragmentCount;
        data.Npkts(i) = data.Npkts(i) + data.PktsPerSequence;
        data.ErrCount(i) = data.Npkts(i) - data.ReceivedFragmentCount(i);
        data.PER(i) = data.ErrCount(i) / data.Npkts(i);
        data.RSSI(i) = result.RSSI;

        if ishandle(hBar)
            waitbar(i/NumPoints,hBar,sprintf('PER = %1.2f%%',100*data.PER(i)));
        else
            fprintf('*** TEST ABORTED ***\n');
            DwPhyLab_DisableESG;
            DwPhyLab_SetRxPower(2,-136);
            return ;
        end
    end
end

DwPhyLab_DisableESG;
DwPhyLab_SetRxPower(2,-136);
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
