function data = DwPhyTest_MeasureRxOffset(varargin)
% data = DwPhyTest_MeasureRxOffset(...)
%
%    Measure RX offset at ADC output with minimum receiver gain.
%    Valid parameters include fcMHz, DCXShift, and Npts

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
fcMHz = 2412;
Npts = 1000;
DCXShift = 5;
DiversityMode = 3;

DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
DwPhyLab_Sleep;
DwPhyLab_WriteRegister(65,0);            % AGAIN = 0
DwPhyLab_WriteRegister(80,128);          % Fixed Gain: LNAGAIN=0
if DwPhyLab_RadioIsRF22
    DwPhyLab_WriteRegister(256+114,'3:0',0); % PGA to minimum gain
else
    DwPhyLab_WriteRegister(256+115,'7:6',0); % PGA to minimum gain
end
DwPhyLab_WriteRegister(4,1);             % RXMode=1 (OFDM Only)
DwPhyLab_WriteRegister(103,2,1);         % Disable OFDM carrier sense
DwPhyLab_Wake;

DwPhyLab_WriteRegister(144,'2:0',DCXShift); % Filter BW

for i=1:Npts,
    pause(0.01);
    DwPhyLab_WriteRegister(144,5,1); % hold accumulators
    data.Acc0R(i) = ReadAccumulator(145);
    data.Acc0I(i) = ReadAccumulator(147);
    data.Acc1R(i) = ReadAccumulator(149);
    data.Acc1I(i) = ReadAccumulator(151);
    DwPhyLab_WriteRegister(144,5,0); % enable filter
end

K = 2^-(DCXShift + 2);
data.Offset0 = K * (mean(data.Acc0R) + j*mean(data.Acc0I));
data.Offset1 = K * (mean(data.Acc1R) + j*mean(data.Acc1I));
data.AvgOfs = round([real(data.Offset0) imag(data.Offset0) real(data.Offset1) imag(data.Offset1)]);
if max(data.AvgOfs) > 30, Result = 0; end
data.Runtime = 24*3600*(now-datenum(data.Timestamp));

%% ReadAccumulator
function X = ReadAccumulator(A)
xH = DwPhyLab_ReadRegister(A+0); % DCXAccXX[12:8]
xL = DwPhyLab_ReadRegister(A+1); % DCXAccXX[ 7:0]
X = 256*xH + xL;
if bitget(X,13)   % check for negative value
    X = X - 2^13; % negative two's complement
end

    

    