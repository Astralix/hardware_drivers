function data = DwPhyTest_InterferenceRejection(varargin)
% data = DwPhyTest_InterferenceRejection(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, LengthPSDU, fcMHz, PktsPerWaveform,
%    MaxPkts, and MinFail.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 54;
LengthPSDU = 1000;
fcMHz = 2412;
PrdBm = -62;
PktsPerWaveform = 1;
FrameBodyType = 0;
MaxPkts = 2000;
MinFail = 100;
MinPER = 0.05;
InterferenceType = 'ACI-OFDM';
UseESG2DualArb = DwPhyLab_Parameters('UseESG2DualArb');
SkipWaveformLoad = 0;

DwPhyLab_EvaluateVarargin(varargin);
switch upper(InterferenceType),
    case 'ACI-OFDM',
        Mbps = 54;
        PrdBm = -62;
        fiMHz = fcMHz + [-40 -20 20 40];
        PidBm = -35:-1:-80;
        PktsPerWaveform = 10;
        AWG520_FilterMHz = 20; 
        ESG_FilterMHz = '40E6';
    case 'ACI-CCK',
        Mbps = 11;
        LengthPSDU = 1024;
        PrdBm = -70;
        fiMHz = fcMHz + [-25 25];
        PidBm = -25:-1:-80;
        PktsPerWaveform = 10;
        FrameBodyType = 1;
        AWG520_FilterMHz = 20; 
        ESG_FilterMHz = '40E6';
    case 'DECT'
        Mbps = 54;
        PrdBm = -62;
        fiMHz = [ 1787.616 1789.344 1791.072 ...
                  1897.344 - 1.728*(0:9) ...
                  1928.448 - 1.728*(0:9) ];
        PidBm = 0:-1:-50;
        AWG520_FilterMHz = 10; 
        ESG_FilterMHz = '2.1E6';
    case '54MBPS-BADFCS-T_IFS=172E-6', 
        Mbps = 54;
        PrdBm = -62;
        fiMHz = fcMHz;
        PidBm = -80;
    otherwise,
        AWG520_FilterMHz = 20; 
        ESG_FilterMHz = '40e6';
end
DwPhyLab_EvaluateVarargin(varargin); % repeat to allow override of default parameters

if isempty(who('SensitivityLevel'))
    if any(Mbps == [1 2 5.5 11])
        SensitivityLevel = 0.08;
    else
        SensitivityLevel = 0.1;
    end
end

PktsPerSequence = max(1000, 10*PktsPerWaveform);
Ploss  = DwPhyLab_RxCableLoss(fcMHz);
Piloss = DwPhyLab_RxCableLoss(fiMHz);

%% Run Test
DwPhyTest_RunTestCommon;

if UseESG2DualArb,
    wfm = DwPhyTest_BuildInterferenceWaveform(InterferenceType);
    if isempty(wfm.m)
       wfm.m = zeros(size(wfm.y));
    end
    if wfm.FsMHz == 80,
        % Increase the sample rate to 100 MHz and apply a LPF. The ESG has a filter
        % at 40 MHz. With 80 MS/s, off-channel stuff from the interference may affect
        % the result. Not needed with the AWG520, because we use its internal filter.
        wfm.y = resample(wfm.y, 100, 80);
        wfm.m = resample(wfm.m, 100, 80);
        N = length(wfm.y); N = N - mod(N,2);
        wfm.y = wfm.y(1:N); wfm.m = wfm.m(1:N);
        wfm.FsMHz = 100;
        [B,A] = butter(3,2*20/wfm.FsMHz);
        wfm.y = filter(B,A,wfm.y);
    end
    pk = max( max(abs(real(wfm.y))), max(abs(imag(wfm.y))) );
    DwPhyLab_LoadWaveform(2, wfm.FsMHz*1e6, wfm.y/pk, wfm.m, 'play');
    ESG2 = DwPhyLab_OpenESG(2);
    DwPhyLab_SendCommand(ESG2,':OUTPUT:MODULATION:STATE ON');
    DwPhyLab_SendCommand(ESG2,':OUTPUT:STATE ON');
    DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:SEARCH:REFERENCE MOD');
    DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:STATE ON');
    DwPhyLab_SendCommand(ESG2,':SOURCE:RAD:ARB:MDES:PULS NONE');
    DwPhyLab_SendCommand(ESG2,':SOURCE:RADIO:ARB:IQ:MODULATION:FILTER %s',ESG_FilterMHz);
    DwPhyLab_CloseESG(ESG2);
else
    % Load the interference waveform into the AWG520 and configure the second ESG to use
    % this signal. Assume the input waveform has a high duty cycle: enable ALC.
    if ~SkipWaveformLoad,
        DwPhyTest_LoadAWG(InterferenceType, 1, 1, AWG520_FilterMHz*1e6);
    else
        DwPhyTest_LoadAWG('SkipWaveformLoad', 1, 1, AWG520_FilterMHz*1e6);
    end
    ESG2 = DwPhyLab_OpenESG(2);
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:SOURCE EXT');
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:STATE ON');
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:MODULATION:FILTER %s',ESG_FilterMHz);
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:EXTERNAL:SOURCE BBG1');
    DwPhyLab_SendCommand(ESG2,':SOURCE:DM:IQAD:EXT:IQAT 6.0');
    DwPhyLab_SendCommand(ESG2,':OUTPUT:MODULATION:STATE ON');
    DwPhyLab_SendCommand(ESG2,':OUTPUT:STATE ON');
    DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:SEARCH:REFERENCE MOD');
    DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:STATE ON');
    DwPhyLab_CloseESG(ESG2);
end

if isempty(who('Broadcast')), Broadcast = 1; end
if isempty(who('ShortPreamble')), ShortPreamble = 1; end;
if isempty(who('T_IFS')), T_IFS = []; end;
if ~SkipWaveformLoad
    FrameBodyTypeS = sprintf('FrameBodyType = %d',FrameBodyType);
    DwPhyLab_LoadPER(Mbps, LengthPSDU, PktsPerSequence, PktsPerWaveform, Broadcast, ...
        ShortPreamble, T_IFS, FrameBodyTypeS);
end
DwPhyLab_SetRxPower(PrdBm + Ploss);

Z = zeros(length(fiMHz), length(PidBm));
data.ReceivedFragmentCount = Z;
data.Npkts = Z;
data.ErrCount = Z;
data.PER = Z;
data.RSSI = Z;

hBar = waitbar(0.0,'Running...','Name',mfilename,'WindowStyle','modal');

for i = 1:length(fiMHz),
    DwPhyLab_SetRxFreqESG(2, fiMHz(i));

    for k = 1:length(PidBm),
        DwPhyLab_SetRxPower(2, PidBm(k)+Piloss(i));
        pause(0.1); % wait for amplitude to settle

        data.Npkts(i,k) = 0;
        data.ErrCount(i,k) = 0;
        data.ReceivedFragmentCount(i,k) = 0;

        while (data.Npkts(i,k) < MaxPkts) && (data.ErrCount(i,k) < MinFail),

            result = DwPhyLab_GetPER;
            data.ReceivedFragmentCount(i,k) = data.ReceivedFragmentCount(i,k) + result.ReceivedFragmentCount;
            data.Npkts(i,k) = data.Npkts(i,k) + PktsPerSequence;
            data.ErrCount(i,k) = data.Npkts(i,k) - data.ReceivedFragmentCount(i,k);
            data.PER(i,k) = data.ErrCount(i,k) / data.Npkts(i,k);
            data.RSSI(i,k) = result.RSSI;

            if ishandle(hBar)
                x = (k + (i-1)*length(PidBm)) / (length(fiMHz)*length(PidBm));
                waitbar(x, hBar, sprintf('fi = %1.3f MHz, Pi = %1.1f dBm, PER = %1.2f%%',fiMHz(i),PidBm(k),100*data.PER(i,k)));
            else
                fprintf('*** TEST ABORTED ***\n');
                DwPhyLab_DisableESG;
                DwPhyLab_SetRxPower(2,-136);
                data.Result = -1;
                return ;
            end
        end

       if data.PER(i,k) <= MinPER,
            break;
       end
            
    end
    % compute rejection
    data.PidBm0(i) = DwPhyTest_CalcSensitivity(PidBm, data.PER(i,:), SensitivityLevel);
    data.Rejection_dB(i) = data.PidBm0(i) - PrdBm;
    
    % pass/fail
    dfMHz = round( abs(fiMHz(i) - fcMHz) );
    switch upper(InterferenceType),
        case 'ACI-OFDM',
            if dfMHz >= 40,
                if data.PidBm0(i) < -47, data.Result = 0; end
            elseif dfMHz >= 25,
                if data.PidBm0(i) < -63, data.Result = 0; end
            elseif (dfMHz >= 20) && (fcMHz > 3000),
                if data.PidBm0(i) < -63, data.Result = 0; end
            end
        case 'ACI-CCK',
            if dfMHz >= 25,
                if data.PidBm0(i) < -35, data.Result = 0; end
            end
    end
end

DwPhyLab_DisableESG;
DwPhyLab_SetRxPower(2,-136);

%%% Disable ARB to avoid interference with subsequent tests
ESG2 = DwPhyLab_OpenESG(2);
DwPhyLab_SendCommand(ESG2,':SOURCE:POWER:ALC:STATE OFF');
DwPhyLab_SendCommand(ESG2,':OUTPUT:MODULATION:STATE OFF');
if UseESG2DualArb,
    DwPhyLab_SendCommand(ESG2,':SOURCE:RADIO:ARB:STATE OFF');
else
    AWG520 = DwPhyLab_OpenAWG520;
    DwPhyLab_WriteAWG520(AWG520,'AWGCONTROL:STOP');
    DwPhyLab_WriteAWG520(AWG520,'OUTPUT1:STATE OFF');
    DwPhyLab_WriteAWG520(AWG520,'OUTPUT2:STATE OFF');
    DwPhyLab_CloseAWG520(AWG520);
end
DwPhyLab_CloseESG(ESG2);

data.Runtime = 24*3600*(now-datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;
