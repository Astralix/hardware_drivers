function data = DwPhyTest_NuisancePERvRegister(varargin)
% data = DwPhyTest_NuisancePERvRegister(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
ShortPreamble = 1;
NuisanceLengthPSDU = 250;
NuisanceMbps = 6;
Broadcast = 1;
fcMHz = 2412;
PrdBm = -50;
PktsPerWaveform = 1;
MaxPkts = 1000;
MinFail = 100;
T_IFS = 34e-6;

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

LoadNuisancePER(Mbps, LengthPSDU, Broadcast, ShortPreamble, T_IFS, ...
    NuisanceMbps, NuisanceLengthPSDU);
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

%% LoadNuisancePER
function LoadNuisancePER(Mbps, LengthPSDU, Broadcast, ShortPreamble, T_IFS, ...
    NuisanceMbps, NuisanceLengthPSDU)

% Generate the nuisance packet
[x1, m1] = DwPhyLab_PacketWaveform(NuisanceMbps, NuisanceLengthPSDU, T_IFS, 1, 1, 0, ...
    'options = ''wiMAC_SetAddress(1, "00112233445566")''');

% Generate the packet waveform
[x2, m2] = DwPhyLab_PacketWaveform(Mbps, LengthPSDU, T_IFS, 1, ShortPreamble, Broadcast);
x = [x1; x2];
m = [m1, m2];
x = x/512; % common scaling used in DwPhyLab

% Load the waveform and configure the ESG
ESG = DwPhyLab_OpenESG;
if isempty(ESG), error('Unable to open ESG'); end

DwPhyLab_LoadWaveform([], 80e6, x, m, 'no_play');

% Configure the sequence to be triggered from the LAN/GPIB using "*TRG"
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:TYPE SINGLE');
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:TRIGGER:SOURCE BUS');

% Load/create the sequence file. This assumes DWPHYLAB100 and DWPHYLAB1000
% already exist in the SEQ directory. These are common burst lengths and
% are not dynamically generated to improve execution time and reduce write
% cylces to the ESG flash memory
Repetitions = 1000;
SequenceName = 'SEQ:DWPHYLAB1000';
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:WAV "%s"',SequenceName);
DwPhyLab_SendCommand(ESG, ':SOURCE:RADIO:ARB:STATE ON');

% Calculate and set the waveform RMS level to be used for subsequent power
% search operations. Disable ALC and enable automatic power search
k = m(2,:) > 0;
rms = sqrt(mean(abs(x(k)).^2));
DwPhyLab_SendCommand(ESG,':SOURCE:RADIO:ARB:HEADER:RMS "%s", %1.6f',SequenceName,rms);
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH:REFERENCE RMS');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:SEARCH ON');
DwPhyLab_SendCommand(ESG,':SOURCE:POWER:ALC:STATE OFF');
DwPhyLab_SendQuery  (ESG,'*OPC?');
DwPhyLab_CloseESG(ESG); 

% Store the sequence duration. This is used to delay timers in subsequent
% PER measurements
DwPhyLab_Parameters('TsequencePER',      Repetitions*length(x)/80e6);
DwPhyLab_Parameters('PktsPerSequencePER',1000);
DwPhyLab_Parameters('SequenceNamePER',   SequenceName);
DwPhyLab_Parameters('SequenceRmsPER',    rms);
