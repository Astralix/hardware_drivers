function DwPhyPlot_TxPwrVsPADGAIN(data)
% DwPhyPlot_TxPwrVsPADGAIN(data) plots the results from data = DwPhyTest_RunTest(207.1)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if any(isfield(X{1},{'Test207_1'})),
    for i=1:N,
        LegendStr{i} = X{i}.PartID;
    end
    for n=1:1,
        fieldname = sprintf('Test207_%d',n);
        if isfield(X{1}, fieldname)
            figure(n);
            PlotTxPower(LegendStr, X, sprintf('Test207_%d',n));
        end
    end
else
    figure(1);
    PlotTxPower(X)
end

%% PlotPvTXPWRLVL
function PlotTxPower(LegendStr, X, fieldname)
if nargin>2,
    N = length(X);
    for i=1:N,
        x{i} = X{i}.(fieldname).PADGAINO;
        y{i} = X{i}.(fieldname).Pout_dBm(:,1);
        dx{i} = x{i}(2:length(x{i}));
        dy{i} = diff(y{i});
    end
    fcMHz = X{1}.(fieldname).fcMHz;
else
    N = 1; i=1;
    x{i} = X{i}.PADGAINO;
    y{i} = X{i}.Pout_dBm(:,1);
    dx{i} = x{i}(2:length(x{i}));
    dy{i} = diff(y{i});
    fcMHz = X{1}.fcMHz;
end
figure(gcf); clf; 
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2)-300 620 680]);
subplot(2,1,1);
DwPhyPlot_Command(N, 'plot','x{%d}','y{%d}'); grid on;
set(gca,'Position',[0.13 0.49 0.78 0.45]);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
axis([164 254 -7 27]);
set(gca,'XTick',2:6:256);
set(gca,'YTick',-30:3:40);
set(gca,'XTickLabel',{''});
ylabel(sprintf('Pout (dBm), %d MHz',fcMHz));
legend(LegendStr);

subplot(2,1,2);
DwPhyPlot_Command(N, 'plot','dx{%d}','dy{%d}'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',14);
axis([164 254 0 2]);
set(gca,'XTick',2:6:256);
set(gca,'YTick',0:0.2:2);
xlabel('PADGAIN'); ylabel('Gain Step (dB)');

