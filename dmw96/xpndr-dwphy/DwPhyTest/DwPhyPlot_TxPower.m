function DwPhyPlot_TxPower(data)
% DwPhyPlot_TxPower(data) plots the results from data = DwPhyTest_TxPower
% DwPhyPlot_TxPower(file) plots the data in the specified file
% DwPhyPlot_TxPower(filelist) plot data from the list of files

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if any(isfield(X{1},{'Test201_1','Test201_2','Test201_3','Test201_4','Test201_5'})),
    for i=1:N,
        LegendStr{i} = X{i}.PartID;
    end
    for n=1:5,
        fieldname = sprintf('Test201_%d',n);
        if isfield(X{1}, fieldname)
            figure(n);
            PlotTxPower(X, sprintf('Test201_%d',n));
            legend(LegendStr);
            c = get(gcf,'Children'); set(c(1),'Position',[0.766 0.137 0.131 0.437]);
        end
    end
elseif isfield(X{1},'Description') && strcmpi(X{1}.Description,'DwPhyTest_TxPower')
    figure(1);
    PlotTxPower(X)
end

%% PlotPvTXPWRLVL
function PlotTxPower(X, fieldname)
if nargin>1,
    N = length(X);
    for i=1:N,
        fcMHz = X{i}.(fieldname).fcMHz;
        x{i}  = X{i}.(fieldname).TXPWRLVL;
        y{i}  = X{i}.(fieldname).Pout_dBm;
    end
else
    N = 1;
    fcMHz = X{1}.fcMHz;
    x{1}  = X{1}.TXPWRLVL;
    y{1}  = X{1}.Pout_dBm;
end
figure(gcf); clf;
DwPhyPlot_Command(N, 'plot','x{%d}','y{%d}'); grid on;
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 360]);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
if min(x{1}) >= 32,
    axis([32 64 2 22]);
else
    axis([0 64 0 24]);
end
set(gca,'XTick',0:4:64);
set(gca,'YTick',-20:2:40);
xlabel('TXPWRLVL'); ylabel(sprintf('Pout (dBm), %d MHz',fcMHz));

%% REVISIONS
% 2010-12-31 [SM]: Checked 'Description' before plotting.
