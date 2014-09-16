function data = DwPhyTest_TxEVM(varargin)
% data = DwPhyTest_TxEVM(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 100;
fcMHz = 2412;
PacketType = 0;
Npts = 20;
ScopeSetup = 1;
hBar = [];
UseWaitBar = 1;
hBarClose = 1;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','Npts','ScopeSetup','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 59;
end

%% Get Waitbar Data or Create a New One
if UseWaitBar

    hBarOfs = 0;
    hBarLen = 1;
    hBarStr = {};

    if isempty(hBar),
        hBar = waitbar(0.0,'Measuring EVM...','Name',mfilename,'WindowStyle','modal');
    else
        if ishandle(hBar),
            hBarClose = 0;
            UserData = get(hBar,'UserData');
            if length(UserData) == 2,
                hBarOfs = UserData(1);
                hBarLen = UserData(2);
            end
            hBarStr = get(get(get(hBar,'Children'),'Title'),'String');
            if ischar(hBarStr)
                clear hBarStr;
                hBarStr{1} = get(get(get(hBar,'Children'),'Title'),'String');
            end
        end
    end
    hBarStrIndex = length(hBarStr) + 1;
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
data.Leakage = [];
data.Flatness = [];
data.EVMdB = [];
data.W_EVMdB = [];
data.AvgEVMdB = NaN;

if ~isempty(who('VaryDelayCFO')) && VaryDelayCFO == 1,
    data.EVMdB20 = [];
    data.EVMdB40 = [];
    data.EVMdB63 = [];
end

if ScopeSetup,
    if isempty(who('VppScope')), VppScope = 1.0; end
    DwPhyTest_ScopeSetup(1, VppScope, VppScope/8, 'AUTO');
    pause(0.5); % [100219BB]
end
DwPhyTest_ConfigSpectrumAnalyzer('EVM');

DwPhyLab_TxBurst(0); pause(0.2);
DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, TXPWRLVL, PacketType); pause(0.1);
[data.VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude;

if UseWaitBar && ~ishandle(hBar),
    fprintf('*** TEST ABORTED ***\n');
    data.Result = -1;
    return;
end

% Change triggering to normal and wait for changes
scope = DwPhyLab_OpenScope;
DwPhyLab_SendQuery  (scope,'*OPC?');
DwPhyLab_SendCommand(scope,'TRIG_MODE NORM;');
DwPhyLab_SendQuery  (scope,'*OPC?');
DwPhyLab_CloseScope (scope);

BadData = 0;
while length(data.EVMdB) < Npts && ~BadData,

    scope = DwPhyLab_OpenScope;
    DwPhyLab_SendCommand(scope,'ARM; WAIT 5.0');
    DwPhyLab_SendQuery  (scope,'*OPC?');
    DwPhyLab_CloseScope (scope);

    [result, z, Fs] = DwPhyLab_GetEVM;

    data.Leakage  = [data.Leakage result.Leakage];
    data.Flatness = [data.Flatness result.Flatness];
    data.EVMdB   = [data.EVMdB result.EVMdB];
    data.W_EVMdB = [data.W_EVMdB result.W_EVMdB];
    data.AvgEVMdB   = 10*log10( mean(10.^(data.EVMdB/10)) );
    BadData = any(isnan(data.EVMdB)) || isempty(result.EVMdB);

    if ~isempty(who('Concise')) && ~Concise,
        data.LastResult = result;
        data.AdjustScopeAmplitude = tracelog;
    end
    if ~isempty(who('VaryDelayCFO')) && VaryDelayCFO == 1,
        
        wiseMex('wiParse_Line()','CalcEVM_OFDM.DelayCFO = 20');
        result = DwPhyLab_GetEVM(z,Fs);
        data.EVMdB20 = [data.EVMdB20 result.EVMdB];
        data.AvgEVMdB20 = 10*log10( mean(10.^(data.EVMdB20/10)) );
        
        wiseMex('wiParse_Line()','CalcEVM_OFDM.DelayCFO = 40');
        result = DwPhyLab_GetEVM(z,Fs);
        data.EVMdB40 = [data.EVMdB40 result.EVMdB];
        data.AvgEVMdB40 = 10*log10( mean(10.^(data.EVMdB40/10)) );

        wiseMex('wiParse_Line()','CalcEVM_OFDM.DelayCFO = 63');
        result = DwPhyLab_GetEVM(z,Fs);
        data.EVMdB63 = [data.EVMdB63 result.EVMdB];
        data.AvgEVMdB63 = 10*log10( mean(10.^(data.EVMdB63/10)) );
        
    end
    
    if UseWaitBar,
        if ishandle(hBar)
            w = hBarOfs + hBarLen * min(1,(length(data.EVMdB)/Npts));
            hBarStr{hBarStrIndex} = sprintf('EVM = %1.1f dB',data.AvgEVMdB);
            waitbar(w,hBar,hBarStr);
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
end
DwPhyLab_TxBurst(0); 
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar) && hBarClose, close(hBar); end;

%% REVISIONS
% 2010-03-05 Disable TX burst after ScopeSetup
%            Modify wait-for-trigger change to use *OPC?
%            ARM the scope trigger at the start of each "while" loop
% 2010-08-04 Use DwPhyLab functions to control the scope