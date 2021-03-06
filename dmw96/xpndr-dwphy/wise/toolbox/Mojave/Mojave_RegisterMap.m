function [RegByField, ValidAddr] = Mojave_RegisterMap(FieldName)
%[RegByField] = Mojave_RegisterMap
%   Returns a register map for the Mojave baseband

%   Developed by Barrett Brickner
%   Copyright 2007 DSP Group, Inc. All rights reserved.

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
X = AddField(X, 'ID',              [  1 7 0], 1, 0);
X = AddField(X, 'PathSel',         [  3 1 0], 0, 0);
X = AddField(X, 'RXMode',          [  4 1 0], 0, 0);
X = AddField(X, 'UpmixConj',       [  6 7 7], 0, 0);
X = AddField(X, 'DownmixConj',     [  6 6 6], 0, 0);
X = AddField(X, 'DmixEnDis',       [  6 5 5], 0, 0);
X = AddField(X, 'UpmixDown',       [  6 3 3], 0, 0);
X = AddField(X, 'DownmixUp',       [  6 2 2], 0, 0);
X = AddField(X, 'UpmixOff',        [  6 1 1], 0, 0);
X = AddField(X, 'DownmixOff',      [  6 0 0], 0, 0);
X = AddField(X, 'TestMode',        [  7 6 4], 0, 0);
X = AddField(X, 'PrdDACSrcR',      [  7 3 2], 0, 0);
X = AddField(X, 'PrdDACSrcI',      [  7 1 0], 0, 0);
X = AddField(X, 'TxExtend',        [  8 7 6], 0, 0);
X = AddField(X, 'TxDelay',         [  8 5 0], 0, 0);
X = AddField(X, 'TX_RATE',         [  9 7 4], 1, 0);
X = AddField(X, 'TX_LENGTH',       [  9 3 0; 10 7 0], 1, 0);
X = AddField(X, 'TX_PWRLVL',       [ 11 7 0], 1, 0);
X = AddField(X, 'TX_SERVICE',      [ 12 7 0; 13 7 0], 0, 0);
X = AddField(X, 'TxFault',         [ 14 7 7], 1, 0);
X = AddField(X, 'TX_PRBS',         [ 14 6 0], 0, 0);
X = AddField(X, 'bRxMode',         [ 17 7 7], 1, 0);
X = AddField(X, 'RxFault',         [ 17 0 0], 1, 0);
X = AddField(X, 'RX_RATE',         [ 18 7 4], 1, 0);
X = AddField(X, 'RX_LENGTH',       [ 18 3 0; 19 7 0], 1, 0);
X = AddField(X, 'RX_SERVICE',      [ 20 7 0; 21 7 0], 1, 0);
X = AddField(X, 'RSSI0',           [ 22 7 0], 1, 0);
X = AddField(X, 'RSSI1',           [ 23 7 0], 1, 0);
X = AddField(X, 'maxLENGTH',       [ 24 7 0], 0, 0);
X = AddField(X, 'Disable72',       [ 25 1 1], 0, 0);
X = AddField(X, 'Reserved0',       [ 25 0 0], 0, 0);
X = AddField(X, 'PilotPLLEn',      [ 30 6 6], 0, 0);
X = AddField(X, 'aC',              [ 30 5 3], 0, 0);
X = AddField(X, 'bC',              [ 30 2 0], 0, 0);
X = AddField(X, 'cC',              [ 31 2 0], 0, 0);
X = AddField(X, 'Sa0',             [ 32 5 3], 0, 0);
X = AddField(X, 'Sb0',             [ 32 2 0], 0, 0);
X = AddField(X, 'Sa1',             [ 33 5 3], 0, 0);
X = AddField(X, 'Sb1',             [ 33 2 0], 0, 0);
X = AddField(X, 'Sa2',             [ 34 5 3], 0, 0);
X = AddField(X, 'Sb2',             [ 34 2 0], 0, 0);
X = AddField(X, 'Sa3',             [ 35 5 3], 0, 0);
X = AddField(X, 'Sb3',             [ 35 2 0], 0, 0);
X = AddField(X, 'Sa4',             [ 36 5 3], 0, 0);
X = AddField(X, 'Sb4',             [ 36 2 0], 0, 0);
X = AddField(X, 'Sa5',             [ 37 5 3], 0, 0);
X = AddField(X, 'Sb5',             [ 37 2 0], 0, 0);
X = AddField(X, 'Sa6',             [ 38 5 3], 0, 0);
X = AddField(X, 'Sb6',             [ 38 2 0], 0, 0);
X = AddField(X, 'Sa7',             [ 39 5 3], 0, 0);
X = AddField(X, 'Sb7',             [ 39 2 0], 0, 0);
X = AddField(X, 'Sa8',             [ 40 5 3], 0, 0);
X = AddField(X, 'Sb8',             [ 40 2 0], 0, 0);
X = AddField(X, 'Sa9',             [ 41 5 3], 0, 0);
X = AddField(X, 'Sb9',             [ 41 2 0], 0, 0);
X = AddField(X, 'Sa10',            [ 42 5 3], 0, 0);
X = AddField(X, 'Sb10',            [ 42 2 0], 0, 0);
X = AddField(X, 'Sa11',            [ 43 5 3], 0, 0);
X = AddField(X, 'Sb11',            [ 43 2 0], 0, 0);
X = AddField(X, 'Sa12',            [ 44 5 3], 0, 0);
X = AddField(X, 'Sb12',            [ 44 2 0], 0, 0);
X = AddField(X, 'Sa13',            [ 45 5 3], 0, 0);
X = AddField(X, 'Sb13',            [ 45 2 0], 0, 0);
X = AddField(X, 'Sa14',            [ 46 5 3], 0, 0);
X = AddField(X, 'Sb14',            [ 46 2 0], 0, 0);
X = AddField(X, 'Sa15',            [ 47 5 3], 0, 0);
X = AddField(X, 'Sb15',            [ 47 2 0], 0, 0);
X = AddField(X, 'Ca0',             [ 48 5 3], 0, 0);
X = AddField(X, 'Cb0',             [ 48 2 0], 0, 0);
X = AddField(X, 'Ca1',             [ 49 5 3], 0, 0);
X = AddField(X, 'Cb1',             [ 49 2 0], 0, 0);
X = AddField(X, 'Ca2',             [ 50 5 3], 0, 0);
X = AddField(X, 'Cb2',             [ 50 2 0], 0, 0);
X = AddField(X, 'Ca3',             [ 51 5 3], 0, 0);
X = AddField(X, 'Cb3',             [ 51 2 0], 0, 0);
X = AddField(X, 'Ca4',             [ 52 5 3], 0, 0);
X = AddField(X, 'Cb4',             [ 52 2 0], 0, 0);
X = AddField(X, 'Ca5',             [ 53 5 3], 0, 0);
X = AddField(X, 'Cb5',             [ 53 2 0], 0, 0);
X = AddField(X, 'Ca6',             [ 54 5 3], 0, 0);
X = AddField(X, 'Cb6',             [ 54 2 0], 0, 0);
X = AddField(X, 'Ca7',             [ 55 5 3], 0, 0);
X = AddField(X, 'Cb7',             [ 55 2 0], 0, 0);
X = AddField(X, 'Ca8',             [ 56 5 3], 0, 0);
X = AddField(X, 'Cb8',             [ 56 2 0], 0, 0);
X = AddField(X, 'Ca9',             [ 57 5 3], 0, 0);
X = AddField(X, 'Cb9',             [ 57 2 0], 0, 0);
X = AddField(X, 'Ca10',            [ 58 5 3], 0, 0);
X = AddField(X, 'Cb10',            [ 58 2 0], 0, 0);
X = AddField(X, 'Ca11',            [ 59 5 3], 0, 0);
X = AddField(X, 'Cb11',            [ 59 2 0], 0, 0);
X = AddField(X, 'Ca12',            [ 60 5 3], 0, 0);
X = AddField(X, 'Cb12',            [ 60 2 0], 0, 0);
X = AddField(X, 'Ca13',            [ 61 5 3], 0, 0);
X = AddField(X, 'Cb13',            [ 61 2 0], 0, 0);
X = AddField(X, 'Ca14',            [ 62 5 3], 0, 0);
X = AddField(X, 'Cb14',            [ 62 2 0], 0, 0);
X = AddField(X, 'Ca15',            [ 63 5 3], 0, 0);
X = AddField(X, 'Cb15',            [ 63 2 0], 0, 0);
X = AddField(X, 'RefPwrDig',       [ 64 6 0], 0, 0);
X = AddField(X, 'InitAGain',       [ 65 5 0], 0, 0);
X = AddField(X, 'AbsPwrL',         [ 66 6 0], 0, 0);
X = AddField(X, 'AbsPwrH',         [ 67 6 0], 0, 0);
X = AddField(X, 'ThSigLarge',      [ 68 6 0], 0, 0);
X = AddField(X, 'ThSigSmall',      [ 69 6 0], 0, 0);
X = AddField(X, 'StepLgSigDet',    [ 70 4 0], 0, 0);
X = AddField(X, 'StepLarge',       [ 71 4 0], 0, 0);
X = AddField(X, 'StepSmall',       [ 72 4 0], 0, 0);
X = AddField(X, 'ThSwitchLNA',     [ 73 5 0], 0, 0);
X = AddField(X, 'StepLNA',         [ 74 4 0], 0, 0);
X = AddField(X, 'ThSwitchLNA2',    [ 75 5 0], 0, 0);
X = AddField(X, 'StepLNA2',        [ 76 4 0], 0, 0);
X = AddField(X, 'Pwr100dBm',       [ 77 4 0], 0, 0);
X = AddField(X, 'ThCCA1',          [ 78 5 0], 0, 0);
X = AddField(X, 'ThCCA2',          [ 79 5 0], 0, 0);
X = AddField(X, 'FixedGain',       [ 80 7 7], 0, 0);
X = AddField(X, 'FixedLNAGain',    [ 80 6 6], 0, 0);
X = AddField(X, 'FixedLNA2Gain',   [ 80 5 5], 0, 0);
X = AddField(X, 'UpdateOnLNA',     [ 80 4 4], 0, 0);
X = AddField(X, 'MaxUpdateCount',  [ 80 2 0], 0, 0);
X = AddField(X, 'ThStepUpRefPwr',  [ 81 7 0], 0, 0);
X = AddField(X, 'ThStepUp',        [ 82 7 4], 0, 0);
X = AddField(X, 'ThStepDown',      [ 82 3 0], 0, 0);
X = AddField(X, 'ThDFSUp',         [ 83 6 0], 0, 0);
X = AddField(X, 'ThDFSDn',         [ 84 6 0], 0, 0);
X = AddField(X, 'DFSOn',           [ 87 7 7], 0, 0);
X = AddField(X, 'DFSCCA',          [ 87 6 6], 0, 0);
X = AddField(X, 'DFS2Candidates',  [ 87 5 5], 0, 0);
X = AddField(X, 'DFSSmooth',       [ 87 4 4], 0, 0);
X = AddField(X, 'DFSMACFilter',    [ 87 3 3], 0, 0);
X = AddField(X, 'DFSHdrFilter',    [ 87 2 2], 0, 0);
X = AddField(X, 'DFSIntDet',       [ 87 0 0], 0, 0);
X = AddField(X, 'DFSPattern',      [ 88 5 4], 0, 0);
X = AddField(X, 'DFSminPPB',       [ 88 2 0], 0, 0);
X = AddField(X, 'DFSdTPRF',        [ 89 3 0], 0, 0);
X = AddField(X, 'DFSmaxWidth',     [ 90 7 0], 0, 0);
X = AddField(X, 'DFSmaxTPRF',      [ 91 7 0], 0, 0);
X = AddField(X, 'DFSminTPRF',      [ 92 7 0], 0, 0);
X = AddField(X, 'DFSminGap',       [ 93 6 0], 0, 0);
X = AddField(X, 'DFSPulseSR',      [ 94 7 0], 1, 0);
X = AddField(X, 'detRadar',        [ 95 7 7], 1, 0);
X = AddField(X, 'detHeader',       [ 95 3 3], 1, 0);
X = AddField(X, 'detPreamble',     [ 95 2 2], 1, 0);
X = AddField(X, 'detStepDown',     [ 95 1 1], 1, 0);
X = AddField(X, 'detStepUp',       [ 95 0 0], 1, 0);
X = AddField(X, 'WaitAGC',         [ 96 6 0], 0, 0);
X = AddField(X, 'DelayAGC',        [ 97 4 0], 0, 0);
X = AddField(X, 'SigDetWindow',    [ 98 7 0], 0, 0);
X = AddField(X, 'SyncShift',       [ 99 3 0], 0, 0);
X = AddField(X, 'DelayRestart',    [100 7 0], 0, 0);
X = AddField(X, 'OFDMSwDEn',       [101 7 7], 0, 0);
X = AddField(X, 'OFDMSwDSave',     [101 6 6], 0, 0);
X = AddField(X, 'OFDMSwDInit',     [101 5 5], 0, 0);
X = AddField(X, 'OFDMSwDDelay',    [101 4 0], 0, 0);
X = AddField(X, 'OFDMSwDTh',       [102 7 0], 0, 0);
X = AddField(X, 'CFOFilter',       [103 7 7], 0, 0);
X = AddField(X, 'ArmLgSigDet3',    [103 5 5], 0, 0);
X = AddField(X, 'LgSigAFEValid',   [103 4 4], 0, 0);
X = AddField(X, 'IdleFSEnB',       [103 3 3], 0, 0);
X = AddField(X, 'IdleSDEnB',       [103 2 2], 0, 0);
X = AddField(X, 'IdleDAEnB',       [103 1 1], 0, 0);
X = AddField(X, 'DelayCFO',        [104 4 0], 0, 0);
X = AddField(X, 'DelayRSSI',       [105 5 0], 0, 0);
X = AddField(X, 'SigDetTh1',       [112 5 0], 0, 0);
X = AddField(X, 'SigDetTh2',       [113 7 0; 114 7 0], 0, 0);
X = AddField(X, 'SigDetDelay',     [115 7 0], 0, 0);
X = AddField(X, 'SigDetFilter',    [116 7 7], 0, 0);
X = AddField(X, 'SyncFilter',      [120 7 7], 0, 0);
X = AddField(X, 'SyncMag',         [120 4 0], 0, 0);
X = AddField(X, 'SyncThSig',       [121 7 4], 0, 0);
X = AddField(X, 'SyncThExp',       [121 3 0], 0, 1);
X = AddField(X, 'SyncDelay',       [122 4 0], 0, 0);
X = AddField(X, 'MinShift',        [128 3 0], 0, 0);
X = AddField(X, 'ChEstShift0',     [129 7 4], 1, 0);
X = AddField(X, 'ChEstShift1',     [129 3 0], 1, 0);
X = AddField(X, 'aC1',             [132 5 3], 0, 0);
X = AddField(X, 'bC1',             [132 2 0], 0, 0);
X = AddField(X, 'aS1',             [133 5 3], 0, 0);
X = AddField(X, 'bS1',             [133 2 0], 0, 0);
X = AddField(X, 'aC2',             [134 5 3], 0, 0);
X = AddField(X, 'bC2',             [134 2 0], 0, 0);
X = AddField(X, 'aS2',             [135 5 3], 0, 0);
X = AddField(X, 'bS2',             [135 2 0], 0, 0);
X = AddField(X, 'DCXSelect',       [144 7 7], 0, 0);
X = AddField(X, 'DCXFastLoad',     [144 6 6], 0, 0);
X = AddField(X, 'DCXHoldAcc',      [144 5 5], 0, 0);
X = AddField(X, 'DCXShift',        [144 2 0], 0, 0);
X = AddField(X, 'DCXAcc0R',        [145 4 0; 146 7 0], 0, 1);
X = AddField(X, 'DCXAcc0I',        [147 4 0; 148 7 0], 0, 1);
X = AddField(X, 'DCXAcc1R',        [149 4 0; 150 7 0], 0, 1);
X = AddField(X, 'DCXAcc1I',        [151 4 0; 152 7 0], 0, 1);
X = AddField(X, 'INITdelay',       [160 7 0], 0, 0);
X = AddField(X, 'EDwindow',        [161 7 6], 0, 0);
X = AddField(X, 'CQwindow',        [161 5 4], 0, 0);
X = AddField(X, 'SSwindow',        [161 2 0], 0, 0);
X = AddField(X, 'CQthreshold',     [162 5 0], 0, 0);
X = AddField(X, 'EDthreshold',     [163 5 0], 0, 0);
X = AddField(X, 'bWaitAGC',        [164 5 0], 0, 0);
X = AddField(X, 'bAGCdelay',       [165 5 0], 0, 0);
X = AddField(X, 'bRefPwr',         [166 5 0], 0, 0);
X = AddField(X, 'bInitAGain',      [167 5 0], 0, 0);
X = AddField(X, 'bThSigLarge',     [168 5 0], 0, 0);
X = AddField(X, 'bStepLarge',      [169 4 0], 0, 0);
X = AddField(X, 'bThSwitchLNA',    [170 5 0], 0, 0);
X = AddField(X, 'bStepLNA',        [171 4 0], 0, 0);
X = AddField(X, 'bThSwitchLNA2',   [172 5 0], 0, 0);
X = AddField(X, 'bStepLNA2',       [173 4 0], 0, 0);
X = AddField(X, 'bPwr100dBm',      [174 4 0], 0, 0);
X = AddField(X, 'DAmpGainRange',   [175 3 0], 0, 0);
X = AddField(X, 'bMaxUpdateCount', [176 3 0], 0, 0);
X = AddField(X, 'CSMode',          [177 5 4], 0, 0);
X = AddField(X, 'bPwrStepDet',     [177 3 3], 0, 0);
X = AddField(X, 'bCFOQual',        [177 2 2], 0, 0);
X = AddField(X, 'bAGCBounded4',    [177 1 1], 0, 0);
X = AddField(X, 'bLgSigAFEValid',  [177 0 0], 0, 0);
X = AddField(X, 'bSwDEn',          [178 7 7], 0, 0);
X = AddField(X, 'bSwDSave',        [178 6 6], 0, 0);
X = AddField(X, 'bSwDInit',        [178 5 5], 0, 0);
X = AddField(X, 'bSwDDelay',       [178 4 0], 0, 0);
X = AddField(X, 'bSwDTh',          [179 7 0], 0, 0);
X = AddField(X, 'bThStepUpRefPwr', [181 7 0], 0, 0);
X = AddField(X, 'bThStepUp',       [182 7 4], 0, 0);
X = AddField(X, 'bThStepDown',     [182 3 0], 0, 0);
X = AddField(X, 'SymHDRCE',        [184 5 0], 0, 0);
X = AddField(X, 'CENSym1',         [185 6 4], 0, 0);
X = AddField(X, 'CENSym2',         [185 2 0], 0, 0);
X = AddField(X, 'hThreshold1',     [186 6 4], 0, 0);
X = AddField(X, 'hThreshold2',     [186 2 0], 0, 0);
X = AddField(X, 'hThreshold3',     [187 2 0], 0, 0);
X = AddField(X, 'SFDWindow',       [188 7 0], 0, 0);
X = AddField(X, 'AntSel',          [192 7 7], 1, 0);
X = AddField(X, 'CFO',             [193 7 0], 1, 0);
X = AddField(X, 'RxFaultCount',    [194 7 0], 1, 0);
X = AddField(X, 'RestartCount',    [195 7 0], 1, 0);
X = AddField(X, 'NuisanceCount',   [196 7 0], 1, 0);
X = AddField(X, 'RSSIdB',          [197 4 4], 0, 0);
X = AddField(X, 'ClrOnWake',       [197 2 2], 0, 0);
X = AddField(X, 'ClrOnHdr',        [197 1 1], 0, 0);
X = AddField(X, 'ClrNow',          [197 0 0], 0, 0);
X = AddField(X, 'Filter64QAM',     [198 7 7], 0, 0);
X = AddField(X, 'Filter16QAM',     [198 6 6], 0, 0);
X = AddField(X, 'FilterQPSK',      [198 5 5], 0, 0);
X = AddField(X, 'FilterBPSK',      [198 4 4], 0, 0);
X = AddField(X, 'FilterAllTypes',  [198 1 1], 0, 0);
X = AddField(X, 'AddrFilter',      [198 0 0], 0, 0);
X = AddField(X, 'STA0',            [199 7 0], 0, 0);
X = AddField(X, 'STA1',            [200 7 0], 0, 0);
X = AddField(X, 'STA2',            [201 7 0], 0, 0);
X = AddField(X, 'STA3',            [202 7 0], 0, 0);
X = AddField(X, 'STA4',            [203 7 0], 0, 0);
X = AddField(X, 'STA5',            [204 7 0], 0, 0);
X = AddField(X, 'minRSSI',         [205 7 0], 0, 0);
X = AddField(X, 'StepUpRestart',   [206 7 7], 0, 0);
X = AddField(X, 'StepDownRestart', [206 6 6], 0, 0);
X = AddField(X, 'NoStepAfterSync', [206 5 5], 0, 0);
X = AddField(X, 'NoStepAfterHdr',  [206 4 4], 0, 0);
X = AddField(X, 'KeepCCA_New',     [206 1 1], 0, 0);
X = AddField(X, 'KeepCCA_Lost',    [206 0 0], 0, 0);
X = AddField(X, 'WakeupTime',      [208 7 0; 209 7 0], 0, 0);
%X = AddField(X, 'WakeupTimeL',     [209 7 0], 0, 0);
X = AddField(X, 'DelayStdby',      [210 7 0], 0, 0);
X = AddField(X, 'DelayBGEnB',      [211 7 0], 0, 0);
X = AddField(X, 'DelayADC1',       [212 7 0], 0, 0);
X = AddField(X, 'DelayADC2',       [213 7 0], 0, 0);
X = AddField(X, 'DelayModem',      [214 7 0], 0, 0);
X = AddField(X, 'DelayDFE',        [215 7 0], 0, 0);
X = AddField(X, 'DelayRFSW',       [216 7 4], 0, 0);
X = AddField(X, 'TxRxTime',        [216 3 0], 0, 0);
X = AddField(X, 'DelayFCAL1',      [217 7 4], 0, 0);
X = AddField(X, 'DelayPA',         [217 3 0], 0, 0);
X = AddField(X, 'DelayFCAL2',      [218 7 4], 0, 0);
X = AddField(X, 'DelayState28',    [218 3 0], 0, 0);
X = AddField(X, 'DelayShutdown',   [219 7 4], 0, 0);
X = AddField(X, 'DelaySleep',      [219 3 0], 0, 0);
X = AddField(X, 'TimeExtend',      [220 7 0], 0, 0);
X = AddField(X, 'SquelchTime',     [221 5 0], 0, 0);
X = AddField(X, 'DelaySquelchRF',  [222 7 4], 0, 0);
X = AddField(X, 'DelayClearDCX',   [222 3 0], 0, 0);
X = AddField(X, 'SelectCCA',       [223 3 0], 0, 0);
X = AddField(X, 'RadioMC',         [224 7 0], 0, 0);
X = AddField(X, 'DACOPM',          [225 5 0], 0, 0);
X = AddField(X, 'ADCAOPM',         [226 5 0], 0, 0);
X = AddField(X, 'ADCBOPM',         [227 5 0], 0, 0);
X = AddField(X, 'RFSW1',           [228 7 0], 0, 0);
X = AddField(X, 'RFSW2',           [229 7 0], 0, 0);
X = AddField(X, 'LNAEnB',          [230 7 6], 0, 0);
X = AddField(X, 'PA24_MODE',       [230 3 2], 0, 0);
X = AddField(X, 'PA50_MODE',       [230 1 0], 0, 0);
X = AddField(X, 'LNA24A_MODE',     [231 7 6], 0, 0);
X = AddField(X, 'LNA24B_MODE',     [231 5 4], 0, 0);
X = AddField(X, 'LNA50A_MODE',     [231 3 2], 0, 0);
X = AddField(X, 'LNA50B_MODE',     [231 1 0], 0, 0);
X = AddField(X, 'ADCFSCTRL',       [232 7 6], 0, 0);
X = AddField(X, 'ADCOE',           [232 4 4], 0, 0);
X = AddField(X, 'LNAxxBOnSW2',     [232 3 3], 0, 0);
X = AddField(X, 'ADCOFCEN',        [232 2 2], 0, 0);
X = AddField(X, 'ADCDCREN',        [232 1 1], 0, 0);
X = AddField(X, 'ADCMUX0',         [232 0 0], 0, 0);
X = AddField(X, 'ADCARDY',         [233 7 7], 1, 0);
X = AddField(X, 'ADCBRDY',         [233 6 6], 1, 0);
X = AddField(X, 'ADCFCAL2',        [233 2 2], 0, 0);
X = AddField(X, 'ADCFCAL1',        [233 1 1], 0, 0);
X = AddField(X, 'ADCFCAL',         [233 0 0], 0, 0);
X = AddField(X, 'DACIFS_CTRL',     [234 2 0], 0, 0);
X = AddField(X, 'DACFG_CTRL1',     [235 7 0], 0, 0);
X = AddField(X, 'DACFG_CTRL0',     [236 7 0], 0, 0);
X = AddField(X, 'RESETB',          [237 7 7], 0, 0);
X = AddField(X, 'SREdge',          [237 2 2], 0, 0);
X = AddField(X, 'SRFreq',          [237 1 0], 0, 0);
if(1)
    X = AddField(X, 'BISTDone',        [768 7 7], 1, 0);
    X = AddField(X, 'BISTADCSel',      [768 6 6], 0, 0);
    X = AddField(X, 'BISTNoPrechg',    [768 5 5], 0, 0);
    X = AddField(X, 'BISTFGSel',       [768 4 4], 0, 0);
    X = AddField(X, 'BISTMode',        [768 3 1], 0, 0);
    X = AddField(X, 'StartBIST',       [768 0 0], 0, 0);
    X = AddField(X, 'BISTPipe',        [769 7 4], 0, 0);
    X = AddField(X, 'BISTN_val',       [769 3 0], 0, 0);
    X = AddField(X, 'BISTOfsR',        [770 7 0], 0, 0);
    X = AddField(X, 'BISTOfsI',        [771 7 0], 0, 0);
    X = AddField(X, 'BISTTh1',         [772 7 0], 0, 0);
    X = AddField(X, 'BISTTh2H',        [773 7 0], 0, 0);
    X = AddField(X, 'BISTTh2L',        [774 7 0], 0, 0);
    X = AddField(X, 'BISTFG_CTRLI',    [775 7 0], 0, 0);
    X = AddField(X, 'BISTFG_CTRLR',    [776 7 0], 0, 0);
    X = AddField(X, 'BISTStatus',      [777 7 0], 1, 0);
    X = AddField(X, 'BISTAccR',        [778 7 0; 779 7 0; 780 7 0], 1, 0);
    X = AddField(X, 'BISTAccI',        [781 7 0; 782 7 0; 783 7 0], 1, 0);
    X = AddField(X, 'BISTRangeLimH',   [784 7 4], 0, 0);
    X = AddField(X, 'BISTRangeLimL',   [784 3 0], 0, 0);
    X = AddField(X, 'DFSIntEOB',       [800 1 1], 0, 0);
    X = AddField(X, 'DFSIntFIFO',      [800 0 0], 0, 0);
    X = AddField(X, 'DFSThFIFO',       [801 6 0], 0, 0);
    X = AddField(X, 'DFSDepthFIFO',    [802 7 0], 1, 0);
    X = AddField(X, 'DFSReadFIFO',     [803 7 0], 1, 0);
    X = AddField(X, 'DFSSyncFIFO',     [804 0 0], 1, 0);
end
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
    if ReadOnly
        X.(Name).ReadOnly = 1; 
    end;
end
if nargin >= 5,
    if(Signed)
        X.(Name).Signed = 1; 
    end 
else
    Signed = 0;
end;

if Signed
    X.(Name).min = -2^(sum(Field(:,2))-sum(Field(:,3))+length(Field(:,3))-1);
    X.(Name).max = -X.(Name).min - 1;
else
    X.(Name).min = 0;
    X.(Name).max = 2^(sum(Field(:,2))-sum(Field(:,3))+length(Field(:,3))) - 1;
end