function DwPhyPlot_TxPacketTiming(data)

K = data.k0 - min(data.k0);
T = K/5; % from 400 MHz to 80 MHz period
t = data.t;
Y = 1e3*data.DACI;

[B,A] = butter(5,2*40/400);
Y = filtfilt(B,A,Y);

J = find(K == min(K));
i = 1:length(t);
if data.Mbps == 11,
    kk = (t>3.5 & t<4);
else
    kk = (t>3.5 & t<5);
end
i = i(kk);
for j=1:length(J),
    p(j) = max((Y(i,j)));
end
k = J(find(p == max(p), 1 ));

figure(gcf); clf;
plot(t,Y,'c', t,Y(:,k),'b'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2); set(c(1),'LineWidth',3);
grid on;
if max(Y(:,:)) > 200,
    axis([3.6 4.6 -280 280]);
else
    axis([3.6 4.6 -200 200]);
end
xlabel('Time (\mus) From MC0 \rightarrow 0');
if any(data.Mbps == [1 2 5.5 11]),
    ylabel({'DSSS/CCK','DACI\_P - DACI\_N (mV)'});
else
    ylabel({'OFDM','DACI\_P - DACI\_N (mV)'});
end    

K = ((data.k0 - min(data.k0)) / 5);
K = sort(K,'descend');


% figure(2); clf;
% for k = K,
%     if k==min(K), c='b'; else c='c'; end
%     i = find(K == k, 1 );
%     fprintf('i=%2d, c=%c, k=%g\n',i,c,k);
%     t = data.wfm{i}.t;
%     x = 1e3*data.wfm{i}.DACI;
%     plot(t,x,c);
% %    fprintf('%d, %d\n',round((data.k0(i)-min(data.k0))/5),data.k0(i));
%     hold on;
% end
% c = get(gca,'Children'); set(c,'LineWidth',2);
% hold off;
% grid on;
% axis([3.6 4.6 -200 200]);
% xlabel('Time (\mus) From MC0 -> 0');
% ylabel('DACI\_P - DACI\_N (mV)');
% 
% figure(3); clf;
% for i=1:length(data.wfm),
%     plot(data.wfm{i}.t, 1e3*data.wfm{i}.DACI);
%     hold on;
% end