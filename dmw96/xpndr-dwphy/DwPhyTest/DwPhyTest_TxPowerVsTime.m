function data = DwPhyTest_TxPowerVsTime(varargin)

T1 = 30;
T2 = 30;
fcMHz = 2412;
TXPWRLVL = 63;
NoWake = 1; %#ok<NASGU>
DisableALC = 1;

DwPhyLab_EvaluateVarargin(varargin);

T = T1+T2;

hBar = waitbar(0.0,'Measuring Relative Power...','Name',mfilename,'WindowStyle','modal');

DwPhyTest_RunTestCommon;
DwPhyLab_Sleep;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

if DisableALC,
    DwPhyLab_WriteRegister(256+65,0,1); % override PADGAIN
end

DwPhyTest_ScopeSetup
scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'COMM_HEADER SHORT');
DwPhyLab_SendCommand(scope, 'TRIG_MODE AUTO');
DwPhyLab_SendCommand(scope, 'TIME_DIV 50E-6');
DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 100K');
DwPhyLab_SendCommand(scope, 'TRIG_DELAY -50E-6');
DwPhyLab_SendCommand(scope, 'C3:VDIV 0.070V');
pause(3);
DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM'); 
pause(1);

data.PCOUNT0 = DwPhyLab_ReadRegister(256+77, '7:1');
DwPhyLab_Wake; tic;
k=0;

DwPhyMex('RvClientStartTxBatch',1e9,1500-4,11,TXPWRLVL,0);
while toc < T1,
    k = k+1;
    DwPhyLab_SendCommand(scope, 'ARM; WAIT 1.0; C3:PARAMETER_VALUE? SDEV');
    ScopeResponse{k} = DwPhyLab_SendQuery(scope); %#ok<AGROW>
    data.t(k) = toc;
    data.PCOUNT(k) = DwPhyLab_ReadRegister(256+77, '7:1');
    if ishandle(hBar)
        waitbar(data.t(k)/T,hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end

DwPhyLab_SendCommand(scope, 'TRIG_MODE NORM'); pause(0.5);
DwPhyLab_TxBurst(0); pause(0.1);
DwPhyLab_TxPacket(6, 1500, TXPWRLVL);
data.k1=k;
while toc < (T1+T2);
    k = k+1;
%    DwPhyLab_TxPacket(6, 1500, TXPWRLVL);
    DwPhyLab_SendCommand(scope, 'ARM');
    DwPhyLab_TxPacket(11, 1500, TXPWRLVL);
    DwPhyLab_SendCommand(scope, 'C3:PARAMETER_VALUE? SDEV');
    data.t(k) = toc;
    ScopeResponse{k} = DwPhyLab_SendQuery(scope); %#ok<AGROW>
    data.PCOUNT(k) = DwPhyLab_ReadRegister(256+77, '7:1');
    if ishandle(hBar)
        waitbar(data.t(k)/T,hBar);
    else
        fprintf('*** TEST ABORTED ***\n');
        data.Result = -1;
        break;
    end
end

DwPhyLab_Sleep;
DwPhyLab_CloseScope(scope);

for k=1:length(ScopeResponse),
    buf = ScopeResponse{k};
    s = buf( (strfind(buf,'SDEV,')+5):length(buf) );
    i = min( [find(s == ' ') find(s == ',') find(s == 'V')] );
    s = s(1:(i-1));
    data.sdev(k) = str2double(s);
end

if ishandle(hBar), close(hBar); end;
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

%% REVISIONS
% 2010-08-04 Use DwPhyLab functions to control the scope