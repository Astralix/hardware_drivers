function DwPhyPlot_SplitBand(x,varargin)
% DwPhyPlot_SplitBand(x, ...)
%   Uses plot(x,...) to plot data in two subplots corresponding to the 2.4
%   and 5 GHz bands.
%
% DwPhyPlot_Splo\itBand('FormatY',YLim,YTick,YTickLabel,YLabel)
%   Format the y-axis on the current SplitBand plot

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if ischar(x) && strcmpi(x,'FormatY')
    FormatY(varargin{1}, varargin{2}, varargin{3}, varargin{4});
else
    figure(gcf); clf;
    P = get(gcf,'Position');
    set(gcf,'Position',[P(1) P(2) 620 320]);

    H = 0.8; % relative axis height
    
    if any(x > 4000) && any(x < 3000),
        subplot(1,2,1); plot(x,varargin{:}); grid on;
        A = axis; axis([2400 2500 A(3) A(4)]);
        set(gca,'Position',[0.12 0.13 0.072 H]);
        set(gca,'XTick',[2412 2484]);
        FormatTraces;

        subplot(1,2,2); plot(x,varargin{:}); grid on;
        A = axis; axis([4900 5810 A(3) A(4)]);
        set(gca,'Position',[0.23 0.13 0.72 H]);
        set(gca,'XTick',4900:100:5900);
        set(gca,'YTickLabel',{''});
        xlabel('Frequency (MHz)  ');
        FormatTraces;
        
    else
        plot(x,varargin{:}); grid on;
        A = axis; axis([2400 2500 A(3) A(4)]);
        set(gca,'XTick',2400:10:2500);
        FormatTraces;
    end
end

%% FormatY
function FormatY(YLim,YTick,YTickLabel,YLabel)
c = get(gcf,'Children');
if length(c) == 2,
    c = [c(2) c(1)]; % reverse so plot on the left is c(1)
    if ~isempty(YLim),
        set(c(1),'YLim',YLim); set(c(1),'YLimMode','manual');
        set(c(2),'YLim',YLim); set(c(2),'YLimMode','manual');
    end
    if ~isempty(YTick),
        set(c(1),'YTick',YTick); set(c(1), 'YTickMode','manual');
        set(c(2),'YTick',YTick); set(c(2), 'YTickMode','manual');
    end
    if ~isempty(YTickLabel),
        set(c(1),'YTickLabel',YTickLabel); 
        set(c(1), 'YTickLabelMode','manual');
    end
    if ~isempty(YLabel),
        h = get(c(1),'YLabel');
        set(h,'String',YLabel);
    end
else
    if ~isempty(YLim),
        set(c(1),'YLim',YLim); set(c(1),'YLimMode','manual');
    end
    if ~isempty(YTick),
        set(c(1),'YTick',YTick); set(c(1), 'YTickMode','manual');
    end
    if ~isempty(YTickLabel),
        set(c(1),'YTickLabel',YTickLabel); 
        set(c(1), 'YTickLabelMode','manual');
    end
    if ~isempty(YLabel),
        h = get(c(1),'YLabel');
        set(h,'String',YLabel);
    end
end

%% FormatTraces
function FormatTraces
c = get(gca,'Children');
set(c,'MarkerSize',18);
set(c,'LineWidth',2);
