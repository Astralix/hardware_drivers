function DwPhyPlot_RxEval(data)
% DwPhyPlot_RxEval(data) plots results for data = DwPhyTest_RunSuite('RxEval')
% DwPhyPlot_RxEval(File) plot RxEval results for the specified filename
% DwPhyPlot_RxEval(FileList) plots results for a list of files

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X, N] = DwPhyPlot_LoadData(data);
IncompleteMRC = 0;

for i=1:N,
    [fcMHz{i}, RxA{i}, RxB{i}, MRC{i}, Rx6{i}, IMR1{i}, IMR2{i}, ...
        PERa2412{i}, PERa5200{i}, PERb2412{i}, PERb5200{i}, PERc5200{i}, PERc2412{i},...
        PERd2412{i}] = GetData(X{i});  %#ok<NASGU>
    DataSet{i} = X{i}.PartID;
    
    if isempty(MRC{i}), IncompleteMRC = 1; end
end

Mbps102_5 = X{1}.Test102_5.Mbps;

% Draw individual plots...
figure(1); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','RxA{%d}');
DwPhyPlot_SplitBand('FormatY',[-78 -65],-80:-65,[],'Sensitivity, 54 Mbps, RXA');
legend(DataSet);

figure(2); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','RxB{%d}');
DwPhyPlot_SplitBand('FormatY',[-78 -65],-80:-65,[],'Sensitivity, 54 Mbps, RXB');
legend(DataSet);

if ~IncompleteMRC,
    figure(3); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','MRC{%d}');
    DwPhyPlot_SplitBand('FormatY',[-78 -65],-80:-65,[],'Sensitivity, 54 Mbps, DualRX-MRC');
    legend(DataSet);
end

figure(4); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','IMR1{%d}');
DwPhyPlot_SplitBand('FormatY',[26 50],26:2:50,[],'RX IMR (dB), RXA');
legend(DataSet);

figure(5); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','IMR2{%d}');
DwPhyPlot_SplitBand('FormatY',[26 50],26:2:50,[],'RX IMR (dB), RXB');
legend(DataSet);

figure(6); clf;
DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERa2412{%d}');
legend(DataSet); ylabel({'PER, 54 Mbps, DualRX-MRC at 2412 MHz'});

if ~AnyEmpty(PERa5200)
    figure(7); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERa5200{%d}');
    legend(DataSet); ylabel({'PER, 54 Mbps, DualRX-MRC at 5200 MHz'});
end

figure(8); clf;
DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERb2412{%d}');
legend(DataSet); ylabel({'PER, 54 Mbps, DualRX-MRC','RX-TX-RX with 15.1 \mus SIFS at 2412 MHz'});

if ~AnyEmpty(PERb5200)
    figure(9); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERb5200{%d}');
    legend(DataSet); ylabel({'PER, 54 Mbps, DualRX-MRC','RX-TX-RX with 15.1 \mus SIFS at 5200 MHz'});
end

figure(10); clf;
DwPhyPlot_Command(N,'DwPhyPlot_PER','-80:0','PERc2412{%d}');
legend(DataSet); 
if X{1}.Test102_5.DiversityMode == 3,
    ylabel({sprintf('PER, %d Mbps, DualRX-MRC at 2412 MHz',Mbps102_5)});
else
    ylabel({sprintf('PER, %d Mbps at 2412 MHz',Mbps102_5)});
end    

if ~AnyEmpty(PERc5200)
    figure(11); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-80:0','PERc5200{%d}');
    legend(DataSet); ylabel({sprintf('PER, %d Mbps, DualRX-MRC at 5200 MHz',Mbps102_5)});
end

figure(12); clf;
DwPhyPlot_Command(N,'DwPhyPlot_PER','-99:3','PERd2412{%d}');
legend(DataSet); ylabel({'PER, 11 Mbps at 2412 MHz'});
axis([-95 3 1e-4 1]); grid on;

figure(13); clf;
DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','Rx6{%d}');
DwPhyPlot_SplitBand('FormatY',[-93 -81],-94:-80,[],'Sensitivity, 6 Mbps, DualRX-MRC');
legend(DataSet);

%% AnyEmpty
function result = AnyEmpty(S)
result = 0;
for i=1:length(S),
    if isempty(S{i}), result = 1; return; end
end

%% GetData
function [fcMHz RxA, RxB, MRC, Rx6, IMR1, IMR2, PERa2412, PERa5200, PERb2412, PERb5200, PERc5200, PERc2412, PERd2412] = GetData(X)
fcMHz = X.Test103_1.fcMHz;
RxA   = X.Test103_1.Sensitivity;
RxB   = X.Test103_2.Sensitivity;
if isfield(X,'Test103_3')
    MRC   = X.Test103_3.Sensitivity;
else
    MRC = [];
end
Rx6   = X.Test103_5.Sensitivity;
if length(size(X.Test107_1.IMRdB)) == 3,
    IMR1  = min(50, mean(X.Test107_1.IMRdB(:,:,1),2));
    IMR2  = min(50, mean(X.Test107_1.IMRdB(:,:,2),2));
else
    IMR1  = min(50, X.Test107_1.IMRdB(:,1));
    IMR2  = min(50, X.Test107_1.IMRdB(:,2));
end
PERa2412 = max(1e-5, X.Test102_2.PER);
PERb2412 = max(1e-5, X.Test106_3.PER);
PERc2412 = max(1e-5, X.Test102_5.PER);
PERd2412 = max(1e-5, X.Test102_3.PER);
if all( isfield(X,{'Test102_1','Test106_2','Test102_4'}) )
    PERa5200 = max(1e-5, X.Test102_1.PER);
    PERb5200 = max(1e-5, X.Test106_2.PER);
    PERc5200 = max(1e-5, X.Test102_4.PER);
else
    PERa5200 = []; PERb5200 = []; PERc5200 = [];
end
