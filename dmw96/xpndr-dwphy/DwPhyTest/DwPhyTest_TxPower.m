function data = DwPhyTest_TxPower(varargin)
%data = DwPhyTest_TxPower(...)
%   Meaures TX output power as a function of fcMHz and TXPWRLVL. 

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 1500;
fcMHz = 2412;
PacketType = 0;
AutoRange = 0;
Average = 0;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','AutoRange','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL)
    TXPWRLVL = 0:63;
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0,'Configuring Spectrum Analyzer...', 'Name',mfilename, 'WindowStyle','modal');
MaxTXPWRLVL = max(TXPWRLVL);
PSA = DwPhyLab_OpenPSA;

% save the original timeout value and increase to cover the expected test time so the
% *OPC? query does not cause an error message
[status, description, Timeout] = DwPhyLab_GetVisaTimeout(PSA);
if status ~= 0, error(description); end
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
    DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, MaxTXPWRLVL, PacketType)
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

Npts = length(fcMHz) * length(TXPWRLVL);
if Average <= 0,     Mpts = 1;
elseif Average == 1, Mpts = 4;
else                 Mpts = round(Average);
end

for k = 1:length(fcMHz),

    DwPhyLab_SendCommand(PSA, ':SENSE:FREQUENCY:CENTER %5.3fMHz',fcMHz(k));
    DwPhyLab_TxBurst(0); pause(0.2); % stop any on-going burst transmissions
    DwPhyLab_SetChannelFreq(fcMHz(k));
    if ~isempty(who('UserReg'))
        DwPhyLab_WriteUserReg(UserReg);
    end
    DwPhyLab_Wake;
    
    % Initial ALC
    % In case updates only occur on TXPWRLVL = 63, send enough packets to allow the
    % loop to settle.
    DwPhyLab_TxBurst( 1e9, 6, 150, 63, PacketType);  pause(1);

    for i = 1:length(TXPWRLVL),

        DwPhyLab_TxBurst( 0 ); pause(0.3);
        DwPhyLab_TxBurst(1500, Mbps, LengthPSDU, TXPWRLVL(i), PacketType); 
        pause(0.2); % allow time for ALC to settle
        if ~DwPhyLab_RadioIsRF22
            data.ReadPCOUNT(k,i)   = DwPhyLab_ReadRegister(256+77,'7:1');
        else
            data.ReadPmeas(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 182,'5:0');
            data.ReadCurve(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 182,6);
            data.ReadVGA(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 222);
            data.ReadDPL(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 171)*255 + DwPhyLab_ReadRegister(32768 - 128 + 170);
            data.ReadIF(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 188);
            data.ReadLow(k,i) = DwPhyLab_ReadRegister(32768 - 128 + 186);
        end
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
            waitbar( (i+(k-1)*length(TXPWRLVL))/Npts, hBar, ...
                sprintf('%d MHz, TXPWRLVL = %d, Pout = %1.1f dBm', ...
                fcMHz(k), TXPWRLVL(i), data.Pout_dBm(k,i)) );
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

%% REVISION HISTORY
% 2008-08-06 - Added transmission of 128 short packets for fast ALC convergance
%            - Added option to average power measurements
% 2010-05-18 - Stop TxBurst before starting in for i = 1:length(TXPWRLVL) loop
% 2010-09-27 - [SM] Modified for RF22.
