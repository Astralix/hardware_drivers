function DwPhyTest_LoadAWG(TestID, Vpp, RunContinuous, BW)
% DwPhyLab_LoadAWG(TestID)

if nargin<2, Vpp = []; end
if nargin<3, RunContinuous = []; end
if nargin<4, BW = []; end

if isempty(Vpp), Vpp = 1; end
if isempty(RunContinuous), RunContinuous = 0; end
if isempty(BW), BW = 20e6; end

% Select Waveforms
switch TestID,
    case {'ACI-OFDM','ACI-CCK','DECT','54MBPS-BADFCS-T_IFS=172E-6'},
        fileI = sprintf('DwPhyTest\\%s-I.wfm',TestID);
        fileQ = sprintf('DwPhyTest\\%s-Q.wfm',TestID);
    case {'SkipWaveformLoad'}
        fileI = [];
        fileQ = [];
    otherwise,
        error('Undefined Waveform Type');
end

% Configure the Instrument
ConfigureAWG520(fileI, fileQ, Vpp, RunContinuous, BW);

%% ConfigureAWG520
function ConfigureAWG520(fileI, fileQ, Vpp, RunContinuous, BW)

AWG520 = DwPhyLab_OpenAWG520;
if ~AWG520, error('Unable to open AWG520'); end

if ~isempty(fileI)
    DwPhyLab_WriteAWG520(AWG520,sprintf('SOURCE1:FUNCTION:USER "%s","MAIN"',fileI));
end
if ~isempty(fileQ)
    DwPhyLab_WriteAWG520(AWG520,sprintf('SOURCE2:FUNCTION:USER "%s","MAIN"',fileQ));
end
DwPhyLab_WriteAWG520(AWG520,sprintf('SOURCE1:VOLTAGE:LEVEL:IMMEDIATE:AMPLITUDE %1.0fmV',1000*Vpp));
DwPhyLab_WriteAWG520(AWG520,sprintf('SOURCE2:VOLTAGE:LEVEL:IMMEDIATE:AMPLITUDE %1.0fmV',1000*Vpp));
DwPhyLab_WriteAWG520(AWG520,'SOURCE1:VOLTAGE:LEVEL:IMMEDIATE:OFFSET 0mV');
DwPhyLab_WriteAWG520(AWG520,'SOURCE2:VOLTAGE:LEVEL:IMMEDIATE:OFFSET 0mV');
DwPhyLab_WriteAWG520(AWG520,sprintf('OUTPUT1:FILTER:LPASS:FREQUENCY %1.1e',BW));
DwPhyLab_WriteAWG520(AWG520,sprintf('OUTPUT2:FILTER:LPASS:FREQUENCY %1.1e',BW));
DwPhyLab_WriteAWG520(AWG520,'OUTPUT1:STATE ON');
DwPhyLab_WriteAWG520(AWG520,'OUTPUT2:STATE ON');
DwPhyLab_WriteAWG520(AWG520,'TRIGGER:SEQUENCE:SOURCE EXTERNAL');
DwPhyLab_WriteAWG520(AWG520,'TRIGGER:SEQUENCE:LEVEL 2.0');

if RunContinuous,
    DwPhyLab_WriteAWG520(AWG520,'AWGControl:RMODE CONT');
else
    DwPhyLab_WriteAWG520(AWG520,'AWGControl:RMODE TRIG');
end
DwPhyLab_WriteAWG520(AWG520,'AWGControl:RUN:IMMEDIATE');

% Wait for the operation to complete
ok = 0;
while(~ok)
    DwPhyLab_WriteAWG520(AWG520,'*OPC?');
    [ok, msg] = DwPhyLab_ReadAWG520(AWG520);
    if(length(msg) < 1), 
        ok = 0;
    else
        ok = (msg(1) == '1');
    end
end
DwPhyLab_CloseAWG520(AWG520);
