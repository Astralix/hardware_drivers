function data = DwPhyTest_CapSel(varargin)
% data = DwPhyTest_CapSel(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;

DwPhyLab_EvaluateVarargin(varargin);

d = -2:2;
d=0;

%% Run Test
DwPhyTest_RunTestCommon;

for k=1:length(fcMHz),
    DwPhyLab_Sleep;
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(fcMHz(k));
    DwPhyLab_Wake;
    
    data.CAPSEL = DwPhyLab_ReadRegister(256+42,'4:0');
    
    for i=1:length(d),

%        DwPhyLab_WriteRegister(256+35,bin2dec('11100000')+data.CAPSEL+d(i));
        [DACHI, DACLO] = MeasureVtune;
        data.DACHI(k,i) = DACHI;
        data.DACLO(k,i) = DACLO;
        data.ReadCAPSEL(k,i) = DwPhyLab_ReadRegister(256+42,'4:0');
    
    end
    
end

%% MeasureVtune
function [DACHI, DACLO] = MeasureVtune


DwPhyLab_WriteRegister(256+25,6,1); % set COMPENBYP

for i=0:31,
    DwPhyLab_WriteRegister(256+25,i);
    DwPhyLab_WriteRegister(256+26,i);
    DwPhyLab_WriteRegister(256+25,bitset(i,6,1)); pause(0.1);
    DwPhyLab_WriteRegister(256+25,i);    
    x = DwPhyLab_ReadRegister(256+42);
    VTUNELO(i+1) = bitget(x, 1+6);
    VTUNEHI(i+1) = bitget(x, 1+5);
end
DACHI = find(VTUNEHI == 1, 1, 'last' ) - 1;
DACLO = find(VTUNELO == 1, 1, 'first') - 1;

if isempty(DACHI), DACHI=NaN; end;
if isempty(DACLO), DACLO=NaN; end;

%% ClosedLoopCalibration
function ClosedLoopCalibration(n)
A = 256+26;
D = DwPhyLab_ReadRegister(A);
DwPhyLab_WriteRegister(A, bitand(D, bin2dec('10000000')));
for i=1:n,
    DwPhyLab_WriteRegister(A, bitand(D, bin2dec('11000000')));
    DwPhyLab_WriteRegister(A, bitand(D, bin2dec('10000000')));
end
DwPhyLab_WriteRegister(A, bitand(D, bin2dec('00000000')));
    

