function data = DwPhyTest_CapSel(varargin)
% data = DwPhyTest_CapSel(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;

DwPhyLab_EvaluateVarargin(varargin);

d = 0;


%% Run Test
DwPhyTest_RunTestCommon;

for k=1:length(fcMHz),
    DwPhyLab_Sleep;
    DwPhyLab_Initialize;
    DwPhyLab_SetChannelFreq(fcMHz(k));
    DwPhyLab_Wake;
    
    data.CAPSEL0 = DwPhyLab_ReadRegister(256+42,'4:0');
    [data.DACHI(k,1), data.DACLO(k,1), data.CAPSEL(k,1)] = MeasureVtune;
 
    DwPhyLab_WriteRegister(256+35,bin2dec('11100000')+data.CAPSEL0-1);
    
%    DwPhyLab_WriteRegister(256+25, 2);
%    DwPhyLab_WriteRegister(256+26, 5);
%    ClosedLoopCalibration(1);
    [data.DACHI(k,2), data.DACLO(k,2), data.CAPSEL(k,2)] = MeasureVtune;
    
%    DwPhyLab_WriteRegister(256+25, 23);
%    DwPhyLab_WriteRegister(256+26,  8);
%    ClosedLoopCalibration(3);
    DwPhyLab_WriteRegister(256+35,bin2dec('11100000')+data.CAPSEL0);
    [data.DACHI(k,3), data.DACLO(k,3), data.CAPSEL(k,3)] = MeasureVtune;
    
%    DwPhyLab_WriteRegister(256+25, 31);
%    DwPhyLab_WriteRegister(256+26, 30);
%    ClosedLoopCalibration(1);
    DwPhyLab_WriteRegister(256+35,bin2dec('11100000')+data.CAPSEL0+1);
    [data.DACHI(k,4), data.DACLO(k,4), data.CAPSEL(k,4)] = MeasureVtune;

end

%% MeasureVtune
function [DACHI, DACLO, CAPSEL] = MeasureVtune

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
CAPSEL = DwPhyLab_ReadRegister(256+42,'4:0');

if isempty(DACHI), DACHI=NaN; end;
if isempty(DACLO), DACLO=NaN; end;

%% ClosedLoopCalibration
function ClosedLoopCalibration(n)

for i=1:n,
    DwPhyMex('DwPhy_PllClosedLoopCalibration');
end
return;

A = 256+26;
D = DwPhyLab_ReadRegister(A);
DwPhyLab_WriteRegister(A, bitand(D, bin2dec('10000000')));
for i=1:n,
    DwPhyLab_WriteRegister(A, bitand(D, bin2dec('11000000')));
    DwPhyLab_WriteRegister(A, bitand(D, bin2dec('10000000')));
end
DwPhyLab_WriteRegister(A, bitand(D, bin2dec('00000000')));
    

