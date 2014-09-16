function DwPhyPlot_Sensitivity(data, NoPatch)
% DwPhyPlot_Sensitivity(data)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<2, NoPatch = 0; end

[X,N] = DwPhyPlot_LoadData(data);

Xa = 5:12;          aMbps = [6 9 12 18 24 36 48 54];
Xn = 13:20;         nMbps = [6.5 13 19.5 26 39 52 58.5 65]; MCS = 0:7; 
Xb = 1:4;           bMbps = [1 2 5.5 11];
Xg = [Xb 4.5 Xa];   gMbps = [bMbps aMbps];
Xn5= [Xa Xn];       n5Mbps= [aMbps nMbps];
Xgn= [Xb 4.5 Xa 12.5 Xn]; gnMbps = [bMbps aMbps nMbps];

if any( isfield(X{1},{'Test100_1','Test100_2','Test100_3','Test100_5','Test100_6','Test100_7',...
                      'Test100_11','Test100_12','Test100_13'}) )
    for i=1:N,
        LegendStr{i} = X{i}.PartID;
        if isfield(X{i},'Test100_1')
            fcMHz_a = X{i}.Test100_1.fcMHz;
            Ya{i} =  X{i}.Test100_1.Sensitivity;
            DiversityMode = GetDiversityMode(X{i}.Test100_1);
        end
        if all(isfield(X{i},{'Test100_2','Test100_3'})),
            Yg{i}  = [X{i}.Test100_3.Sensitivity NaN X{i}.Test100_2.Sensitivity];
            fcMHz_g = X{i}.Test100_2.fcMHz;
            DiversityMode = GetDiversityMode(X{i}.Test100_2);
        end
        if all(isfield(X{i},{'Test100_3','Test100_5'})),
            YgA{i}  = [X{i}.Test100_3.Sensitivity NaN X{i}.Test100_5.Sensitivity];
            fcMHz_gA = X{i}.Test100_3.fcMHz;
            DiversityModeA = GetDiversityMode(X{i}.Test100_3);
        end
        if all(isfield(X{i},{'Test100_6','Test100_7'})),
            YgB{i}  = [X{i}.Test100_6.Sensitivity NaN X{i}.Test100_7.Sensitivity];
            fcMHz_gB = X{i}.Test100_6.fcMHz;
            DiversityModeB = GetDiversityMode(X{i}.Test100_6);
        end
        if isfield(X{i},'Test100_5')
            if (length(X{1}.Test100_5.Mbps) == 12) && all(X{1}.Test100_5.Mbps == gMbps) % [SM] to accommodate legacy results from RF52B31
                YgA{i} = [X{i}.Test100_5.Sensitivity(1:4) NaN X{i}.Test100_5.Sensitivity(5:12)];
                XgA = Xg;
            elseif (length(X{1}.Test100_5.Mbps) == 8) && all(X{1}.Test100_5.Mbps == aMbps)
                if isfield(X{i},'Test100_3'),
                    YgA{i} = [X{i}.Test100_3.Sensitivity NaN X{i}.Test100_5.Sensitivity];
                    XgA = Xg;
                else
                    YgA{i} = X{i}.Test100_5.Sensitivity;
                    XgA = Xa;
                end
            else
                error('Unsupported Mbps for Test100_5.');
            end
            fcMHz_g2 = X{i}.Test100_5.fcMHz;
            DiversityModeA = GetDiversityMode(X{i}.Test100_5);
        end
        if isfield(X{i},'Test100_11')
            YnA{i} = [X{i}.Test100_11.Sensitivity];
            fcMHz_n = X{i}.Test100_11.fcMHz;
        end
        if isfield(X{i},'Test100_13'),
            Yn{i} = [X{i}.Test100_13.Sensitivity];
        end
    end
    
elseif isfield(X{1},'Description') && strcmpi(X{1}.Description,'DwPhyTest_Sensitivity')
    for i=1:N,
        M = length(X{i}.Mbps);
        if     (M== 8) && all(X{i}.Mbps == aMbps), Ya{i} = X{i}.Sensitivity; fcMHz_a = X{i}.fcMHz;
        elseif (M== 8) && all(X{i}.Mbps == nMbps), Yn{i} = X{i}.Sensitivity; fcMHz_n = X{i}.fcMHz;
        elseif (M== 4) && all(X{i}.Mbps == bMbps), Yb{i} = X{i}.Sensitivity; fcMHz_b = X{i}.fcMHz;
        elseif (M==12) && all(X{i}.Mbps == gMbps), Yg{i} = X{i}.Sensitivity; fcMHz_g = X{i}.fcMHz;
        elseif (M==16) && all(X{i}.Mbps == n5Mbps),Yn5{i}= X{i}.Sensitivity; fcMHz_n5= X{i}.fcMHz;
        end
    end
    DiversityMode = GetDiversityMode(X{1});
    
elseif isfield(X{1},'Description') && strcmpi(X{1}.Description,'DwPhyTest_SetRxSensitivity')
    [GetRxSensitivity,I] = sort(X{1}.GetRxSensitivity(1,:),'descend');
    gMbps = [bMbps -1 aMbps];
    fcMHz_g = X{1}.fcMHz;
    DiversityMode = GetDiversityMode(X{1});
    [M,N] = size(data.Sensitivity);
    for i = 1:N,
        y = NaN*ones(size(gMbps));
        for j = 1:M,
            y(gMbps == data.Mbps(j)) = data.Sensitivity(j,i);
        end
        Yg{I(i)} = y;
        LegendStr{i} = sprintf('%d dBm',GetRxSensitivity(i));
    end
end
    

%% Plotting    
n = gcf - 1; nInit = n;

if ~isempty(who('Ya')) && ~isempty(Ya{1})
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xa','Ya{%d}');
    axis([4.5 12.5 -94 -68]);
    FormatPlot(fcMHz_a, DiversityMode)
end

if ~isempty(who('Yb'))
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xb','Yb{%d}');
    axis([0.5 4.5 -96 -76]);
    FormatPlot(fcMHz_b, DiversityMode)
end

if ~isempty(who('Yg'))
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xg','Yg{%d}');
    axis([0.5 12.5 -94 -68]);
    FormatPlot(fcMHz_g, DiversityMode);
    DrawLimits(NoPatch);
end

if ~isempty(who('YgA'))
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xg','YgA{%d}');
    axis([0.5 12.5 -94 -68]);
    FormatPlot(fcMHz_gA, DiversityModeA);
    DrawLimits(NoPatch);
end

if ~isempty(who('YgB'))
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xg','YgB{%d}');
    axis([0.5 12.5 -94 -68]);
    FormatPlot(fcMHz_gB, DiversityModeB);
    DrawLimits(NoPatch);
end

if ~isempty(who('Yn')) && ~isempty(Yn{1}) && size(Yn,2) == N,
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xn','Yn{%d}');
    axis([12.5 20.5 -90 -64]);
    FormatPlot(fcMHz_n, DiversityMode);  
    DrawLimits(NoPatch);
end

if ~isempty(who('YgA'))
    n=n+1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','XgA','YgA{%d}');
    axis([0.5 12.5 -94 -68]);
    FormatPlot(fcMHz_g, DiversityModeA);
    DrawLimits(NoPatch);
end

if ~isempty(who('Yn5'))
    n=n+1; figure(n); clf;
    if numel(Xn5) == 16,
        Xn5 = [Xn5(1:8) 12.5 Xn5(9:16)];
        for i=1:length(Yn5),
            Yn5{i} = [Yn5{i}(1:8) NaN  Yn5{i}(9:16)];
        end
    end
    DwPhyPlot_Command(N,'plot','Xn5','Yn5{%d}');
    axis([4.5 20.5 -92 -65]);
    FormatPlot(fcMHz_n5, DiversityMode);
    xlabel('Data Rate (Mbps) | MCS');
    DrawLimits(NoPatch);
end

if ~isempty(who('YnA'))
    n = n + 1; figure(n); clf;
    DwPhyPlot_Command(N,'plot','Xn','YnA{%d}');
    axis([12.5 20.5 -90 -62]);
    FormatPlot(fcMHz_n, 1);
    xlabel('MCS');
    DrawLimits(NoPatch);
end

if numel(who('YgA','YnA')) == 2,
    n = n+1; figure(n); clf;
    for i = 1:numel(YgA),
        YgnA{i} = [YgA{i} NaN YnA{i}];
    end
    DwPhyPlot_Command(N,'plot','Xgn','YgnA{%d}');
    FormatPlot(fcMHz_n, 1);
    xlabel('Data Rate (Mbps) | MCS');
    DrawLimits(NoPatch);
end

if numel(who('Yg','Yn')) == 2,
    n = n+1; figure(n); clf;
    for i = 1:numel(Yg),
        Ygn{i} = [Yg{i} NaN Yn{i}];
    end
    DwPhyPlot_Command(N,'plot','Xgn','Ygn{%d}');
    FormatPlot(fcMHz_n, 3);
    xlabel('Data Rate (Mbps) | MCS');
    DrawLimits(NoPatch);
end

if n == nInit close; end

%% FormatPlot
function FormatPlot(fcMHz, DiversityMode)
grid on;
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 320]);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);

set(gca,'XTick',1:20);
%set(gca,'XTickLabel',{'1','2','5.5','11','6','9','12','18','24','36','48','54','6.5','13','19.5','26','39','52','58.5','65'});
set(gca,'XTickLabel',{'1','2','5.5','11','6','9','12','18','24','36','48','54','0','1','2','3','4','5','6','7'});
xlabel('Data Rate (Mbps)');

set(gca,'YTick',-100:2:0);
ylabelstr{1} = 'Sensitivity, Pr(dBm)';
switch DiversityMode,
    case 1, ylabelstr{2} = sprintf('RXA at %d MHz',fcMHz);
    case 2, ylabelstr{2} = sprintf('RXB at %d MHz',fcMHz);
    case 3, ylabelstr{2} = sprintf('Dual-RX (MRC) at %d MHz',fcMHz);
end
ylabel(ylabelstr);

if evalin('caller', '~isempty(who(''GetRxSensitivity''))'),
    P = get(gcf,'Position'); set(gcf,'Position',[P(1) min(400,P(2)) 620 500]);
    set(gca,'YTick',-100:3:0);
    A = axis; axis([A(1:2) -99 -53]);
end

evalin('caller','if ~isempty(who(''LegendStr'')), legend(LegendStr); end');

% Set the legend position
c = get(gcf,'Children');
if numel(c) > 1,
    Pa = get(gca,'Position'); Pa(3) = 0.62; set(gca,'Position',Pa);
    P = get(c(1),'Position'); 
    P(1) = 0.78;
    P(4) = Pa(4);
    P(2) = Pa(2)+Pa(4) - P(4); % align top of plot and legend
    set(c(1),'Position',P);
end

%% DrawLimits
function DrawLimits(NoPatch)
    X = [-1 -1  2 2   4 4 5 5 6 6 7 7 8 8 9 9 10 10 11 11 12 12 ...
        13 13 14 14 15 15 16 16 17 17 18 18 19 19 20 20 ] + 0.5;
    Y = [ 0 -80 -80 -76 -76 ...
         -82 -82 -81 -81 -79 -79 -77 -77 -74 -74 -70 -70 -66 -66 -65 -65 ...
         -82 -82 -79 -79 -77 -77 -74 -74 -70 -70 -66 -66 -65 -65 -64 -64 0];
if NoPatch,
    hold on;
    plot(X,Y,'r');
    hold off;
else
    A = axis;
    h = patch(X,Y,'y');
    set(h,'FaceAlpha',0.25);
    set(h,'EdgeColor','r');
    axis(A);
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
% 2011-01-01 [SM]: 1. Add GetDiversityMode() to retrieve DiversityMode depending on radio chip. 
%                  2. Add processing for 100.6, 100.7; modify 100.5 for backward compatibility.
