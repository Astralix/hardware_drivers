function DwPhyPlot_RxEval22(data)
% DwPhyPlot_RxEval22(data) plots results for data = DwPhyTest_RunSuite('RxEval22')
% DwPhyPlot_RxEval22(File) plot RxEval22 results for the specified filename
% DwPhyPlot_RxEval22(FileList) plots results for a list of files

[X, N] = DwPhyPlot_LoadData(data);

for i=1:N,
    [fcMHz{i}, RxA{i}, RxB{i}, Rx6{i}, Rx54{i}, fcMHzIMR{i}, IMR1{i}, IMR2{i}, ...
     PERa2412{i}, PERb2412{i}, PERc2412{i}, PERd2412{i}] = GetData(X{i});
    DataSet{i} = X{i}.PartID;
end

% Draw individual plots...
n = 5;
if ~isempty(RxA{1})
    figure(n); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','RxA{%d}');
    DwPhyPlot_SplitBand('FormatY',[-78 -65],-80:-65,[],'Sensitivity, 54 Mbps, RXA');
    legend(DataSet);
end

if ~isempty(RxB{1})
    figure(n+1); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','RxB{%d}');
    DwPhyPlot_SplitBand('FormatY',[-78 -65],-80:-65,[],'Sensitivity, 54 Mbps, RXB');
    legend(DataSet);
end

if ~isempty(IMR1{1})
    figure(n+2); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHzIMR{%d}','IMR1{%d}');
    DwPhyPlot_SplitBand('FormatY',[26 50],26:2:50,[],'RX IMR (dB), RXA');
    legend(DataSet);
end

if ~isempty(IMR2{1})
    figure(n+3); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHzIMR{%d}','IMR2{%d}');
    DwPhyPlot_SplitBand('FormatY',[26 50],26:2:50,[],'RX IMR (dB), RXB');
    legend(DataSet);
end

if ~isempty(PERa2412{1})
    figure(n+4); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERa2412{%d}');
    legend(DataSet); ylabel(sprintf('PER, 54 Mbps, RXA at %d MHz', X{1}.Test102_2.fcMHz));
end

if ~isempty(PERb2412{1})
    figure(n+5); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-90:0','PERb2412{%d}');
    legend(DataSet); ylabel({'PER, 54 Mbps, RXA', ['RX-TX-RX with 15.1 \mus SIFS at ' sprintf('%d MHz', X{1}.Test106_3.fcMHz)]});
end

if ~isempty(PERc2412{1})
    figure(n+6); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-80:0','PERc2412{%d}');
    legend(DataSet); ylabel(sprintf('PER, 72 Mbps, RXA at %d MHz', X{1}.Test102_5.fcMHz));
end

if ~isempty(PERd2412{1})
    figure(n+7); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_PER','-99:3','PERd2412{%d}');
    legend(DataSet); ylabel(sprintf('PER, 11 Mbps at %d MHz', X{1}.Test102_3.fcMHz));
    axis([-95 3 1e-4 1]); grid on;
end

if ~isempty(Rx6{1})
    figure(n+8); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','Rx6{%d}');
    DwPhyPlot_SplitBand('FormatY',[-93 -81],-94:-80,[],'Sensitivity, 6 Mbps, RXA');
    legend(DataSet);
end

if ~isempty(Rx54{1})
    figure(n+9); clf;
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','Rx54{%d}');
    DwPhyPlot_SplitBand('FormatY',[-30 0],-40:2:6,[],'Max Input, 54 Mbps, RXA');
    legend(DataSet);
end

%% AnyEmpty
function result = AnyEmpty(S)
result = 0;
for i=1:length(S),
    if isempty(S{i}), result = 1; return; end
end

%% GetData
function [fcMHz, RxA, RxB, Rx6, Rx54, fcMHzIMR, IMR1, IMR2, PERa2412, PERb2412, PERc2412, PERd2412] = GetData(X)
tests = {'Test103_2', 'Test103_3', 'Test103_5', 'Test101_6'};
check = isfield(X, tests);
if any(check)
    name = tests{min(find(check == 1))};
    eval(sprintf('fcMHz = X.%s.fcMHz;', name));
else
    fcMHz = [];
end

if isfield(X, 'Test103_3') RxA = X.Test103_3.Sensitivity;
else                       RxA = [];                      
end

if isfield(X, 'Test103_2') RxB = X.Test103_2.Sensitivity;
else                       RxB = [];                      
end

if isfield(X, 'Test103_5') Rx6 = X.Test103_5.Sensitivity;
else                       Rx6 = [];
end

if isfield(X, 'Test101_6')
    for i=1:length(X.Test101_6.result)
        Rx54(i) = X.Test101_6.result{i}.Sensitivity;
    end
else                       
    Rx54 = [];
end

if isfield(X, 'Test107_1')
    fcMHzIMR = X.Test107_1.fcMHz;
    if length(size(X.Test107_1.IMRdB)) == 3,
        IMR1  = min(50, mean(X.Test107_1.IMRdB(:,:,1),2));
        IMR2  = min(50, mean(X.Test107_1.IMRdB(:,:,2),2));
    else
        IMR1  = min(50, X.Test107_1.IMRdB(:,1));
        IMR2  = min(50, X.Test107_1.IMRdB(:,2));
    end
else
    IMR1 = []; IMR2 = []; fcMHzIMR = [];
end

if isfield(X, 'Test102_2') PERa2412 = max(1e-5, X.Test102_2.PER);
else                       PERa2412 = []; 
end

if isfield(X, 'Test106_3') PERb2412 = max(1e-5, X.Test106_3.PER);
else                       PERb2412 = [];
end

if isfield(X, 'Test102_5') PERc2412 = max(1e-5, X.Test102_5.PER);
else                       PERc2412 = [];
end

if isfield(X, 'Test102_3') PERd2412 = max(1e-5, X.Test102_3.PER);
else                       PERd2412 = [];
end
