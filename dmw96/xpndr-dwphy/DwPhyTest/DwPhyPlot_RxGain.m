function DwPhyPlot_RxGain(data)
% DwPhyPlot_RxGain(data) plots results from data = DwPhyTest_RunTest(300.2)
% DwPhyPlot_RxGain(File) plot blocking results for the specified filename
% DwPhyPlot_RxGain(FileList) plots results for a list of files

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

for i=1:N,
    if isfield(X{i},'Test300_2')
        [Ava{i}, Avb{i}, dBr{i}, fcMHz{i}] = CalcVoltageGain(X{i}.Test300_2);
    else
        [Ava{i}, Avb{i}, dBr{i}, fcMHz{i}] = CalcVoltageGain(X{i});
    end
    LegendStr{i} = X{i}.PartID;
end

figure(1); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','Ava{%d}');
DwPhyPlot_SplitBand('FormatY',[49 58],49:58,[],'Av(dB), RXA');
legend(LegendStr);

figure(2); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','Avb{%d}');
DwPhyPlot_SplitBand('FormatY',[49 58],49:58,[],'Av(dB), RXB');

figure(3); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','dBr{%d}');
DwPhyPlot_SplitBand('FormatY',[-6 3],-6:3,[],'Relative Gain RXB/RXA (dB)');

%% CalcVoltageGain
function [AvAdB, AvBdB, dBr, fcMHz] = CalcVoltageGain(data)
fcMHz = data.fcMHz;
mWin = 10.^(data.PrdBmHigh'/10);             % Pr[mW] at the antenna
mVin = sqrt(50 * mWin * 1e-3) * [1 1] * 1e3; % mVrms at the antenna, 50 Ohm
AvdB = 20*log10(data.mVstd ./ mVin);         % Voltage gain (dB)
AvAdB = AvdB(:,1);
AvBdB = AvdB(:,2);
dBr = AvBdB - AvAdB;

