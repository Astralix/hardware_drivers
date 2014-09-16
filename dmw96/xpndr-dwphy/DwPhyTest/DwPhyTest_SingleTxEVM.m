function data = DwPhyTest_SingleTxEVM(varargin)
% data = DwPhyTest_SingleTxEVM(...)
%    This is similar to DwPhyTest_TxEVM but packets are transmitted one at a time
%    instead of in burst mode for the EVM measurement. Burst mode is still used for
%    scope amplitude adjustment.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 100;
fcMHz = 2412;
PacketType = 0;
Npts = 20;
ScopeSetup = 1;
ScopeAmplitude = 1;
hBar = [];
UseWaitBar = 1;
hBarClose = 1;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps','LengthPSDU','fcMHz','PacketType','Npts','ScopeSetup','TXPWRLVL'});

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 32;
end

%% Get Waitbar Data or Create a New One
if UseWaitBar

    hBarOfs = 0;
    hBarLen = 1;
    hBarStr = {};

    if isempty(hBar),
        hBar = waitbar(0.0,{sprintf('Mbps = %d, LengthPSDU = %d, TXPWRLVL = %d',...
            Mbps,LengthPSDU,TXPWRLVL),'Measuring EVM...'}, ...
            'Name',mfilename,'WindowStyle','modal');
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
end
DwPhyTest_ConfigSpectrumAnalyzer('EVM');

% Send packets for ALC and adjust scope amplitude if requested
DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, TXPWRLVL, PacketType)
pause(0.1);
if ScopeAmplitude,
    [data.VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude;
end
DwPhyLab_TxBurst(0);

% Adjust scope waveform length
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'AUTO_CALIBRATE OFF');
DwPhyLab_SendCommand(scope, 'TRIG_MODE AUTO;');

Tpacket = 20e-6 + 4e-6*ceil(8*LengthPSDU/Mbps/4);
if     Tpacket < 150e-6,
    DwPhyLab_SendCommand(scope, 'TIME_DIV 20E-6'); T = 20e-6;
    DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 250K');
elseif Tpacket < 400e-6,
    DwPhyLab_SendCommand(scope, 'TIME_DIV 50E-6'); T = 50e-6;
    DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 500K');
elseif Tpacket < 850e-6,
    DwPhyLab_SendCommand(scope, 'TIME_DIV 100E-6'); T = 100e-6;
    DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 1MA');
elseif Tpacket < 1750e-6,
    DwPhyLab_SendCommand(scope, 'TIME_DIV 200E-6'); T = 200e-6;
    DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 2.5MA');
else
    DwPhyLab_SendCommand(scope, 'TIME_DIV 500E-6'); T = 500e-6;
    DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 5MA');
end

% Adjust trigger position for WavePro 7200 (cannot use % like the 960)
if DwPhyLab_Parameters('ScopeIsWavePro7200')
    DwPhyLab_SendCommand(scope, sprintf('TRIG_DELAY %de-6',-4*T*1e6));
end

pause(1.0);
DwPhyLab_SendQuery  (scope, '*OPC?');
DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM;');
DwPhyLab_SendQuery  (scope, '*OPC?');
DwPhyLab_SendCommand(scope, 'AUTO_CALIBRATE ON');
DwPhyLab_CloseScope(scope);

% Ad-hoc: We seem to need to send a packet ahead of the one to be used.
DwPhyLab_TxPacket(Mbps, LengthPSDU, TXPWRLVL, PacketType);
pause(0.1);

if UseWaitBar && ~ishandle(hBar),
    fprintf('*** TEST ABORTED ***\n');
    data.Result = -1;
    return;
end

BadData = 0;
while length(data.EVMdB) < Npts && ~BadData,

    DwPhyLab_TxPacket(Mbps, LengthPSDU, TXPWRLVL, PacketType);
    pause(0.1);
    [result, z, Fs] = DwPhyLab_GetEVM;
    
    data.EVMdB   = [data.EVMdB result.EVMdB];
    data.W_EVMdB = [data.W_EVMdB result.W_EVMdB];
    data.AvgEVMdB   = 10*log10( mean(10.^(data.EVMdB/10)) );
    BadData = any(isnan(data.EVMdB)) || isempty(result.EVMdB);

    if exist('Concise','var') && ~Concise,
        data.LastResult = result;
        if ScopeAmplitude
            data.AdjustScopeAmplitude = tracelog;
        end
    end
    if ~isempty(who('VaryDelayCFO')) && VaryDelayCFO == 1,
        
        wiseMex('wiParse_Line()','CalcEVM_OFDM.DelayCFO = 0');
        result = DwPhyLab_GetEVM(z,Fs);
        data.EVMdB0 = [data.EVMdB20 result.EVMdB];
        data.AvgEVMdB0 = 10*log10( mean(10.^(data.EVMdB0/10)) );
        
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
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar) && hBarClose, close(hBar); end;

%% REVISIONS
% 2010-02-22 [BB] Turn off autocalibration and check OPC when changing acquisition mode.
% 2010-08-04 Use DwPhyLab functions to control the scope
% 2010-11-30 [SM] Bug fix for accessing tracelog when Concise=0. 