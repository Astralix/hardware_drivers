function [outS,outY] = mexPlotMojave_Startup;

[S,Y] = LoadData;

if(ishandle(1))
    figure(gcf); clf;
else
    figure(1); set(gcf,'Position',[5 375 1250 750]);
end

k80 = 1:length(S.traceRxState);
k40 = 1:length(S.c);
k20 = 1:length(S.y);

dt = 50.55;
dt = 100.55;
t20=k20/20-dt; Y.t20 = t20;
t40=k40/40-dt; Y.t40 = t40;
t80=k80/80-dt; Y.t80 = t80;
Y.t22 = Y.t80(Y.bRX.k80+1);

if(nargout>0) outS=S; outY=Y; end

p1=0.09;
p3=0.88;
p4=0.27;

nPlot=3;
subplot(nPlot,1,1); plot(t40,real(S.r(:,1)),t40,real(S.c(:,1)),'r',t20,real(S.y(:,1)),'g'); grid on; 
legend('ADC Output','DCX Output','Downsample Output');
A = axis; 
SetLimit = 0;
SetLimit = 1; t1 = -30; t2=12;
if(~SetLimit) t1=A(1); end; A(1) = t1;
if(~SetLimit) t2=A(2); end; A(2) = t2; 
A(3) = -600; A(4) = 600; axis(A);
XTick = get(gca,'XTick');
XTick = -30:2:120; 
set(gca,'XTick',XTick); set(gca,'XTickLabel',{''});

subplot(nPlot,1,2); plot(t80,Y.State,t80,Y.PktTime,'r'); grid on; axis([t1 t2 0 20]); legend('State','PktTime');
set(gca,'YTick',0:2:40);
set(gca,'XTick',XTick); set(gca,'XTickLabel',{''});
ThickLines;

N =length(Y.aSigOut.ValidRSSI); k = 2:N;
Nb=length(Y.bSigOut.ValidRSSI); kb= 2:Nb-3;

subplot(nPlot,1,3);
plot(t80,Y.MC,t80,Y.DACOPM/2+4,t80,Y.ADCAOPM+8,t80,Y.ADCFCAL+12,t80,Y.ClearDCX+14,t80,Y.DFEEnB+16,t20,Y.aSigOut.SigDet(k)+18);
grid on;
set(gca,'XTick',XTick);
set(gca,'YTick',0:1:19); set(gca,'YTickLabel',{'MC[1:0]','','','','DACOPM[2:0]/2','','','','ADCAOPM[1:0]','','','','ADCFCAL','','ClearDCX','','DFEEnB','','aSigDet',''});
axis([t1 t2 0 20]);
xlabel('Time (\mus)'); 
ThickLines;

c = get(gcf,'Children');
for i=1:length(c),
    P = get(c(i),'Position');
    if(P(3) > 0.5)
        P(1) = 0.12;
        P(3) = 0.82;
        P(4) = 0.80/nPlot;
        set(c(i),'Position',P);
    end
end
set(c(3),'Position',[0.12 0.50 0.82 0.18]);
set(c(1),'Position',[0.12 0.08 0.82 0.39]);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function ThickLines;
c = get(gca,'Children');
set(c,'LineWidth',2);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [S,Y] = LoadData;
if(0)
    wiParse_Line('Mojave.EnableTrace    = 1');
    wiParse_Line('Mojave.EnableTraceCCK = 1');
    wiParse_Line('Mojave.EnableModelIO  = 1');
end
wiParse_ScriptFile('wise.txt');
[S,Y] = DW52_GetTraceState;
clear wiseMex;
