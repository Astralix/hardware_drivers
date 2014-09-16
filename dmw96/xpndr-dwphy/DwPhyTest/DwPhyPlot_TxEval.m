function DwPhyPlot_TxEval(data)
% DwPhyPlot_TxEval(data) plot result from data = DwPhyTest_RunSuite('TxEval')
% DwPhyPlot_TxEval(file) plot data from the specified file
% DwPhyPlot_TxEval(filelist) plot data from the list of files

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

for i=1:N,
    LegendStr{i} = X{i}.PartID;
    if isempty(LegendStr{i}) LegendStr{i} = ''; end;
    [fcMHz{i}, P{i}, TXPWRLVL, x24{i}, y24{i}, f24, x50{i}, y50{i}, f50] = GetData(X{i});
    PrintPhaseNoise(X{i});
end

if ~isempty(fcMHz{1})
    figure(1);
    DwPhyPlot_Command(N,'DwPhyPlot_SplitBand','fcMHz{%d}','P{%d}');
    %DwPhyPlot_SplitBand('FormatY',[6 22],6:2:22,[], ...
        DwPhyPlot_SplitBand('FormatY',[14 24],14:2:24,[], ...    
        sprintf('Pout (dBm), TXPWRLVL = %d',TXPWRLVL));
    legend(LegendStr);
end

if ~isempty(x24{1})
    figure(2);
    PlotEVMvPout(N, x24, y24, f24);
    legend(LegendStr);
end

if ~isempty(x50{1})
    figure(3);
    PlotEVMvPout(N, x50, y50, f50);
    legend(LegendStr);
end

if isfield(X{1},'Test206_1'),
    for i=1:N,
        x{i} = X{i}.Test206_1.LengthPSDU;
        y{i} = X{i}.Test206_1.AvgEVMdB;
    end
    figure(4); 
    PlotEVMvLength(N,x,y, X{1}.Test206_1.TXPWRLVL, X{1}.Test206_1.fcMHz)
    legend(LegendStr);
end

if isfield(X{1},'Test206_2'),
    for i=1:N,
        x{i} = X{i}.Test206_2.Mbps;
        y{i} = X{i}.Test206_2.AvgEVMdB;
    end
    figure(5); 
    PlotEVMvRate(N,x,y, X{1}.Test206_2.TXPWRLVL, X{1}.Test206_2.fcMHz)
    legend(LegendStr);
end

if isfield(X{1},'Test206_3'),
    for i=1:N,
        x{i} = 0:7;
        Y{i} = X{i}.Test206_3.AvgEVMdB;
    end
    figure(6);
    PlotEVMvMCS(N,x,y, X{1}.Test206_2.TXPWRLVL, X{1}.Test206_2.fcMHz);
    legend(LegendStr);
end


%% PlotEVMvLength
function PlotEVMvLength(N,x,y,TXPWRLVL,fcMHz)
figure(gcf); clf;
DwPhyPlot_Command(N,'plot','x{%d}','y{%d}');
xlabel('PSDU Length (Bytes)'); 
ylabel({sprintf('EVM (dB) at %d MHz',fcMHz),sprintf('TXPWRLVL = %d',TXPWRLVL)});
axis([0 1600 -31 0]); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
h = patch([0 4096 4096 0],[0 0 -25 -25],'y');
set(h,'FaceAlpha',0.25);
set(h,'EdgeColor','r');


%% PlotEVMvRate
function PlotEVMvRate(N,x,y,TXPWRLVL,fcMHz)
figure(gcf); clf;
DwPhyPlot_Command(N,'plot','x{%d}','y{%d}');
xlabel('L-OFDM Data Rate (Mbps)');
ylabel({sprintf('EVM (dB) at %d MHz',fcMHz),sprintf('TXPWRLVL = %d',TXPWRLVL)});
axis([5 56 -31 0]); grid on;
set(gca,'XTick',[6 9 12 18 24 36 48 54]);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
hX = [-1 -1 7.5 7.5 10.5 10.5  15  15  21  21  30  30  42  42  51  51  100 100];
hY = [ 0 -5  -5  -8   -8  -10 -10 -13 -13 -16 -16 -19 -19 -22 -22 -25  -25   0];
h = patch(hX,hY,'y');
set(h,'FaceAlpha',0.25);
set(h,'EdgeColor','r');

%% PlotEVMvMCS
function PlotEVMvMCS(N,x,y,TXPWRLVL,fcMHz)
figure(gcf); clf;
DwPhyPlot_Command(N,'plot','x{%d}','y{%d}');
xlabel('HT-OFDM MCS');
ylabel({sprintf('EVM (dB) at %d MHz',fcMHz),sprintf('TXPWRLVL = %d',TXPWRLVL)});
axis([-0.5 7.5 -31 0]); grid on;
set(gca,'XTick',0:7);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
hX = [-1 -1   1    1   2   2   3   3   4   4   5   5   6   6   7   7  99 99] - 0.5;
hY = [ 0 -5  -5  -10 -10 -13 -13 -16 -16 -19 -19 -22 -22 -25 -25 -28 -28  0];
h = patch(hX,hY,'y');
set(h,'FaceAlpha',0.25);
set(h,'EdgeColor','r');

%% PlotEVMvPout
function PlotEVMvPout(N, x, y, fcMHz)
if ~isempty(x) && ~isempty(y)
    figure(gcf); clf;
    DwPhyPlot_Command(N,'plot','x{%d}','y{%d}');
    A = axis;
    if max(y{1}) < -19,
        axis([3 23 -31 -19]); grid on;
    elseif max(y{1}) < -17,
        axis([3 23 -31 -17]); grid on;
    else
        axis([0 24 -31 -13]); grid on;
    end
    xlabel('Pout (dBm)'); ylabel(sprintf('EVM (dB), %d MHz',fcMHz));
    c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
    set(gca,'YTick',-39:2:0); set(gca,'XTick',0:2:30);
end

%% GetData
function [fcMHz P, TXPWRLVL, P24,EVM24,fcMHz24, P50,EVM50,fcMHz50] = GetData(X)
if isfield(X,'Test200')
    k = find(X.Test200.TXPWRLVL == 63);
    fcMHz = X.Test200.fcMHz;
    P     = X.Test200.Pout_dBm(:,k);
    TXPWRLVL = 63;
elseif isfield(X,'Test200_1')
    k = find(X.Test200_1.TXPWRLVL == 63);
    fcMHz = X.Test200_1.fcMHz;
    P     = X.Test200_1.Pout_dBm(:,k);
    TXPWRLVL=X.Test200_1.TXPWRLVL(k);
else
    fcMHz = []; P = []; TXPWRLVL = [];
end
if isfield(X,'Test204_1')
    P24     = X.Test204_1.Pout_dBm;
    EVM24   = X.Test204_1.AvgEVMdB;
    fcMHz24 = X.Test204_1.fcMHz;
else
    P24 = []; EVM24 = []; fcMHz24 = [];
end
if isfield(X,'Test204_2')
    P50     = X.Test204_2.Pout_dBm;
    EVM50   = X.Test204_2.AvgEVMdB;
    fcMHz50 = X.Test204_2.fcMHz;
else
    P50 = []; EVM50 = []; fcMHz50 = [];
end


%% PrintPhaseNoise
function PrintPhaseNoise(X)
if all( isfield(X,{'Test203_1','Test203_2'}) ),
    n2 = 3;
    n5 = 3;
    fprintf('% 4s: 2412 MHz(%1.1f, %1.1f); 5200 MHz (%1.1f, %1.1f), CFO = %1.1f ppm\n',X.PartID, ...
        X.Test203_1.result{n2}.PhaseNoise_dBc, X.Test203_1.result{n2}.IQ_dBc, ...
        X.Test203_2.result{n5}.PhaseNoise_dBc, X.Test203_2.result{n5}.IQ_dBc, ...
        X.Test203_1.result{n5}.CFOppm);
end

%% REVISIONS
% 2010-02-25 Add null LegendStr if PartID is not available; define TXPWRLVL = 63 for
%            Test200.
% 2011-01-02 [SM] Add null TXPWRLVL and return fcMHz in GetData().