function DwPhyPlot_PERvPrdBm(data)
% DwPhyPlot_PERvPrdBm(data) - Plot data from DwPhyTest_PERvPrdBm
% DwPhyPlot_PERvPrdBm(file) - Plot data from specified data file
% DwPhyPlot_PERvPrdBm(filelist) - Plot data from the specified list of data files

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if any(isfield(X{1},{'Test102_1','Test102_2','Test102_3','Test102_4','Test102_5'})),
    for i=1:N,
        LegendStr{i} = X{i}.PartID;
        GetStats(X{i});
    end
%    for n=2:5,
    for n=[2, 3, 5, 9],
        fieldname = sprintf('Test102_%d',n);
        figure(n); PlotPERvPrdBm(X, fieldname);
        legend(LegendStr);
    end
else
    figure(2); PlotRSSI(X);
    figure(1); PlotPERvPrdBm(X);
end

%% PlotPERvPrdBm
function PlotPERvPrdBm(X, fieldname)
if nargin<2,
    [x,y{1},z,fcMHz{1},Mbps{1}] = GetData(X{1});
else
    for i=1:length(X),
        [x,y{i},z,fcMHz{i},Mbps{i}] = GetData(X{i}.(fieldname));
    end
end    
if isfield(X{1},'DiversityMode'), DiversityMode=X{1}.DiversityMode; else DiversityMode=3; end
figure(gcf); clf;
DwPhyPlot_Command(length(X),'DwPhyPlot_PER','x','y{%d}');
ylabelstr{1} = sprintf('PER, %d Mbps',Mbps{1});
ylabelstr{2} = sprintf('%d MHz, DiversityMode = %d',fcMHz{1},DiversityMode);
ylabel(ylabelstr);
axis([-90 0 1e-4 1]);


%% PlotRSSI
function PlotRSSI(X)
[x,y,z,fcMHz,Mbps] = GetData(X{1});
figure(gcf); clf;
plot([-100 0],[0 100],'k'); hold on;
plot(x,z,'r.-');
c = get(gca,'Children'); set(c,'LineWidth',2'); set(c,'MarkerSize',18);
xlabel('Pr[dBm]'); ylabel(sprintf('RSSI, %d MHz',fcMHz));
axis([-90 0 0 100]);
grid on;

%% GetData
function [x,y,z,fcMHz,Mbps] = GetData(data)
x = data.PrdBm;
y = data.PER;
y = max(y, 0.5/data.MaxPkts); % set floor for plotting aestetics
z = data.RSSI;
z((data.RSSI == 0) & (data.PER == 1)) = NaN;
fcMHz = data.fcMHz;
Mbps = data.Mbps;

%% GetStats
function GetStats(X)
fprintf('% 4s> CFO=%2.1f ppm, StepLNA = %3.1f,%3.1f; %3.1f,%3.1f dB\n', ...
    X.PartID, X.Test13_1.CFOppm, ...
    X.Test300_2.StepLNA0dB(1, 1), X.Test300_2.StepLNA1dB( 1,1), ...
    X.Test300_2.StepLNA0dB(14,1), X.Test300_2.StepLNA1dB(14,1) ...
    );
%    X.Test300_2.StepLNA0dB(25,1), X.Test300_2.StepLNA1dB(25,1) ...