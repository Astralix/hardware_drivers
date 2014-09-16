function DwPhyPlot_TxPowerRamp(data)

AlignToMC0 = 1;

t = data.scope{1}.t;
p1 = abs(data.wfm{1}.y).^2;
p2 = abs(data.wfm{2}.y).^2;

k0 = find(p1 > max(p1)/2, 1 );
k = k0:length(p1);
p0 = mean(p1(k));
P1 = 10*log10(p1/p0);
P2 = 10*log10(p2/p0);

if ~AlignToMC0
    k0 = find(P1 > -3, 1 );
    t = t-t(k0);
end

figure(1); clf;
P = get(gcf,'Position'); P(3) = 615; P(4)=300; set(gcf,'Position',P);
plot(t,P1,'g',t,P2,'b'); grid on; 
set(get(gca,'Children'),'LineWidth',2)
if AlignToMC0
    xlabel('Time (\mus) Relative to MC0 \rightarrow 0');
    axis([0 5 -73 3]);
else
    xlabel('Time (\mus) Relative to Packet Start'); 
    axis([-4 1 -73 3]); 
end
ylabel('Signal Level (dBr)');
DelayPA = bitand( data.RegList(217,2), 15);
LODISONSQLCH = bitget( data.RegList(256+24,2), 7);
PADPDSQLCHEN = bitget( data.RegList(256+63,2), 8);
DCOFFCNT = floor( data.RegList(256+53,2)/2);
T0CNT   = data.RegList(256+78,2);
T1CNT   = data.RegList(256+79,2);
T2CNT   = data.RegList(256+80,2);
s = sprintf('DelayPA = %d, DCOFFCNT = %d, T1CNT = %d, LODISONSQLCH = %d, PADPDSQLCHEN = %d', ...
    DelayPA,DCOFFCNT,T1CNT,LODISONSQLCH,PADPDSQLCHEN);
title(s);


t = data.wfm{1}.t;
t = t(17:length(t));
P1 = 10*log10(data.wfm{2}.p);
P2 = 10*log10(data.wfm{2}.q);

figure(2); clf;
P = get(gcf,'Position'); P(3) = 615; P(4)=300; set(gcf,'Position',P);
plot(t,P1,'g',t,P2,'b'); grid on; 
set(get(gca,'Children'),'LineWidth',2)
xlabel('Time (\mus) Relative to Packet Start'); 
axis([-5 0 -80 0]); 
ylabel('Signal Detect Threshold');
DelayPA = bitand( data.RegList(217,2), 15);
LODISONSQLCH = bitget( data.RegList(256+24,2), 7);
PADPDSQLCHEN = bitget( data.RegList(256+63,2), 8);
DCOFFCNT = floor( data.RegList(256+53,2)/2);
T0CNT   = data.RegList(256+78,2);
T1CNT   = data.RegList(256+79,2);
T2CNT   = data.RegList(256+80,2);
s = sprintf('DelayPA = %d, DCOFFCNT = %d, T1CNT = %d, LODISONSQLCH = %d, PADPDSQLCHEN = %d', ...
    DelayPA,DCOFFCNT,T1CNT,LODISONSQLCH,PADPDSQLCHEN);
title(s);
