function data = DwPhyTest_VtuneCompare(varargin)
% data = DwPhyTest_VtuneCompare(...)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;
VTUNE = 0:31;
DwPhyLab_EvaluateVarargin(varargin);


%% Run Test
DwPhyTest_RunTestCommon;

for k=1:length(VTUNE)

    DwPhyLab_WriteRegister(256+24, 2*data.VTUNE(k) + 1);
    [data.DACHI(k), data.DACLO(k)] = MeasureVtune;
   
end

%% MeasureVtune
function [DACHI, DACLO] = MeasureVtune

DwPhyLab_WriteRegister(256+25,6,1); % set COMPENBYP

for i=0:31,
    DwPhyLab_WriteRegister(256+25,i);
    DwPhyLab_WriteRegister(256+26,0);
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
