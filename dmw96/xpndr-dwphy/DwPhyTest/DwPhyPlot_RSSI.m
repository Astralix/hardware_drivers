function DwPhyPlot_RSSI(data, AandB)

if nargin<2, AandB = 0; end;

[X, N] = DwPhyPlot_LoadData(data);

if (N == 1) && isfield(X{1},'Description') && strcmp(X{1}.Description,'DwPhyTest_PERvPrdBm')
    if AandB,
        [PrdBm{1}, RSSI{1}, e{1}, fcMHz, RSSI{2}, e{2}] = GetData(X{1}, 1);
        PrdBm{2} = PrdBm{1};
        DataSet = {'RXA','RXB'};
        N = 2;
    else
        [PrdBm{1}, RSSI{1}, e{1}, fcMHz] = GetData(X{1}, AandB);
        DataSet{1} = '';
    end
elseif isfield(X{1}, 'Test108_1')
    for i=1:N,
        [PrdBm{i}, RSSI{i}, e{i}, fcMHz] = GetData(X{i}.Test108_1);  %#ok<NASGU>
        DataSet{i} = X{i}.PartID;
    end
end

if exist('fcMHz', 'var')
    % Draw individual plots...
    figure(1); clf;
    subplot(2,1,1);
    plot(-100:0,0:100,'y'); hold on;
    DwPhyPlot_Command(N,'plot','PrdBm{%d}','RSSI{%d}');
    c = get(gca,'Children'); 
    set(c,'LineWidth',2); set(c,'MarkerSize',14);
    set(c(length(c)),'LineWidth',5);
    axis([-82 -20 18 80]); grid on; ylabel(sprintf('RSSI, %d MHz',fcMHz));
    set(gca,'XTick',-100:5:0);
    set(gca,'XTickLabel',{''});
    set(gca,'Position',[0.13 0.5 0.775 0.45]);

    subplot(2,1,2);
    DwPhyPlot_Command(N,'plot','PrdBm{%d}','e{%d}');
    axis([-82 -20 -5 5]); grid on;
    set(gca,'YTick',-5:5);
    set(gca,'XTick',-100:5:0);
    ylabel('RSSI Error (dB)');
    xlabel('Pr [dBm]');
    c = get(gca,'Children'); 
    set(c,'LineWidth',2); set(c,'MarkerSize',14);
    legend(DataSet);
end

%% GetData
function [PrdBm, RSSI, e, fcMHz, RSSI1, e1] = GetData(X, AandB)
if nargin < 2, AandB = 0; end
fcMHz = X.fcMHz;
PrdBm = X.PrdBm;
RSSI  = X.MeanPacketRSSI;
RSSI = reshape(RSSI,size(PrdBm));
e = (RSSI - (PrdBm+100));
if AandB,
    RSSI  = X.MeanPacketRSSI0; RSSI  = reshape(RSSI,  size(PrdBm));
    RSSI1 = X.MeanPacketRSSI1; RSSI1 = reshape(RSSI1, size(PrdBm));
    e1    = RSSI1 - (PrdBm+100);
end

%% REVISIONS
% 2010-12-31 [SM]: Skip plotting if specified fields are not found.
