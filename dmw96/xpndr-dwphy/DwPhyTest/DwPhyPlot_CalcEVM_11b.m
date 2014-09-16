function dataOut = DwPhyPlot_CalcEVM_11b(data, SplitPlots)
% data = DwPhyPlot_CalcEVM_11b(data) where data is the result from DwPhy_CalcEVM_11b

% Written by Barrett Brickner
% Copyright (c) 2007-2008 DSP Group, Inc. All rights reserved

if nargin <2, SplitPlots = 0; end
if nargout>0, dataOut = data; end;

%% Unpack data
M = data.M;
y = data.y;
u = data.u;
Verr = max(data.Verr);

%% Pass/Fail Radius
R = 0.35 * sqrt(2); % 35% of signal vector length

%% Constellation View
figure(1); clf;
if ~SplitPlots,
    subplot(2,2,1);
end
plot([-1 1],[-1 1],'r.-',[1 -1],[-1 1],'r.-');
set(get(gca,'Children'),'MarkerSize',24);
hold on;
plot(u,'.'); grid on; axis('square');
set(gca,'XTick',-1.5:0.5:1.5); set(gca,'XTickLabel',{'',-1,'',0,'',1,''});
set(gca,'YTick',-1.5:0.5:1.5); set(gca,'YTickLabel',{'',-1,'',0,'',1,''});
title('QPSK Constellation');
for a=-1:2:1,
    for b=-1:2:1,
        plot(a+R*[1 1 -1 -1 1],b+R*[-1 1 1 -1 -1],'r');
        rectangle('Curvature',[1 1],'Position',[a-R b-R 2*R 2*R],'EdgeColor','red');
    end
end
axis([-1.55 1.55 -1.55 1.55]);

%% Eye Diagram
if SplitPlots,
    figure(2); clf;
else
    subplot(2,2,2);
end
T=M; t=(1:T)/T - 0.5;
N = floor((length(y))/T)-1;
Y = reshape(y(M/2+1+(1:N*T)),T,N);

k = round((0:M) + M/2 + 0);
t2=(0:T)/T - 0.5;
for i=1:N-1 % 12092010 [SM] one less loop to avoid possible out-of-range indexing 
    Z(:,i) = y(k+i*M);
end

%plot(t2,real(Z),'c',t2,real(Z),'b.',0,real(u),'r.'); grid on; 
plot(t2,real(Z),'c',0,real(u),'b.'); grid on; 
title('Sampled Eye, "I" Component');
axis([-0.5 0.5 -1.5 1.5]);
set(gca,'XTick',-0.5:0.25:0.5); set(gca,'XTickLabel',{'-T/2','','0','','T/2'});
set(gca,'YTick',-1.5:0.5:1.5);  set(gca,'YTickLabel',{'',-1,'',0,'',1,''});

%% Samples Vs Time
if SplitPlots,
    figure(3); clf;
else
    subplot(2,1,2);
end
k=1:length(u); k=k-1; k1=max(k);
plot([0 k1],(-1-R)*[1 1],'r',[0 k1],(-1+R)*[1 1],'r',[0 k1],(1-R)*[1 1],'r',[0 k1],(1+R)*[1 1],'r'); hold on;
set(get(gca,'Children'),'LineWidth',2);
plot(k,real(u),'b.',k,imag(u),'g.'); grid on;
axis([0 length(u)-1, -1.55 1.55]);
set(gca,'YTick',[-1 0 1]);
xlabel('Sample # (11 MHz)');
if(Verr > 0.35)
    title(sprintf('FAILED: Verr = %1.3f, EVM = %1.1f dB',Verr, data.EVMdB));
else
    title(sprintf('max{Verr} = %1.3f, Margin = %1.1f dB, EVM = %1.1f dB',Verr,data.VerrMargin_dB,data.EVMdB));
end
