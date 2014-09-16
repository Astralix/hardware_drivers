function DwPhyPlot_InterferenceRejection(data, NoPatch)

if nargin<2, NoPatch = 0; end

[X,N] = DwPhyPlot_LoadData(data);

for i=1:N,
    if isfield(X{i},'PartID')
        PartID = X{i}.PartID;
    else
        PartID = [];
    end
    [f2, f5, r5a{i}, r5b{i}, r2a{i}, r2b{i}] = GetData(X{i});
    LegendStr{i} = X{i}.PartID;
end

if ~isempty(f5) && ~isempty(r5a{1}),
    PlotIRR(1, N, f5, r5a, 15, NoPatch);
    legend(LegendStr);
    ylabel({'Alternate Adjacent Channel Rejection (dB)','54 Mbps at Pr = -62 dBm, fi = fc \pm 40 MHz'});
    axis([4900 5900 10 26]);

    PlotIRR(2, N, f5, r5b, -1, NoPatch);
    legend(LegendStr);
    ylabel({'Adjacent Channel Rejection (dB)','54 Mbps at Pr = -62 dBm, fi = fc \pm 20 MHz'});
    axis([4900 5900 -2 14]);
end

if ~isempty(f2)
    PlotIRR(3, N, f2, r2a, -1, NoPatch);
    legend(LegendStr);
    ylabel({'Adjacent Channel Rejection (dB)','54 Mbps at Pr = -62 dBm, fi = fc \pm 25 MHz'});
    axis([2400 2500 -2 22]);

    PlotIRR(4, N, f2, r2b, 35, NoPatch);
    legend(LegendStr);
    ylabel({'Adjacent Channel Rejection (dB)','11 Mbps at Pr = -70 dBm, fi = fc \pm 25 MHz'});
    axis([2400 2500 30 45]);
end

%% PlotIRR
function PlotIRR(n, N, f, IRR, Limit, NoPatch) %#ok<INUSL>
figure(n); clf;
eval( DwPhyPlot_Command(N,'plot','f','IRR{%d}') );

P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 320]);
c = get(gca,'Children');
set(c,'LineWidth',1.5);
set(c,'MarkerSize',18);
grid on;
xlabel('Channel Frequency (MHz)'); 
ylabel('Interference Rejection (dB)');

if ~NoPatch
    A = axis;
    h = patch([0 10000 10000 0],[-100 -100 Limit Limit],'y');
    set(h,'FaceAlpha',0.25);
    set(h,'EdgeColor','r');
    axis(A);
end

%% GetData
function [f2,f5,r5a,r5b,r2a,r2b] = GetData(X)

if isfield(X, 'Test104_2')
    f2 = X.Test104_2.fcMHz;
    for i=1:length(X.Test104_2.result),
        r2a(i) = min(X.Test104_2.result{i}.Rejection_dB);
        r2b(i) = min(X.Test104_3.result{i}.Rejection_dB);
    end
else
   f2 = []; r2a = []; r2b = []; 
end

if isfield(X, 'Test104_1')
    f5 = X.Test104_1.fcMHz;
    x = X.Test104_1.result;
    for i=1:length(x),
        y = x{i}.Rejection_dB;
        r5a(i) = min(y([1 4]));
        r5b(i) = min(y([2 3]));
    end
else
    f5 = []; r5a = []; r5b = [];
end
    
%% REVISIONS
% 2010-12-31 [SM]: Check if Test104_2 exists.
