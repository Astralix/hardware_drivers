function PlotFlatness(data)


if isfield(data,'LastResult')
    X = data.LastResult.result;
else
    X = data.result;
end

% (SM) the following FIR must match the one in CalcEVM_OFDM_DownsamFIR() of WiSE
b = [-0.002150, -0.016279,  0.003041,  0.004113,  0.006649,  0.005551,  0.000839, ...
     -0.005340, -0.009258, -0.007907, -0.001146,  0.007692,  0.013195,  0.011099, ... 
      0.001315, -0.011326, -0.019094, -0.015891, -0.001426,  0.017364,  0.029147, ... 
      0.024354,  0.001512, -0.029722, -0.051415, -0.045144, -0.001560,  0.073340, ... 
      0.158603,  0.225993,  0.251566,  0.225993,  0.158603,  0.073340, -0.001560, ...
     -0.045144, -0.051415, -0.029722,  0.001512,  0.024354,  0.029147,  0.017364, ...
     -0.001426, -0.015891, -0.019094, -0.011326,  0.001315,  0.011099,  0.013195, ... 
      0.007692, -0.001146, -0.007907, -0.009258, -0.005340,  0.000839,  0.005551, ... 
      0.006649,  0.004113,  0.003041, -0.016279, -0.002150];
f = (-32:31)/64*20;
G = abs(freqz(b,1,f,80))';

for i=1:length(X),
    H(:,i) = X{i}.h0 ./G;
end


HdB = 20*log10(abs(H));

HdBx = mean(HdB,2);

k = -32:31;
p = mean(abs(H).^2,2);
i = find(abs(k) >= 1 & abs(k) <= 16);
j = find(abs(k) >= 1 & abs(k) <= 26);
p0 = mean(p(i));     % average in subcarriers +/-1 to +/-16
d = 10*log10(p/p0);  % deviation
Flatness = 1;
if max(abs(d(i))) >  2, Flatness = 0; end
if max(   (d(j))) >  2, Flatness = 0; end
if min(   (d(j))) < -4, Flatness = 0; end %#ok<FNDSB>
if d(32) < 2, Leakage = 1; else Leakage = 0; end

% Mojave Upsample FIR
b0 = [ 0,  0,  0,  0,  0,  0, 64,  0,  0,  0,  0,  0];
b1 = [ 0,  1, -2,  4, -7, 19, 57,-11,  5, -3,  1, -1];
b2 = [-1,  2, -3,  6,-12, 40, 40,-12,  6, -3,  2, -1];
b3 = [-1,  1, -3,  5,-11, 57, 19, -7,  4, -2,  1,  0];
a = 2^8;
b = [];
for ii=1:length(b0),
    b = [b b0(ii)];
    b = [b b1(ii)];
    b = [b b2(ii)];
    b = [b b3(ii)];
end

[G,F] = freqz(b,a,800,80); G=abs(G);
G0 = mean(G(i).^2);
G = 10*log10(G.^2/G0);
Fk = F*64/20;

kk = -28:0.1:28;
MaskH = 2*ones(size(kk));
MaskL = -4*ones(size(kk));
MaskL(abs(kk) < 16.6) = -2;
MaskL(abs(kk) <  1.0) = -Inf;

figure
plot(kk,MaskH,'r',kk,MaskL,'r',k,d,'b.-'); grid on;
axis([-27 27 -20 3]);
xlabel('Subcarrier'); ylabel('Magnitude (dB)');
set(gca,'XTick',-32:4:32);
set(gca,'YTick',-20:2:10);
c = get(gca,'Children'); set(c,'LineWidth',2); set(c,'MarkerSize',18);
set(c(2:3),'LineWidth',3);
P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 320]);

%% REVISIONS
% 2009-03-16 Corrected PlotFlatness function per e-mail from Marc Morin
