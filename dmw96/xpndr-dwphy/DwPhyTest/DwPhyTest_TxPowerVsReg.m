function data = DwPhyTest_TxPowerVsReg(varargin)
%data = DwPhyTest_TxPower(...)
%   Meaures TX output power as a function of fcMHz and TXPWRLVL. 
%
%    Specify register to vary with RegAddr, RegRange, and RegField

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 1500;
fcMHz = 2412;
TXPWRLVL = 63;
PacketType = 0;
AutoRange = 0;
Average = 0;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','AutoRange','TXPWRLVL'});

if isempty(who('RegAddr')) || isempty(who('RegRange')),
    error('Must specify at least RegAddr and RegRange');
end
if isempty(who('RegField')), RegField = '7:0'; end
if isnumeric(RegField), RegField = sprintf('%d:%d',RegField,RegField); end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0,'Configuring Spectrum Analyzer...', 'Name',mfilename, 'WindowStyle','modal');
PSA = DwPhyLab_OpenPSA;

% save the original timeout value and increase to cover the expected test time so the
% *OPC? query does not cause an error message
Timeout = DwPhyLab_GetVisaTimeout(PSA);
DwPhyLab_SetVisaTimeout(PSA, 10000);

DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
DwPhyLab_SendCommand(PSA,':CONF:SAN'); % Measure Off
DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON; *WAI'); % continuous sweep
DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO:RULES NORMAL');

DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:EAMEAS YES');
DwPhyLab_SendCommand(PSA,':CONFIGURE:CHPOWER');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:BANDWIDTH:INTEGRATION 20.0 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:FREQ:SPAN 40 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:AVERAGE:STATE OFF');

if AutoRange,
    DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, TXPWRLVL, PacketType)
    DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON; *WAI'); % continuous sweep
    DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:RANGE:AUTO ONCE; *WAI');
else
    Atten = round( 30 - min(data.Ploss) );
    RefLevel = 23;
    DwPhyLab_SendCommand(PSA,':DISPLAY:WINDOW:TRACE:Y:SCALE:RLEVEL %d dBm',RefLevel);
    DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:ATTENUATION %d',Atten);
end

DwPhyLab_TxBurst(0); % stop any on-going burst transmissions
DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO:RULES ACCURACY; *WAI');
pause(0.5); 

Npts = length(RegRange) * length(fcMHz);
if Average <= 0,     Mpts = 1;
elseif Average == 1, Mpts = 4;
else                 Mpts = round(Average);
end

for k = 1:length(fcMHz),

    DwPhyLab_SendCommand(PSA, ':SENSE:FREQUENCY:CENTER %5.3fMHz',fcMHz(k));
    DwPhyLab_TxBurst(0); % stop any on-going burst transmissions
    DwPhyLab_SetChannelFreq(fcMHz(k));
    if ~isempty(who('UserReg'))
        DwPhyLab_WriteUserReg(UserReg);
    end
    DwPhyLab_Wake;

    for i = 1:length(RegRange),

        DwPhyLab_WriteRegister(RegAddr, RegField, RegRange(i));

        DwPhyLab_TxBurst(128, 54, 28, TXPWRLVL, PacketType); pause(0.01); % ALC tuning
        DwPhyLab_TxBurst(1e6, Mbps, LengthPSDU, TXPWRLVL, PacketType); % measurement pkts
        data.ReadPCOUNT(k,i)   = DwPhyLab_ReadRegister(256+77,'7:1');
        data.ReadTXPWRLVL(k,i) = DwPhyLab_ReadRegister(11);
        
        for j=1:Mpts,
            DwPhyLab_SendCommand(PSA,':INIT:CHPOWER');  
            DwPhyLab_SendQuery  (PSA,'*OPC?'); pause(0.1);
            result = DwPhyLab_SendQuery(PSA,':FETCH:CHPOWER:CHPOWER?');
            data.p(k,i,j) = str2double(result) + data.Ploss(k);
        end
        data.Pout_dBm(k,i) = 10*log10( mean(10.^(data.p(k,i,:)/10)) );
            
        if max(diff(TXPWRLVL))==0, data.t(i) = now; end % timestamp

        if ishandle(hBar)
            waitbar( (i+(k-1)*length(RegRange))/Npts, hBar, ...
                sprintf('%d MHz, Reg(0x%s) = %d (0x%s), Pout = %1.1f dBm', fcMHz(k), ...
                dec2hex(RegAddr,3), RegRange(i), dec2hex(RegRange(i),2), data.Pout_dBm(k,i)) );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end

    end
    if ~ishandle(hBar), break; end
end

DwPhyLab_TxBurst(0); % stop any on-going burst transmissions
DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE ON');
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

