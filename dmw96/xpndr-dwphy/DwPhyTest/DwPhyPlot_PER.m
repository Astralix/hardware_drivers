function DwPhyPlot_PER(x, varargin)
% DwPhyPlot_PER(x, ...) plot log(PER) vs Pr[dBm]

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

figure(gcf); clf;
semilogy(x, varargin{:});
axis([-90 0 1e-3 1]); grid on;

P = get(gcf,'Position');
set(gcf,'Position',[P(1) P(2) 620 320]);

xlabel('Pr[dBm]'); ylabel('PER');
set(gca,'YTick',[1e-5 1e-4 1e-3 1e-2 1e-1 1]);
set(gca,'YTickLabel',{'0.001%','0.01%','0.1%','1%','10%','100%'});
set(gca,'XTick',-100:5:5);

c = get(gca,'Children');
set(c,'MarkerSize',18);
set(c,'LineWidth',2);
