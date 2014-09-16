function DwPhyPlot_TxPwrVsDACOFFSET(data)
% DwPhyPlot_TxPwrVsDACOFFSET(data) plots the results from data = DwPhyTest_RunTest(208.1)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if any(isfield(X{1},{'Test208_1'})),
    LegendStr = {N,1};
    for i=1:N,
        LegendStr{i} = X{i}.PartID;
    end
    for n=1:1,
        fieldname = sprintf('Test208_%d',n);
        if isfield(X{1}, fieldname)
            figure(3*n-2);
            PlotTxPower(LegendStr, X, sprintf('Test208_%d',n));
            figure(3*n-1);
            PlotPCOUNT (LegendStr, X, sprintf('Test208_%d',n));
            figure(3*n);
            PlotPowerDifference(LegendStr, X, sprintf('Test208_%d',n));
        end
    end
else
    figure(1);
    PlotTxPower(X)
end

%% PlotTxPower
function PlotTxPower(LegendStr, X, fieldname)
if nargin>2,
    N = length(X);
    x = {N,1}; y=x; dx=x; dy=x;
    for i=1:N,
        x{i} = X{i}.(fieldname).DACOFFSET;
        y{i} = X{i}.(fieldname).Pout_dBm(:,1);
        dx{i} = x{i}(2:length(x{i}));
        dy{i} = diff(y{i});
    end
    fcMHz = X{1}.(fieldname).fcMHz;
else
    N = 1; i=1;
    x{i} = X{i}.DACOFFSET;
    y{i} = X{i}.Pout_dBm(:,1);
    dx{i} = x{i}(2:length(x{i}));
    dy{i} = diff(y{i});
    fcMHz = X{1}.fcMHz;
end
figure(gcf); clf; 
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 350]);
DwPhyPlot_Command(N, 'plot','x{%d}','y{%d}'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
axis([32 120 6 22]);
set(gca,'XTick',0:8:256);
set(gca,'YTick',0:2:40);
xlabel('DACOFFSET'); ylabel({'Pout (dBm)',sprintf('TXPWRLVL = 63, %d MHz',fcMHz)});
legend(LegendStr);


%% PlotPCOUNT
function PlotPCOUNT(LegendStr, X, fieldname)
if nargin>2,
    N = length(X);
    for i=1:N,
        x{i} = X{i}.(fieldname).DACOFFSET;
        y{i} = X{i}.(fieldname).result63.ReadPCOUNT;
    end
    fcMHz = X{1}.(fieldname).fcMHz;
else
    N = 1; i=1;
    x{i} = X{i}.DACOFFSET;
    y{i} = X{i}.result63.ReadPCOUNT;
    fcMHz = X{1}.fcMHz;
end
figure(gcf); clf; 
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 350]);
DwPhyPlot_Command(N, 'plot','x{%d}','y{%d}'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
axis([32 120 80 120]);
set(gca,'XTick',0:8:256);
set(gca,'YTick',0:2:260);
xlabel('DACOFFSET'); ylabel({'PCOUNT',sprintf('TXPWRLVL = 63, %d MHz',fcMHz)});
legend(LegendStr);

%% PlotPowerDifference
function PlotPowerDifference(LegendStr, X, fieldname)
if nargin>2,
    N = length(X);
    x = {N,1}; y=x; dx=x; dy=x;
    for i=1:N,
        x{i} = X{i}.(fieldname).DACOFFSET;
        y{i} = X{i}.(fieldname).Pout_dBm(:,1);
    end
    fcMHz = X{1}.(fieldname).fcMHz;
else
    N = 1; i=1;
    x{i} = X{i}.DACOFFSET;
    y{i} = X{i}.Pout_dBm(:,1);
    fcMHz = X{1}.fcMHz;
end

Sy = zeros(size(y{1}));
for i = 1:N,
    Sy = Sy + 10.^(y{i}/10);
end
yAvg = 10*log10(Sy/N);
for i = 1:N,
    dy{i} = y{i} - yAvg;
end

figure(gcf); clf; 
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 350]);
DwPhyPlot_Command(N, 'plot','x{%d}','dy{%d}'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
axis([32 120 -3 3]);
set(gca,'XTick',0:8:256);
set(gca,'YTick',-10:10);
xlabel('DACOFFSET'); ylabel({'Deviation from Average Power (dB)',sprintf('TXPWRLVL = 63, %d MHz',fcMHz)});
legend(LegendStr);

