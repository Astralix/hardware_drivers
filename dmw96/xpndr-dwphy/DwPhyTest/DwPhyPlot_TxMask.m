function DwPhyPlot_TxMask(data, TXPWRLVL)
% DwPhyPlot_TxMask(data) 
% DwPhyPlot_TxMask(Filename)
% DwPhyPlot_TxMask(FileList, TXPWRLVL)

[X,N] = DwPhyPlot_LoadData(data);

% % If data is a filename, load the data structure...
% if ischar(data)
%     S = load(data);
%     data = S.data;
% end

% Retrieve the PartID...
for i=1:N,
    if isfield(X{i},'PartID')
        PartID{i} = X{i}.PartID;
    else
        PartID{i} = [];
    end
end

if N == 1,
    % Determine type of data provided and plot accordingly...
    if isfield(X{1},'Test202_1') || isfield(X{1},'Test202_2') || isfield(X{1},'Test202_3') || isfield(X{1},'Test202_4')
        if nargin<2, TXPWRLVL = []; end
        for n=1:4,
            fieldname = sprintf('Test202_%d',n);
            if isfield(X{1}, fieldname)
                PlotTest202(n, X{1}.(fieldname), PartID, TXPWRLVL);
            end
        end
    end
    if isfield(X{1},'Test211_1'), 
         PlotTest211(X{1,1}.Test211_1, X{1,1}.PartID, X{1,1}.Timestamp) ;        
    elseif isfield(X{1}, 'TestID') && strcmp(X{1}.TestID,'211.1'),
        PlotTest211(X{1});
    elseif isfield(X{1},'Description') && strcmpi(X{1}.Description,'DwPhyTest_TxSpectrumMask')
        PlotSpectrum(1, X{1}.fcMHz, X{1}.fMHz, X{1}.PSDdBr, X{1}.Limit, X{1}.TXPWRLVL);
    end
else
    % Determine type of data provided and plot accordingly...
    if isfield(X{1},'Test202_1') || isfield(X{1},'Test202_2') || isfield(X{1},'Test202_3') || isfield(X{1},'Test202_4')
        if nargin<2, TXPWRLVL = 63; end
        for n=1:4,
            fieldname = sprintf('Test202_%d',n);
            if isfield(X{1}, fieldname)
                PlotTest202N(N, n, X, fieldname, PartID, TXPWRLVL(end));
            end
        end
    end
end

%% PlotTest202
function PlotTest202(n, data, PartID, TXPWRLVL)
x = data.result{1}.fMHz;
z = data.result{1}.Limit;

if isempty(TXPWRLVL)
    if min(data.TXPWRLVL < 0)
        TXPWRLVL = [0 18 36 54];
    else
        TXPWRLVL = [43 55 59 63];
    end
end

y = zeros(length(TXPWRLVL), length(x));
for i=1:length(TXPWRLVL),
   k = find(data.TXPWRLVL == TXPWRLVL(i));
   y(i,:) = data.result{k}.PSDdBr;
end

PlotSpectrum(n, data.fcMHz, x, y, z, TXPWRLVL);

if ~isempty(PartID),
   title( sprintf('Part %s, Tested %s',PartID{1}, data.Timestamp) );
end

%% PlotTest202N
function PlotTest202N(N, n, X, fieldname, PartID, TXPWRLVL)
ResultStr = {'FAIL','PASS'};
x = X{1}.(fieldname).result{1}.fMHz;
z = X{1}.(fieldname).result{1}.Limit;
y = zeros(N, length(x));
for i = 1:N,
    k = find(X{i}.(fieldname).TXPWRLVL == TXPWRLVL);
    y(i,:) = X{i}.(fieldname).result{k}.PSDdBr;
end

figure(n); clf;
plot(x,y, x,z,'k',x,z,'y'); grid on;
hold on; plot(x,y);
c = get(gca,'Children');
set(c,'LineWidth',1.5);
set(c(N+2),'LineWidth',5);
axis([-50 50 -60 1]);
legend(PartID);
xlabel(sprintf('Frequency Offset (MHz) at %d MHz',X{1}.(fieldname).fcMHz)); 
switch n,
    case 1, ylabel('PSD (dB) | L-OFDM');
    case 2, ylabel('PSD (dB) | DSSS/CCK');
    case 4, ylabel('PSD (dB) | HT-OFDM');
    otherwise, error('Undefined Case');
end
%title(fieldname);
result = 1; 
for i=1:N 
    result = result && (X{i}.(fieldname).Result==1)
end
%titlestr = sprintf('%s: %s',fieldname,ResultStr{X{n}.(fieldname).Result + 1});
titlestr = sprintf('%s  Power = %d:  %s' ,fieldname, TXPWRLVL,ResultStr{result + 1});
titlestr(titlestr == '_') = '.';
title(titlestr);

%        titlestr = sprintf('Test %s: %s',TestName{i},ResultStr{Result+1});
%        titlestr(titlestr == '_') = '.';

%% PlotTest211
function PlotTest211(data,PartID,Timestamp)
x = data.result{1}.fMHz;
z = data.result{1}.Limit;

K = data.k + [0 -2 -4 -6];
PADGAIN = data.PADGAIN(K);
y = zeros(length(K), length(x));
for i=1:length(K),
   y(i,:) = data.result{K(i)}.PSDdBr;
end

PlotSpectrum(1, data.fcMHz, x, y, z, PADGAIN); ylabel('PSD (dB) | L-OFDM');
if ~isempty(PartID),
            title( sprintf('Part %s, Tested %s',PartID,Timestamp) );
end
PlotSpectrum(2, data.fcMHz, data.resultCCK.fMHz, data.resultCCK.PSDdBr, ... 
    data.resultCCK.Limit, data.PADGAIN(data.k)); ylabel('PSD (dB) | DSSS/CCK');
if ~isempty(PartID),
            title( sprintf('Part %s, Tested %s',PartID,Timestamp) );
end

%% PlotSpectrum
function PlotSpectrum(n, fcMHz, x, y, z, TXPWRLVL)

label = cell(length(TXPWRLVL), 1);
if max(TXPWRLVL < 64)
    for i=1:length(TXPWRLVL),
       label{i} = sprintf('TXPWRLVL = %d',TXPWRLVL(i));
    end
else
    for i=1:length(TXPWRLVL),
       label{i} = sprintf('PADGAIN = %d',TXPWRLVL(i));
    end
end    

figure(n); clf;
plot(x,y, x,z,'k',x,z,'y'); grid on;
hold on; plot(x,y);
c = get(gca,'Children');
set(c,'LineWidth',2);
set(c(length(TXPWRLVL)+2),'LineWidth',5);
axis([-50 50 -60 1]);
legend(label);
xlabel(sprintf('Frequency Offset (MHz) at %d MHz',fcMHz)); 
ylabel('PSD (dBr)');

%% REVISIONS
% 2010-12-30 [SM]: 1. Make PlotTest202() similar to PlotTest202N() to accept user-defined TXPWRLVL.
%                  2. When N=1 and no matching tests are found, do nothing instead of error.
