function DwPhyPlot_TxPhaseNoise(data, n)
% DwPhyPlot_TxPhaseNoise(data) plot data = DwPhyTest_TxPhaseNoise
% DwPhyPlot_TxPhaseNoise(data,n) TXPWRLVL(n) from data = DwPhyTest_RunSuite('TxEval')
% DwPhyPlot_TxPhaseNoise(file) plot data contained in the specified file

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

[X,N] = DwPhyPlot_LoadData(data);

if N == 1,
    if any( isfield(X{1},{'Test203_1','Test203_2'}) )
        if nargin < 2, n = 3; end
        x{1} = X{1}.Test203_1.result{n}.PhaseNoiseSpectrum.f;
        y{1} = X{1}.Test203_1.result{n}.PhaseNoiseSpectrum.dBc;
        if isfield(X{1},'Test203_2'),
            x{2} = X{1}.Test203_2.result{n}.PhaseNoiseSpectrum.f;
            y{2} = X{1}.Test203_2.result{n}.PhaseNoiseSpectrum.dBc;
        end
        PlotPhaseNoise(x,y);
        if length(x)>1, legend('2412+10 MHz','5200+10 MHz'); end
    elseif isfield(X{1}, 'PhaseNoiseSpectrum')
        x{1} = X{1}.PhaseNoiseSpectrum.f;
        y{1} = X{1}.PhaseNoiseSpectrum.dBc;
        PlotPhaseNoise(x,y, X{1}.fcMHz);
        PlotSpectrum(X{1}.trace.fMHz, X{1}.trace.PdBm, X{1}.fcMHz);
    end
else
    if isfield(X{1},'Test203_1'),
        n = 4;
        for i=1:N,
            PartID{i} = X{i}.PartID;
            x{i} = X{i}.Test203_1.result{n}.PhaseNoiseSpectrum.f;
            y{i} = X{i}.Test203_1.result{n}.PhaseNoiseSpectrum.dBc;
            IMRdB(i) = -X{i}.Test203_1.result{n}.IQ_dBc;
            dBc(i)   =  X{i}.Test203_1.result{n}.PhaseNoise_dBc;
            LOdBc(i) =  X{i}.Test203_1.result{n}.LO_dBc;
            fprintf('%20s %6.1f %6.1f %6.1f\n',PartID{i},dBc(i),IMRdB(i),LOdBc(i));
        end
        PlotPhaseNoise(x,y);
        legend(PartID);
        title(sprintf('Phase Noise at %d MHz',X{1}.Test203_1.fcMHz));
    end
end
    
%% PlotSpectrum
function PlotSpectrum(x,y,fcMHz)
figure(gcf+1); clf;
plot(x,y,'.-'); grid on;
c = get(gca,'Children'); set(c,'LineWidth',2);
xlabel(sprintf('Frequency Offset at %d MHz',fcMHz));
ylabel('Power (dBm)');

%% PlotPhaseNoise
function PlotPhaseNoise(x,y,fcMHz)
if ~isempty(x) && ~isempty(y),
    figure(gcf); clf;
    DwPhyPlot_Command(length(x),'semilogx','x{%d}','y{%d}'); grid on;
    xlabel('Frequency (Hz)'); ylabel('PSD (dBc/Hz)');
    if nargin>2,
        ylabel({'PSD (dBc/Hz)',sprintf('%d + 10 MHz',fcMHz)});
    end
    c = get(gca,'Children'); set(c,'LineWidth',3);
    set(gca,'YTick',-200:10:0);
    set(gca,'XTick',[1e3 1e4 1e5 1e6 1e7]);
    set(gca,'XTickLabel',{'1 kHz','10 kHz','100 kHz','1 MHz','10 MHz'});
end

%% REVISIONS
% 2010-12-31 [SM]: If N=1 and no matching tests/fields are found, do nothing.
