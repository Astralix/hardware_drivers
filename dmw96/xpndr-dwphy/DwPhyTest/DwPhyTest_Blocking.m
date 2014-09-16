function data = DwPhyTest_Blocking(varargin)
% data = DwPhyTest_Blocking(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
fcMHz = 2412;
fiMHz = 1:6000;
PrdBm = -62;
PidBmMax =   0;
PidBmMin = -50;
PktsPerWaveform = 10;
MaxPkts = 1000;
MinFail = 100;
SensitivityLevel = 0.1;
PidBmStep = 1;
Filter = [];

DwPhyLab_EvaluateVarargin(varargin);

Ploss  = DwPhyLab_RxCableLoss(fcMHz);
Piloss = DwPhyLab_RxCableLoss(fiMHz, Filter);
PktsPerSequence = max( 1000, 10*PktsPerWaveform);

%% Run Test
DwPhyTest_RunTestCommon;
hBar = waitbar(0.0,'Running...','Name',mfilename,'WindowStyle','modal');

DwPhyLab_LoadPER(Mbps, LengthPSDU, PktsPerSequence, PktsPerWaveform);
DwPhyLab_SetRxPower(PrdBm + Ploss);
DwPhyLab_SetRxPower(2, -136);
DwPhyLab_SetRxFreqESG(2, fiMHz(1));

data.Timestamp = datestr(now);

ESG2 = DwPhyLab_OpenESG(2);
DwPhyLab_SendCommand(ESG2,':SOURCE:DM:STATE OFF');
DwPhyLab_SendCommand(ESG2,':OUTPUT:MODULATION:STATE OFF');
DwPhyLab_SendCommand(ESG2,':OUTPUT:STATE ON');
DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:STATE ON');

PidBm = PidBmMax;

for i = 1:length(fiMHz),
    
    PidBm = min(PidBmMax, PidBm + PidBmStep);
    DwPhyLab_SendCommand(ESG2, ':SOURCE:FREQUENCY:CW %5.9fGHz',fiMHz(i)/1000);
    DwPhyLab_SendCommand(ESG2, ':SOURCE:POWER:LEVEL:IMMEDIATE:AMPLITUDE %1.1fdBm',PidBm+Piloss(i));
    pause(0.02); % wait for gain/frequency settling

    PER = GetPER(MinFail, MaxPkts, PktsPerSequence);
    if PER < SensitivityLevel,
        while (PER < SensitivityLevel) && (PidBm <= PidBmMax) && ishandle(hBar),
            PER1 = PER; PidBm1 = PidBm;
            PidBm = PidBm + PidBmStep;
            DwPhyLab_SendCommand(ESG2, ':SOURCE:POWER:LEVEL:IMMEDIATE:AMPLITUDE %1.1fdBm',PidBm+Piloss(i));
            pause(0.02);
            PER = GetPER(MinFail, MaxPkts, PktsPerSequence);
        end
        PER = PER1; PidBm = PidBm1;
    else
        while (PER > SensitivityLevel) && (PidBm > PidBmMin) && ishandle(hBar),
            PidBm = PidBm - PidBmStep;
            DwPhyLab_SendCommand(ESG2, ':SOURCE:POWER:LEVEL:IMMEDIATE:AMPLITUDE %1.1fdBm',PidBm+Piloss(i));
            PER = GetPER(MinFail, MaxPkts, PktsPerSequence);
        end
    end
    data.PER(i) = PER;
    data.PidBm(i) = PidBm;
    
    if ishandle(hBar)
        x = i/length(fiMHz);
        waitbar(x,hBar,sprintf('fi = %1.3f MHz, Pi = %1.1f dBm, PER = %1.2f%%',fiMHz(i),PidBm,100*data.PER(i)));
    else
        fprintf('*** TEST ABORTED ***\n');
        break;
    end
end

DwPhyLab_CloseESG(ESG2);
DwPhyLab_DisableESG;
DwPhyLab_SetRxPower(2,-136);
data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% GetPER
function PER = GetPER(MinFail, MaxPkts, PktsPerSequence)
ReceivedFragmentCount = 0;
ErrCount = 0;
Npkts = 0;
while (ErrCount < MinFail) && (Npkts < MaxPkts)
    result = DwPhyLab_GetPER;
    ReceivedFragmentCount = ReceivedFragmentCount + result.ReceivedFragmentCount;
    Npkts = Npkts + PktsPerSequence;
    ErrCount = Npkts - ReceivedFragmentCount;
end
PER = ErrCount / Npkts;
