function DwPhyPlot_RxTiming(data)
% DwPhyPlot_RxTiming(data) plots results for data = DwPhyTest_RunSuite('RxTiming')

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X, N] = DwPhyPlot_LoadData(data);

%TestName = {'400_1','400_2','400_4','401_1','401_3','401_4','401_5','402_1','402_2','402_3',...
TestName = {'400_1','400_2','400_4','401_1','401_3','401_4','402_1','402_2','402_3',...
    '403_1','403_2','403_3'};

close all;

for i=1:N,
    LegendStr{i} = X{i}.PartID;
end

ResultStr = {'FAIL','PASS'};
ScreenSize = get(0,'ScreenSize');
W = ScreenSize(3); H = ScreenSize(4) - 75;
for i=1:length(TestName),
    if any( isfield(X{1},sprintf('Test%s',TestName{i}))), 
        [x,y,Result,ylabelstr] = GetData(X, TestName{i});
        figure(i); set(gcf,'Position',[W-620-10 H-305-30*(i-1) 620 300]);
        DwPhyPlot_Command(N, 'semilogy','x{%d}','y{%d}'); grid on;
        set(gca,'YMinorGrid','off'); set(gca,'YMinorTick','off');
        xlabel('Pr[dBm]'); ylabel(ylabelstr);
    %    A = axis; A(3) = 1e-4; A(4) = 1; axis(A);
        A = axis; A(3) = 1e-5; A(4) = 1; axis(A);
        set(gca,'YTick',[1e-4 1e-3 1e-2 1e-1 1]);
        set(gca,'YTickLabel',{'0.01%','0.1%','1%','10%','100%'});
        titlestr = sprintf('Test %s: %s',TestName{i},ResultStr{Result+1});
        titlestr(titlestr == '_') = '.';
        title(titlestr); set(gcf,'Name',titlestr);
        c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
        legend(LegendStr);
    %    P = get(gcf,'Position'); P(3) = 620; P(4) = 300; set(gcf,'Position',P); 
        P = get(gca,'Position'); P(1) = 0.16; P(3) = 0.60; set(gca,'Position',P);
        c = get(gcf,'Children');
        set(c(1),'Position',[0.77 0.33 0.21 0.59]);

        for j=1:length(y),
            yMax(j) = max(y{j});
        end
        fprintf('%s: Max PER = %1.2f%%\n',TestName{i},100*max(yMax));
    end
end


%% GetData
function [x,y,Result,ylabelstr] = GetData(X, TestName)
TestName = sprintf('Test%s',TestName);
Result = 1;
for i=1:length(X),
    x{i} = X{i}.(TestName).PrdBm;
    y{i} = max(X{i}.(TestName).PER, 0.5e-4);
    if X{i}.(TestName).Result == 0, Result = 0; end
end
Mbps = X{1}.(TestName).Mbps(length(X{1}.(TestName).Mbps));
fcMHz = X{1}.(TestName).fcMHz;
ylabelstr = {sprintf('PER, %d Mbps',Mbps),sprintf('fcMHz = %d\n',fcMHz)};
