function DwPhyPlot_TxEVMvPADGAIN(data, NoPatch)

if nargin<2, NoPatch = 0; end;

% If data is a filename, load the data structure...
if ischar(data)
    S = load(data);
    data = S.data;
end

% Retrieve the PartID...
if isfield(data,'PartID')
    PartID = data.PartID;
else
    PartID = [];
end

% Determine type of data provided and plot accordingly...
if isfield(data,'Test210_1')
    if isfield(data,'Test211_1')
        kMask = data.Test211_1.k;
    else
        kMask = [];
    end
    PlotTest210(gcf, data.Test210_1, kMask, data.PartID, NoPatch);
elseif isfield(data,'Test213_1')
    PlotTest210(gcf, data.Test213_1, data.Test213_1.k, data.PartID, NoPatch);
elseif isfield(data,'Description') && strcmpi(data.Description,'DwPhyTest_TxEVMvRegister')
    if isfield(data,'k'), k=data.k; else k=[]; end
    PlotTest210(gcf, data, k, [], NoPatch);
else
    error('Unrecognized data type');
end

%% PlotTest210
function PlotTest210(n, data, kMask, PartID, NoPatch)

if nargin<3, kMask = []; end
if nargin<4, PartID = []; end

figure(n); clf;
x = data.RegRange;
y = data.AvgEVMdB;
p = data.Pout_dBm;
plotyy(x,p,x,y); grid on;
c = get(gcf,'Children');
for i=1:2,
    set(c(i),'Position',[0.12 0.11 0.775 0.815]);
end
c1 = get(c(1),'Children');
c2 = get(c(2),'Children');
set(c1,'Marker','.'); set(c1,'MarkerSize',18); set(c1,'LineWidth',2);
set(c2,'Marker','.'); set(c2,'MarkerSize',18); set(c2,'LineWidth',2);
minX = max( [min(x) 192] );
maxX = min( [max(x) 248] );
axis(c(1), [minX maxX -30 -15]);
axis(c(2), [minX maxX 8 23]);
%set(c(1),'YColor','r'); set(c1,'Color','r');
set(c(1),'XTick',192:8:254); set(c(1),'YTick',-29:2:-15);
set(c(2),'XTick',192:8:254); set(c(2),'YTick', 1:2:30);
xlabel('PADGAIN[7:0]'); ylabel(c(1),'EVM (dB)'); ylabel(c(2),'Pout (dBm)');

% axes(c(1)); hold on; 
% plot([0 256],-25*[1 1],'r');
% hold off; axes(c(2));

if ~isempty(kMask),
    if ~NoPatch,
        h = patch ([(x(kMask)+1)*[1 1] 256 256],[-40 40 40 -40],[1 1 1 1],'y');
        set(h,'FaceAlpha',0.25);
        set(h,'EdgeColor','r');
    else
        hold on;
        plot((x(kMask)+1) * [1 1],[-40 40],'r');
    end
end

if isempty(PartID)
    title( sprintf('%d MHz, TXPWRLVL = %d',data.fcMHz, data.TXPWRLVL) );
else
    title( sprintf('%s at %d MHz, TXPWRLVL = %d',PartID, data.fcMHz, data.TXPWRLVL) );
end
