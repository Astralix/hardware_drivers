function data = DwPhyTest_TxEVMvPout(varargin)
%data = DwPhyTest_TxEVMvPout(...)
%   Measure TX EVM as a function of TXPWRLVL. If MeasurePower = 1, also measure
%   output power.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6; %#ok<NASGU>
LengthPSDU = 100; %#ok<NASGU>
fcMHz = 2412;
PacketType = 0; %#ok<NASGU>
Npts = 20;
MeasurePower = 0;
UserReg = {};

DwPhyLab_EvaluateVarargin(varargin);

if isempty(who('TXPWRLVL'))
    TXPWRLVL = DwPhyLab_Parameters('TXPWRLVL');
end
if isempty(TXPWRLVL),
    TXPWRLVL = 32;
end

if ~isempty(UserReg) && ~iscell(UserReg{1})
    UserReg = {UserReg};
end

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);
if ~exist('Concise','var'), Concise = 1; end

hBar = waitbar(0.0,{'',''}, 'Name',mfilename, 'WindowStyle','modal');

for i=1:length(TXPWRLVL),
    
    if MeasurePower,
        result = DwPhyTest_TxPower(data, sprintf('TXPWRLVL = %d; UseExistingConfig = 1;',data.TXPWRLVL(i)));
        data.Pout_dBm(i) = result.Pout_dBm;
        Attn = max(0, 3 + ceil(result.Pout_dBm - result.Ploss));
        DwPhyLab_SendCommand('PSA',':SENSE:POWER:RF:ATTENUATION %d dB',Attn);
    else
        data.Pout_dBm(i) = NaN;
    end
    
    if ishandle(hBar)
        set(hBar,'UserData',[(i-1) 1]/length(TXPWRLVL));
    else
        fprintf('*** TEST ABORTED ***\n'); 
        data.Result = -1;
        break;
    end

    argList = cell(1,4);
    argList{1} = sprintf('Npts = %d',Npts);
    argList{2} = sprintf('TXPWRLVL = %d; UseExistingConfig = 1;',data.TXPWRLVL(i));
    argList{3} = sprintf('ScopeSetup = %d',i==1);
    argList{4} = sprintf('Concise = %d',Concise);
    result = DwPhyTest_TxEVM(data, hBar, argList);
    data.lastresult = result;
    if ~Concise, data.result{i} = result; end

    data.EVMdB{i} = result.EVMdB;
    data.AvgEVMdB(i) = result.AvgEVMdB;
    data.Flatness(i) = all(result.Flatness == 1);
    data.Leakage(i)  = all(result.Leakage == 1);

    if exist('Verbose','var') && Verbose,
        fprintf('   TXPWRLVL = %2u, Pout = %5.1f dBm, AvgEVM = %5.1f dBm\n', ...
            data.TXPWRLVL(i), data.Pout_dBm(i), data.AvgEVMdB(i) );
    end
    
    if ishandle(hBar) && (result.Result ~= -1)
        waitbar( i/length(TXPWRLVL), hBar, ...
            sprintf('TXPWRLVL = %d, Pout = %1.1f dBm, EVM = %1.1f dB', ...
            data.TXPWRLVL(i),data.Pout_dBm(i),data.AvgEVMdB(i)) );
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% FormatUserRegStr
function UserRegStr = FormatUserRegStr(UserReg)
UserRegStr = sprintf('UserReg = {');
for i=1:length(UserReg),
    X = UserReg{i};
    if length(X) == 2,
        UserRegStr = sprintf('%s{%d,%d},',UserRegStr,X{1},X{2});
    elseif ischar(X{2}),
        UserRegStr = sprintf('%s{%d,''%s'',%d},',UserRegStr,X{1},X{2},X{3});
    else
        UserRegStr = sprintf('%s{%d,%d,%d},',UserRegStr,X{1},X{2},X{3});
    end
end
UserRegStr = sprintf('%s}',UserRegStr);

%% REVISIONS
% 2011-01-02 [SM]: Added FormatUserRegStr() function similar to DwPhyTest_TxEVMvRegister.m to support UserReg usage.
% 2011-03-15 [SM]: Utilize 'UseExistingConfig=1' to skip re-configuration when DwPhyTest_RunTestCommon is called again within 
%                  DwPhyTest_TxPower and DwPhyTest_TxEVM. 

