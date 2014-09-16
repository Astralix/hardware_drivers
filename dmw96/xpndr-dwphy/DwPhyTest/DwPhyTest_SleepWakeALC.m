function data = DwPhyTest_SleepWakeALC(varargin)
%data = DwPhyTest_SleepWakeALC(...)
%   Meaures PCOUNT when transmitting a fixed (usually one) packet between sleep
%   cycles. This is a regression for a bug in RF52B21 fixed in RF52B31.

% Written by Barrett Brickner
% Copyright 2009 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 36;
LengthPSDU = 50;
fcMHz = 2412;
PacketType = 0;
N = 100;
M = 1;
ToggleSTARTO = 0;
TxAfterToggle = 0;
TXPWRLVL = 63;
SetRxSensitivity = -50;

DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

hBar = waitbar(0.0,'', 'Name',mfilename, 'WindowStyle','modal');

k = 0;
for n=1:length(N),
    for i=1:N(n),
        DwPhyLab_Wake; 
        pause(0.01);

        for j=1:M(n),
            k = k + 1;
            DwPhyLab_TxPacket(Mbps, LengthPSDU, TXPWRLVL); 
            PCOUNT(k) = DwPhyLab_ReadRegister(256+77,'7:1');
        end
        pause(0.05); 

        if ToggleSTARTO
            DwPhyMex('DwPhy_RF52B21_ToggleSTARTO');
        end
        if TxAfterToggle,
            k = k + 1;
            DwPhyLab_TxPacket(Mbps, LengthPSDU, TXPWRLVL); 
            PCOUNT(k) = DwPhyLab_ReadRegister(256+77,'7:1');
        end
        DwPhyLab_Sleep; 

        if ishandle(hBar)
            waitbar( k/sum(N), hBar, sprintf('PCOUNT = %d\n',PCOUNT(k)) );
        else
            fprintf('*** TEST ABORTED ***\n');
            data.Result = -1;
            break;
        end
    end
end

if (PCOUNT(k) == 0) || (PCOUNT(k) == 127),
    data.Result = 0;
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% REVISION HISTORY
% 2008-08-06 - Added transmission of 128 short packets for fast ALC convergance
%            - Added option to average power measurements
