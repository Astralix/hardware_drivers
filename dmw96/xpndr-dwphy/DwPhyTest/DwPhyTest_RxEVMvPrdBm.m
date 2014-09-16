function data = DwPhyTest_RxEVMvPrdBm(varargin)
%data = DwPhyTest_RxEVMvPrdBm(...)
%   Measure RXA EVM as a function of Pr[dBm].

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 100;
fcMHz = 2484;
PacketType = 0;
Npts = 20;
PrdBm = -82:2:-62;

DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_RxCableLoss(fcMHz);
if ~exist('Concise','var'), Concise = 1; end

hBar = waitbar(0.0,{'',''}, 'Name',mfilename, 'WindowStyle','modal');

for i=1:length(PrdBm),
    
    if ishandle(hBar)
        set(hBar,'UserData',[(i-1) 1]/length(PrdBm));
    else
        fprintf('*** TEST ABORTED ***\n'); 
        data.Result = -1;
        break;
    end

    argList{1} = sprintf('Npts = %d',Npts);
    argList{2} = sprintf('TXPWRLVL = %d',data.TXPWRLVL(i));
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
