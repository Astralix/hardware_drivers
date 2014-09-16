function [outS,outY] = mexPlotMojave(t1,t2,tt1,tt2,dt)

PlotOnly = 0;
SetLimit = 0;
dt = [];
if(nargin>=4)
    S  = t1;  Y  = t2;
    t1 = tt1; t2 = tt2;
    PlotOnly = 1;
    SetLimit = 1;
end
if (nargin==2) || (nargin==3),
    if(isstruct(t1) && isstruct(t2))
        S = t1; Y = t2;
        PlotOnly = 1;
    else
        SetLimit = 1;
    end
end
if nargin==3, dt=tt1; end;
if nargin==1, dt=t1; end;

%%% Get the Trace Data
if(~PlotOnly)
    [S,Y] = LoadData; 
end;

if(ishandle(1))
    figure(gcf); clf;
else
    figure(1); set(gcf,'Position',[5 125 1250 535]);
%    figure(1); set(gcf,'Position',[5 375 1250 750]);
end

k80 = 1:length(S.traceRxState);
k40 = 1:length(S.c);
k20 = 1:length(S.y);

if isempty(dt), dt = 50.525; end; % OFDM
if isempty(dt), dt = 50.600; end; % DSSS/CCK
t20=k20/20-dt; Y.t20 = t20;
t40=k40/40-dt; Y.t40 = t40;
t80=k80/80-dt; Y.t80 = t80;
Y.t22 = Y.t80(Y.bRX.k80+1);

if(nargout>0), outS=S; outY=Y; end

%p1=0.09;
%p3=0.88;
%p4=0.27;

nPlot=3;
%subplot(nPlot,1,1); plot(t40,real(S.r(:,1)),t40,real(S.c(:,1)),'r',t20,real(S.y(:,1)),'g'); grid on; 
%subplot(nPlot,1,1); plot(t40,real(S.r(:,1)),t40,imag(S.r(:,1)),'r'); grid on; 
subplot(nPlot,1,1); plot(t40,real(S.r(:,1)),t40,real(S.c(:,1)),'r'); grid on; 
%p = get(gca,'Position'); set(gca,'Position',[p1 p(2) p3 p4]);
set(gca,'XTickLabel',{''});
A = axis; 
if(~SetLimit), t1=A(1); end; A(1) = t1;
if(~SetLimit), t2=A(2); end; A(2) = t2; 
A(3) = -600; A(4) = 600; axis(A);


%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN); grid on; axis([t1 t2 0 40]);
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State); grid on; axis([t1 t2 0 40]);
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.RSSI0(2:length(Y.RSSI0))*0.75); grid on; axis([t1 t2 0 40]);
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.DFE.State); grid on; axis([t1 t2 0 40]); legend('MC','State','PktTime','AGAIN','LNAGAIN','DFEState');
subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.DFE.State,Y.t20,Y.RSSI0(2:length(Y.RSSI0))*0.75); grid on; axis([t1 t2 0 40]); legend('MC','State','PktTime','AGAIN','LNAGAIN','DFEState','RSSI0(dB)');
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State,Y.t20,Y.RSSI0(2:length(Y.RSSI0))*0.75); grid on; axis([t1 t2 0 40]); legend('MC','State','PktTime','AGAIN','LNAGAIN','FrontCtrl','RSSI0(dB)');
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State,Y.t22,Y.bRX.DGain); grid on; axis([t1 t2 0 40]); legend('MC','State','PktTime','AGAIN','LNAGAIN','FrontCtrl','bDGain');
%subplot(nPlot,1,2); plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.DACOPM+0.02,t80,Y.ADCAOPM+0.04,t80,Y.ADCBOPM+0.06); grid on; axis([t1 t2 0 40]); legend('MC','State','PktTime','AGAIN','DACOPM','ADCAOPM','ADCBOBM');
set(gca,'YTick',0:4:40);
set(gca,'XTickLabel',{''});
%p = get(gca,'Position'); set(gca,'Position',[p1 p(2) p3 p4]);
ThickLines;
%legend('MC','State','PktTime','AGAIN','LNAGAIN');
%legend('MC','State','PktTime','AGAIN','LNAGAIN','bState');

N =length(Y.aSigOut.ValidRSSI); k = 2:N;
Nb=length(Y.bSigOut.ValidRSSI); kb= 2:Nb-3;
%Nb=length(Y.bSigOut.ValidRSSI); kb= 2:Nb-2;

SignalSet = 4;
switch(SignalSet)
    case 0, % TX
        subplot(nPlot,1,3); plot(t80,Y.MAC_TX_EN+0,t80,Y.TxEnB+2,t80,Y.TxDone+4,t80,Y.SW1+6,t80,Y.PA24+8); grid on;
        set(gca,'YTick',0:2:8); set(gca,'YTickLabel',{'MAC_TX_EN','TxEnB','TxDone','SW1','PA24'});
        axis([t1 t2 0 10]);
    case 1,
        subplot(nPlot,1,3); plot(t80,Y.MAC_TX_EN+0,t80,Y.PHY_CCA+2,t80,Y.ClearDCX+4,t80,Y.SW1+6); grid on;
        set(gca,'YTick',0:2:6); set(gca,'YTickLabel',{'MAC_TX_EN','PHY_CCA','ClearDCX','SW1'});
        axis([t1 t2 0 8]);
    case 2, % DFS
        subplot(nPlot,1,3); 
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.PulseDet(k)+6,t20,Y.aSigOut.StepUp(k)+8,t20,Y.aSigOut.StepDown(k)+10,t20,Y.RadarDet(k)+12);
        grid on;
        set(gca,'YTick',0:2:12); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','PulseDet','aStepUp','aStepDown','RadarDet'});
        axis([t1 t2 0 14]);
    case 3, % "b"
        subplot(nPlot,1,3); 
        plot(t80,Y.PHY_CCA+0, Y.t22,Y.bSigOut.ValidRSSI(kb)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.PulseDet(k)+6,Y.t22,Y.bSigOut.StepUp(kb)+8,Y.t22,Y.bSigOut.StepDown(kb)+10,Y.t22,Y.CEDone+12,Y.t22,Y.bSigOut.RxDone(kb)+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','PulseDet','bStepUp','bStepDown','CEDone','bRxDone'});
        axis([t1 t2 0 16]);
    case 4, % OFDM-RX
        subplot(nPlot,1,3); 
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.AGCdone(k)+6, t20,Y.aSigOut.PeakFound(k)+8,t20,Y.aSigOut.HeaderValid(k)+10,t20,Y.aSigOut.RxDone(k)+12,t80,Y.ADCFCAL+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','AGCdone','PeakFound','HeaderValid','RxDone','ADCFCAL'});
        axis([t1 t2 0 16]);
    case 5, % External LNA
        subplot(nPlot,1,3); 
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.AGCdone(k)+6, t80,Y.LNAGAIN2+8, t80,Y.LNA50A+10, t80,Y.LNA50B+12);
        grid on;
        set(gca,'YTick',0:2:12); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','AGCdone','LNAGAIN2','LNA50A','LNA50B'});
        axis([t1 t2 0 14]);
    case 6, % New Packet Restart
        subplot(nPlot,1,3); 
        plot(t80,Y.PHY_CCA+0, t80,Y.RestartRX+2, t80,Y.DFERestart+4, t20,Y.aSigOut.StepUp(k)+6,t20,Y.aSigOut.StepDown(k)+8,t20,Y.aSigOut.SigDet(k)+10,Y.t22,Y.bSigOut.SigDet(kb)+12,t80,Y.DW_PHYEnB+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','RestartRX','DFERestart','aStepUp','aStepDown','aSigDet','bSigDet','DW_PHYEnB'});
        axis([t1 t2 0 16]);
    case 7, % Signal Detect
        subplot(nPlot,1,3); 
        SigDetValid = double(Y.SigDet.State == max(Y.SigDet.State)); % assume max value is SigDetDelay
        plot(t80,Y.PHY_CCA+0, t80,Y.ModemSleep+2, t80,Y.DFERestart+4, t20,Y.SigDet.SDEnB+6,t20,SigDetValid+8,t20,Y.aSigOut.SigDet(k)+10,Y.t22,Y.bSigOut.SigDet(kb)+12,t80,Y.DFEEnB+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','ModemSleep','DFERestart','SDEnB','SigDetValid','aSigDet','bSigDet','DFEEnB'});
        axis([t1 t2 0 16]);

end

if(nPlot==4)
    ThickLines;
    set(gca,'XTickLabel',{''});
    subplot(nPlot,1,4); 
%    plot(t20,Y.FrameSync.FSEnB+0,t20,Y.FrameSync.SigDetValid+2,t20,Y.FrameSync.PeakFound+4,t20,Y.FrameSync.AGCDone+6,t20,Y.FrameSync.PeakSearch+8,t20,Y.FrameSync.Ratio+10);
%    set(gca,'YTick',0:2:10); set(gca,'YTickLabel',{'FSEnB','SigDetValid','PeakFound','AGCDone','PeakSearch','Ratio'});
    plot(t20,Y.SigDet.SDEnB+0,t20,Y.FrameSync.SigDetValid+2,t20,Y.SigDet.SigDet+4,t20,Y.FrameSync.AGCDone+6,t20,Y.FrameSync.PeakSearch+8,t20,Y.SigDet.x+10);
    set(gca,'YTick',0:2:10); set(gca,'YTickLabel',{'FSEnB','SigDetValid','PeakFound','AGCDone','PeakSearch','Ratio'});
%    plot(t80,Y.bPathMux+0, Y.t22, bSigOut.RxDone(kb)+2, Y.t20, aSigOut.Run11b(k)+4);
%    set(gca,'YTick',0:2:4); set(gca,'YTickLabel',{'bPathMux','bRxDone','Run11b'});
%    plot(t20,Y.aSigOut.SigDet(k), Y.t22,Y.bSigOut.SigDet(kb)+2);
%    set(gca,'YTick',0:2:2); set(gca,'YTickLabel',{'aSigDet','bSigDet'});
    grid on;
    axis([t1 t2 0 12]);
end
xlabel('Time (\mus)'); 
%p = get(gca,'Position'); set(gca,'Position',[p1 p(2) p3 p4]);
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function ThickLines
c = get(gca,'Children');
set(c,'LineWidth',2);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [S,Y] = LoadData
if(0)
    wiParse_Line('Mojave.EnableTrace    = 1');
    wiParse_Line('Mojave.EnableTraceCCK = 1');
    wiParse_Line('Mojave.EnableModelIO  = 1');
end
wiParse_ScriptFile('wise.txt');
[S,Y] = DW52_GetTraceState;
clear wiseMex;
