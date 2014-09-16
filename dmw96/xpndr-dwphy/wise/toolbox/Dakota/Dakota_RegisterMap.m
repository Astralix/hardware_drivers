function [RegByField, ValidAddr] = Dakota_RegisterMap(FieldName)
%[RegByField] = Dakota_RegisterMap
%
%   Returns a register map for the Dakota PHY. The address space is
%   relative to the PHY, not the MAC; i.e., the ID address is 1.

%   Developed by Barrett Brickner
%   Copyright 2003 Bermai, Inc. All rights reserved.

RegByField = GetRegByField;

if nargin == 0,
    if nargout > 1,
        ValidAddr  = GetValidAddr(RegByField);
    end
elseif nargin == 1,
    if isfield(RegByField,FieldName),
        RegByField = RegByField.(FieldName);
    else
        error('Unrecognized field name "%s"',FieldName);
    end
end

%% Define Register Map by Field
function RegByField = GetRegByField
X = [];
X = AddField(X, 'ID',        [1 7 0]);
X = AddField(X, 'PWRFAST',   [2 2 2]);
X = AddField(X, 'PWRSLOW',   [2 1 1]);
X = AddField(X, 'ModemSleep',[2 0 0]);
X = AddField(X, 'ADCBypass', [3 7 7]);
X = AddField(X, 'Diversity', [3 2 0]);
X = AddField(X, 'SREdge',    [4 6 6]);
X = AddField(X, 'SRFreq',    [4 5 4]);
X = AddField(X, 'TestMode2', [4 3 2]);
X = AddField(X, 'TxPwrlvlGAIN',[4 1 1]);
X = AddField(X, 'TxPwrlvlSR',[4 0 0]);
X = AddField(X, 'DmixEnSel', [6 6 6]);
X = AddField(X, 'DCXSelect', [6 4 4]);
X = AddField(X, 'UpmixDown', [6 3 3]);
X = AddField(X, 'DownmixUp', [6 2 2]);
X = AddField(X, 'UpmixOff',  [6 1 1]);
X = AddField(X, 'DownmixOff',[6 0 0]);
X = AddField(X, 'TestMode',  [7 7 4]);
X = AddField(X, 'PrdDACSrcR',[7 3 2]);
X = AddField(X, 'PrdDACSrcI',[7 1 0]);
X = AddField(X, 'DLY_TXB',   [8 7 6]);
X = AddField(X, 'TxDelay',   [8 5 0]);
X = AddField(X, 'TX_RATE',   [9 7 4], 1);
X = AddField(X, 'TX_LENGTH', [9 3 0; 10 7 0], 1);
X = AddField(X, 'TX_PWRLVL', [11 7 0], 1);
X = AddField(X, 'TX_SERVICE',[12 7 0; 13 7 0], 1);
X = AddField(X, 'TxFault',   [14 7 7], 1);
X = AddField(X, 'TX_PRBS',   [14 6 0]);
X = AddField(X, 'DLY_ANTSW1',[15 7 4]);
X = AddField(X, 'DLY_ANTSW0',[15 3 0]);
X = AddField(X, 'RxDelay',   [16 7 0]);
X = AddField(X, 'RxFault',   [17 0 0], 1);
X = AddField(X, 'RX_RATE',   [18 7 4], 1);
X = AddField(X, 'RX_LENGTH', [18 3 0; 19 7 0], 1);
X = AddField(X, 'RX_SERVICE',[20 7 0; 21 7 0], 1);
X = AddField(X, 'RSSI0',     [22 7 0], 1);
X = AddField(X, 'RSSI1',     [23 7 0], 1);
X = AddField(X, 'PilotPLLEn',[30 6 6]);
X = AddField(X, 'aC',        [30 5 3]);
X = AddField(X, 'bC',        [30 2 0]);
X = AddField(X, 'cC',        [31 2 0]);

for i=0:15,
    X = AddField(X, sprintf('Sa%d',i), [32+i 5 3]);
    X = AddField(X, sprintf('Sb%d',i), [32+i 2 0]);
    X = AddField(X, sprintf('Ca%d',i), [48+i 5 3]);
    X = AddField(X, sprintf('Cb%d',i), [48+i 2 0]);
end

X = AddField(X, 'LgSigAFEValid',[64 7 7]);
X = AddField(X, 'GainStepLNA',  [64 4 0]);
X = AddField(X, 'FixedGain',    [65 7 7]);
X = AddField(X, 'FixedLNAGain', [65 6 6]);
X = AddField(X, 'InitAGain',    [65 5 0]);
X = AddField(X, 'InitDGain',    [66 5 0], 0, 1);
X = AddField(X, 'ThSigLarge',   [67 6 0]);
X = AddField(X, 'ThSigSmall',   [68 6 0]);
X = AddField(X, 'StepSizeLarge',[69 4 0]);
X = AddField(X, 'StepSizeSmall',[70 4 0]);
X = AddField(X, 'ThSwitchLNA',  [71 4 0]);
X = AddField(X, 'AbsPowerL',    [72 6 0]);
X = AddField(X, 'AbsPowerH',    [73 6 0]);
X = AddField(X, 'RefPowerDig',  [74 6 0]);
X = AddField(X, 'Pwr100dBm',    [75 7 0]);
X = AddField(X, 'ThCCA',        [76 6 0]);
X = AddField(X, 'InitAGainOfs', [77 5 0]);
X = AddField(X, 'CFOFilter',    [96 7 7]);
X = AddField(X, 'CFOMux',       [96 6 6]);
X = AddField(X, 'CFOTest',      [96 5 5]);
X = AddField(X, 'WaitAGC',      [97 7 6; 96 4 0]);
X = AddField(X, 'DelayAGC',     [97 4 0]);
X = AddField(X, 'SigDetWindow', [98 7 0]);
X = AddField(X, 'SyncShift',    [99 3 0]);
X = AddField(X, 'DelayRestart', [100 7 0]);
X = AddField(X, 'SigDetFilter', [112 7 7]);
X = AddField(X, 'SigDetTh1',    [112 5 0]);
X = AddField(X, 'SigDetTh2',    [115 7 0; 114 7 0; 113 7 0]);
X = AddField(X, 'SyncFilter',   [120 7 7]);
X = AddField(X, 'SyncMag',      [120 4 0]);
X = AddField(X, 'SyncThSig',    [121 7 5]);
X = AddField(X, 'SyncThExp',    [121 4 0], 0, 1);
X = AddField(X, 'SyncDelay',    [122 4 0]);
X = AddField(X, 'minShift',     [128 3 0]);
X = AddField(X, 'ChEstShift0',  [129 7 4], 1);
X = AddField(X, 'ChEstShift1',  [129 3 0], 1);
X = AddField(X, 'DelaySpread0', [130 7 0], 1);
X = AddField(X, 'DelaySpread1', [131 7 0], 1);
RegByField = X;

%% Create a List of Valid Addresses
function ValidAddr = GetValidAddr(RegByField)
ValidAddr = [];
name = fieldnames(RegByField);
for i=1:length(name),
    A = RegByField.(name{i}).Field(:,1);
    for j=1:length(A),
        if(length(find(ValidAddr==A(j)))<1)
            ValidAddr = [ValidAddr A(j)];
        end
    end
end
ValidAddr = sort(ValidAddr);

%=========================================================================
% FUNCTION: AddField
%=========================================================================
function X = AddField(X, Name, Field, ReadOnly, Signed)
if nargin<3, error('Must specify Name and Field'); end
X.(Name).Field    = Field;
if nargin >= 4,
    if ReadOnly,
        X.(Name).ReadOnly = 1; 
    end;
end
if nargin >= 5,
    if Signed,
        X.(Name).Signed = 1; 
    end 
else
    Signed = 0;
end;

if(Signed)
    X.(Name).min = -2^(sum(Field(:,2))-sum(Field(:,3))+length(Field(:,3))-1);
    X.(Name).max = -X.(Name).min - 1;
else
    X.(Name).min = 0;
    X.(Name).max = 2^(sum(Field(:,2))-sum(Field(:,3))+length(Field(:,3))) - 1;
end