function DwPhyPlot_Test304(data)
% DwPhyPlot_Test304(data) plot result from data = DwPhyTest_RunTest(304.1)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if any( isfield(X{1},{'Test304_1'}) )
    for i=1:N,
        LegendStr{i}  = X{i}.PartID;
        InitAGain{i}  = X{i}.Test304_1.InitAGain;
        Sensitivity{i}= X{i}.Test304_1.Sensitivity;
        Rejection{i}  = X{i}.Test304_1.Rejection_dB;
        fcMHz         = X{i}.Test304_1.fcMHz;
        DiversityMode = GetDiversityMode(X{i}.Test304_1.resultI{1});
    end
elseif any( isfield(X{1},{'Test304_2'}) )
    for i=1:N,
        LegendStr{i}  = X{i}.PartID;
        InitAGain{i}  = X{i}.Test304_2.InitAGain;
        Sensitivity{i}= X{i}.Test304_2.Sensitivity;
        Rejection{i}  = X{i}.Test304_2.Rejection_dB;
        fcMHz         = X{i}.Test304_2.fcMHz;
        DiversityMode = GetDiversityMode(X{i}.Test304_2.resultI{1});
    end
elseif any( isfield(X{1},{'Test304_3'}) )
    for i=1:N,
        LegendStr{i}  = X{i}.PartID;
        InitAGain{i}  = X{i}.Test304_3.InitAGain;
        Sensitivity{i}= X{i}.Test304_3.Sensitivity;
        Rejection{i}  = X{i}.Test304_3.Rejection_dB;
        fcMHz         = X{i}.Test304_3.fcMHz;
        DiversityMode = GetDiversityMode(X{i}.Test304_3.resultI{1});
    end
elseif isfield(X{1},'TestID') && (floor(X{1}.TestID) == 304)
    LegendStr = [];
    N = 1;
    InitAGain{1}   = X{1}.InitAGain;
    Sensitivity{1} = X{1}.Sensitivity;
    Rejection{1}   = X{1}.Rejection_dB;
    fcMHz          = X{1}.fcMHz;
    DiversityMode  = GetDiversityMode(X{1}.resultI{1});
end

if exist('DiversityMode', 'var')
    Plot304(N,InitAGain,Rejection,Sensitivity,LegendStr,fcMHz,DiversityMode);
end

%% PlotEVMvLength
function Plot304(N,x,y1,y2,LegendStr,fcMHz,DiversityMode)
figure(gcf); clf;
for i=1:N,
    t{i} = min(x{i}):0.1:max(x{i});
    P = polyfit(x{i},y1{i},2); z1{i} = polyval(P,t{i});
    P = polyfit(x{i},y2{i},3); z2{i} = polyval(P,t{i});
end
subplot(2,1,1); DwPhyPlot_Command(N,'plot','x{%d}','y1{%d}'); grid on;
set(gca,'Position',[0.13 0.52 0.775 0.39]);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
set(gca,'XTick',0:2:40); set(gca,'XTickLabel',{''});
ylabel({'ACR (dB)','CCK-11 Mbps'});
set(gca,'YTick',0:2:100);
[A(1) A(2)] = GetRange(x, 1);
[A(3) A(4)] = GetRange(y1,1);
A(3) = min([A(3), 34]); A(4) = max([A(4), 36]);
h = patch([0 100 100 0],[0 0 35 35],'y');
set(h,'FaceAlpha',0.25);
set(h,'EdgeColor','r');
axis(A);

if length(LegendStr) == 1,
    title(sprintf('Receiver Dynamic Range at %d MHz: Part %s\n',fcMHz,LegendStr{1}));
else
    title(sprintf('Receiver Dynamic Range at %d MHz\n',fcMHz));
end

subplot(2,1,2); DwPhyPlot_Command(N,'plot','x{%d}','y2{%d}'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
xlabel('InitAGain'); 
if DiversityMode == 1,
    ylabel({'Sensitivity (dBm)','54 Mbps, RXA'});
    h = patch([0 100 100 0],[-65 -65 0 0],'y');
elseif DiversityMode == 2,
    ylabel({'Sensitivity (dBm)','54 Mbps, RXB'});
    h = patch([0 100 100 0],[-65 -65 0 0],'y');
else
    ylabel({'Sensitivity (dBm)','54 Mbps, DualRX-MRC'});
    h = patch([0 100 100 0],[-68 -68 0 0],'y');
end    
set(h,'FaceAlpha',0.25);
set(h,'EdgeColor','r');
[A(3), A(4)] = GetRange(y2,1);
A(3) = min([A(3), -74]); A(4) = max([A(4), -64]); axis(A);
set(gca,'XTick',0:2:40);
set(gca,'YTick',-100:2:0);
if length(LegendStr) > 1,
    legend(LegendStr);
end

%% GetRange
function [xMin,xMax] = GetRange(x, Round2)
xMin =  1e9;
xMax = -1e9;
for i=1:length(x),
    xMin = min([xMin min(x{i})]);
    xMax = max([xMax max(x{i})]);
end
if nargin>1 && Round2,
    xMin = 2 * floor(xMin/2);
    xMax = 2 * ceil (xMax/2);
end

%% GetDiversityMode
function diversity = GetDiversityMode(data)

if DwPhyLab_RadioIsRF22(data)
    if isfield(data, 'DiversityMode') 
        if any(data.DiversityMode == [1 3]) diversity = 1;
        else                                diversity = 2;
        end
    else                              
        diversity = 1;
    end
else
    diversity = data.RegList(3,2);
end

%% REVISIONS
% 2010-12-31 [SM]: 1. Skip plotting if specified fields are not found.
%                  2. Add GetDiversityMode() to retrieve DiversityMode depending on radio chip. 
%                  3. Add processing for Test304_3 