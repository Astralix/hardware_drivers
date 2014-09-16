function DwPhyTest_ConfigSpectrumAnalyzer(Mode, Standard)

if nargin < 1, Mode = []; end
if nargin < 2, Standard = []; end

if isempty(Mode),     Mode = 'EVM'; end
if isempty(Standard), Standard = '802.11a'; end

switch upper(Mode),

    case {'SA','SPECTRUM','SPECTRUMANALYZER','DEFAULT'}
        fcMHz = DwPhyLab_Parameters('fcMHz');
        if isempty(fcMHz), fcMHz = 2412; end;
        PSA = DwPhyLab_OpenPSA;
        DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
        DwPhyLab_SendCommand(PSA,':CONF:SAN');
        DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS OFF');
        DwPhyLab_SendQuery  (PSA,'*OPC?'); pause(1); % make sure the PSA is configured
        DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 100.0e6'); 
        DwPhyLab_SendCommand(PSA, ':SENSE:FREQUENCY:CENTER %5.3fGHz',fcMHz/1000);
        DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO ON');
        DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:RESOLUTION 100 kHz');
        DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:VIDEO 30 kHz');
        DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
        DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:ATTENUATION:AUTO ON');
        DwPhyLab_SendCommand(PSA,':DISPLAY:WINDOW:TRACE:Y:RLEVEL 30 dBm');
        DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
        DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON'); % continuous sweep
        DwPhyLab_SendCommand(PSA,':INIT:IMMEDIATE; *WAI');
        DwPhyLab_ClosePSA(PSA);

    case {'EVM','ZEROSPAN','ZERO-SPAN','70MHZIF','70MHZ IF'},
        Attn_dB = 10;%20;
        PSA = DwPhyLab_OpenPSA;
        DwPhyLab_SendCommand(PSA,':CONF:SAN');
        DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
        DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:SPAN 0 Hz'); 
        DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME 1 ms');
        DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON'); % continuous sweep
        DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:RESOLUTION 30 kHz');
        DwPhyLab_SendCommand(PSA,':SENSE:BANDWIDTH:VIDEO 30 kHz');
        DwPhyLab_SendCommand(PSA,':SENSE:AVERAGE:STATE OFF');
        DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:ATTENUATION %d dB',Attn_dB);
        DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
        DwPhyLab_ClosePSA(PSA);

    case {'MASK','TXMASK','TXSPECTRUMMASK','TXSPECTRUM'}
        PSA = DwPhyLab_OpenPSA;
        DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
        DwPhyLab_SendCommand(PSA,':CONFIGURE:SEMASK');
        DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
        
        switch upper(Standard),
            case '802.11A', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11A');
            case '802.11B', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11B');
            case '802.11G', DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:SELECT WL802DOT11G');
            otherwise, error('Unknown Standard "%s"',Standard);
        end
        DwPhyLab_ClosePSA(PSA);
        
    case {'PHASENOISE'}
        fcMHz = DwPhyLab_Parameters('fcMHz');
        PSA = DwPhyLab_OpenPSA;
        DwPhyLab_SendCommand(PSA,':INST PNOISE'); % mode = Phase Noise
        DwPhyLab_SendCommand(PSA,':SENSE:FREQUENCY:CARRIER %1.9e',fcMHz*1e6);
        DwPhyLab_SendCommand(PSA,':SENSE:LPLOT:AVERAGE:COUNT 4');
        DwPhyLab_SendCommand(PSA,':SENSE:LPLOT:AVERAGE:STATE ON');
        DwPhyLab_SendCommand(PSA,':SENSE:LPLOT:FREQUENCY:OFFSET:START 1.0e3');
        DwPhyLab_SendCommand(PSA,':SENSE:LPLOT:FREQUENCY:OFFSET:STOP 10.0e6');
        DwPhyLab_ClosePSA(PSA);
       
    otherwise,
        error('Unrecognized mode "%s"',Mode);
end
