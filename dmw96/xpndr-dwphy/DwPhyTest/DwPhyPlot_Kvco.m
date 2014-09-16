function DwPhyPlot_Kvco(data,CalLimits,OverlapPlot,PlotLO)

if nargin<2, CalLimits=[]; end;
if nargin<3, OverlapPlot=0; end;
if nargin<4, PlotLO=0; end;

PartID = [];

if ischar(data),
    S = load(data);
    data = S.data.Test205_1;
    PartID = S.data.PartID;
end
N = length(data.CAPSEL);

figure(gcf); clf;
if     N <= 3, set(gcf,'Position',[20 600 620 320]);
elseif N <= 7, set(gcf,'Position',[20 600 620 400]);
elseif N <=11, set(gcf,'Position',[20 600 620 500]); 
elseif N <=20, set(gcf,'Position',[20 350 620 700]); 
else           set(gcf,'Position',[20 180 620 900]); 
end

f = data.f'/1e6;
if PlotLO,
    if data.fcMHz < 3000, f = f*2/3;
    else                  f = f*3/4;
    end
end

plot(data.VCOREFDAC,f,'.-'); grid on; hold on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
xlabel('VCOREFDAC'); 
if ~isempty(PartID) title(PartID); end
if PlotLO, ylabel('LO Frequency (MHz)'); else ylabel('Channel (MHz)'); end
legendstr = cell(1,N);
for i=1:N, legendstr{i} = sprintf('CAPSEL = %d',data.CAPSEL(i)); end
legend(legendstr);
c = get(gcf,'Children');
P = get(c(1),'Position'); P(1) = 0.75;
set(c(1),'Position',P);
A = axis; A(2)=32; axis(A);
P = get(gca,'Position'); P(3)=0.6; set(gca,'Position',P);
if ~isempty(CalLimits),
    plot(CalLimits(1)*[1 1]-0.2*32,[0 1e4],'r--');
    plot(CalLimits(2)*[1 1]       ,[0 1e4],'r--');
end

if data.SweepByBand,
    if data.fcMHz < 3000,
        if isfield(data,'TestAllCAPSEL') && data.TestAllCAPSEL,
            axis([0 32 2200 3140]);
        else
            axis([0 32 2370 2520]);
        end
        set(gca,'YTick',1000:20:5000);
    else
        if isfield(data,'TestAllCAPSEL') && data.TestAllCAPSEL,
            axis([0 32 4400 6300]);
        else
            axis([0 32 4900 5900]);
        end
        set(gca,'YTick',1000:100:7000);
    end
else
    plot([0 32],data.fcMHz*[1 1],'k-');
    YTick = get(gca,'YTick'); 
    dYTick = round(mean(diff(YTick)));
    YTick = data.fcMHz + dYTick*(-10:10);
    set(gca,'YTick',YTick);
end
set(gca,'XTick',0:4:32);

% Replot CAPSEL curves so they overlap the reference lines
plot(data.VCOREFDAC,data.f'/1e6,'.-');
hold off;

%% CREATE OVERLAP PLOT
if OverlapPlot,
    figure(gcf+1); clf;
    X = zeros(2,N);
    Y = ones(2,N);
    f = data.f/1e6;
    for i=1:N,
        X(1,i) = GetFreq(data.VCOREFDAC, f(i,:), CalLimits(1)-(0.2*31));
        X(2,i) = GetFreq(data.VCOREFDAC, f(i,:), CalLimits(2));
        Y(:,i) = data.CAPSEL(i)*[1 1]';
    end
    for i=2:N,
        y = [Y(1,i-1) Y(1,i)];
        x1 = X(1,i-1);
        x2 = X(2, i );
        if x1 < x2,
            plot(x1*[1 1],y,'r-', x2*[1 1],y,'r-'); 
        end
    end
    plot(data.fcMHz*[1 1],[0 64],'g',X,Y,'b-'); grid on;
    xlabel('Channel Frequency (MHz)'); ylabel('CAPSEL');
    title(sprintf('VCO Tuning Range with DACLO = %d, DACHI = %d',CalLimits));
    YTick = get(gca,'YTick');
    if mean(diff(YTick)) < 1, 
        set(gca,'YTick',0:31);
    end
    A = axis; axis([A(1) A(2) min(data.CAPSEL)-0.5 max(data.CAPSEL)+0.5]);
    hold on;
    for i=2:N,
        y = [Y(1,i-1) Y(1,i)];
        y1 = Y(1,i-1);
        y2 = Y(1, i );
        x1 = X(1,i-1);
        x2 = X(2, i );
        if x1 < x2,
%            plot(x1*[1 1],y,'r-', x2*[1 1],y,'r-'); 
            xx = [x1 x1 x2 x2]; yy = [y1 y2 y2 y1];
            patch(xx,yy,'y');
            c = get(gca,'Children'); set(c(1),'EdgeColor',[1 0 0]);
        end
    end
    plot(X,Y,'b');
    c = get(gca,'Children'); set(c(1:N),'LineWidth',5);
    hold off;
end
    
%% DISPLAY OVERLAP INFORMATION
Np = 2;
if N>2 && ~isempty(CalLimits),
    x = data.VCOREFDAC;
    for i=2:(N-1),
        f1 = GetFreq(x,data.f(i,:),CalLimits(2));
        f2 = GetFreq(x,data.f(i,:),CalLimits(1)-(0.2*31));
        Vtune1 = GetVTune(data.VCOREFDAC,data.f(i-1,:),f1);
        Vtune2 = GetVTune(data.VCOREFDAC,data.f(i+1,:),f2);
        Vm1 = Vtune1 - (CalLimits(1)/31+0.2);
        Vm2 = (CalLimits(2)/31+0.4) - Vtune2;
        m1 = Vm1*31;
        m2 = Vm2*31;        
        DACLO = floor( 31*(Vtune1-0.2) );
        DACHI = floor( 31*(Vtune2-0.4) );
        fprintf('CAPSEL = %2u, f=%1.1f MHz: [Vtune = %1.2fV (%2u)], [Vtune = %1.2fV (%2u), Margins = (%1.1f,%1.1f)\n',data.CAPSEL(i),f1/1e6,Vtune1,DACLO,Vtune2,DACHI,m1,m2);
    end
end


%% GetFreq
function f0 = GetFreq(VCOREFDAC,f,x0)
f0 = interp1(VCOREFDAC,f,x0,'cubic','extrap');


%% GetVTUNE
function Vtune = GetVTune(VCOREREFDAC,f,f0)
x = 0.4 + VCOREREFDAC/31;
k0 = find(f>=f0, 1, 'first');
if isempty(k0) || (k0==1),
    Vtune = NaN;
else
    k = k0 + [-1 0];
    P = polyfit(x(k),f(k)-f0,1);
    Vtune = roots(P);
end
