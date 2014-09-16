function [X,S] = LoadWiSE_PERvPrdBm(filename, varargin)
% [X,S] = LoadWiSE_PERvPrdBm(filename, opt)

PlotPER = 0;
Smoothing = 1;
SmoothingN = 5;

if nargin<1, filename = 'wiTest.out'; end
if nargin > 1,
    for i=1:length(varargin),
        eval(sprintf('%s;',varargin{i}));
    end
end

F = fopen(filename,'r');
if F<1, error('Unable to load file %s',filename); end
n = 0;
while ~feof(F)
    n = n+1;
    t{n}.line = fgets(F);
end
fclose(F);
S.file = t;

% Locate Data Rate
for i=1:n,
    if(findstr(t{i}.line,'Data Rate'))
        S.DataRate = GetParameter(t{i}.line);
    end
end

% Locate Data Length
for i=1:n,
    if(findstr(t{i}.line,'Data Length'))
        S.Length = GetParameter(t{i}.line);
    end
end

% Locate Data Array
nheader = 0;
for i=1:n,
    if (~isempty(findstr(t{i}.line,'dB')) && ...
        ~isempty(findstr(t{i}.line,'# Errors')) && ...
        ~isempty(findstr(t{i}.line,'BER')) && ...
        ~isempty(findstr(t{i}.line,'PER')) )
        nheader = i;
    end
end
if ~nheader, error('  Header Not Found: Check File Format'); end

% Extract SNR Type
if ~isempty(findstr(t{nheader}.line,'Pr[dBm]'))
    S.SNRlabel='Pr[dBm]';
elseif ~isempty(findstr(t{nheader}.line,'Eb/No(dB)'))
    S.SNRlabel='Eb/No(dB)';
else
    S.SNRlabel='(dB)';
end

% Extract Data Array
X = [];
for i=nheader+2:n,
    x = str2num(t{i}.line); %#ok<ST2NM>
    if(length(x)>1)
        X = [X;x];
    end
end
S.SNR = X(:,1);
S.BER = X(:,3);
S.PER = X(:,5);

S.SNR010 = GetSensitivity(S,0.10);
S.SNR001 = GetSensitivity(S,0.01);

if Smoothing,
    S.SmoothPER = polysmooth(S.SNR, S.PER, SmoothingN);
    S.smoothPER = S.SmoothPER; % legacy name
end
if PlotPER,
    figure(gcf); clf;
    semilogy([-1000 S.SNR010 S.SNR010],[0.10 0.10 1e-20],'r'); hold on
    semilogy([-1000 S.SNR001 S.SNR001],[0.01 0.01 1e-20],'r'); hold on;
    if Smoothing,
        semilogy(S.SNR,S.SmoothPER,'b-', S.SNR,S.PER,'b.');
    else
        semilogy(S.SNR,S.PER,'b.-');
    end
    axis([min(S.SNR) max(S.SNR) 5e-4 1]); grid on;
    xlabel(S.SNRlabel); ylabel(sprintf('PER, %d Bytes, %g Mbps',S.Length,S.DataRate));
    set(gca,'yticklabel',{'0.1%','1%','10%','100%'});
    c = get(gca,'Children');
    set(c(1),'LineWidth',2.5);
    set(c(1),'MarkerSize',18);
    hold off;
end

% -----------------------------------------------------------------------
function x = GetParameter(line)
k = findstr(line,'=')+1;
while line(k) == ' ', k = k + 1; end
line = line(k:length(line));
x = sscanf(line,'%g');

% -----------------------------------------------------------------------
function SNR0 = GetSensitivity(S,PER0)
x = S.SNR; y=S.PER;
[x,I] = sort(x); y=y(I);       % sort to ascending order of SNR
k = find(y>0); x=x(k); y=y(k); % get rid of PER=0 points
k = find(y<1); x=x(k); y=y(k); % get rid of PER>=1 points
y = log10(y); y0=log10(PER0);  % convert to log scale
e = abs(y-y0);
k0 = find(e==min(e), 1 );
if(isempty(k0)||length(y)<4)
    SNR0 = NaN;
else
%     if(mean(diff(x))>0.3) 
%         k = k0 + (-2:1);
%     else
%         k = k0 + (-3:3);
%         if max(abs(y(k)-y0)) > 1, k = k0 + (-2:2); end;
%         if max(abs(y(k)-y0)) > 1, k = k0 + (-2:1); end;
%     end
%     SNR0 = interp1(y(k),x(k),y0,'spline');

    warning('off','all');
    P = polyfit(x,y-y0,4);
    R = roots(P);
    e = abs(R-x(k0));
    SNR0 = R(find(e==min(e), 1 ));
end

% -----------------------------------------------------------------------
function z = polysmooth(x,y,n)
if(nargin<3), n=3; end
z=y;
k1 = find(y==1, 1, 'last' ); if(isempty(k1)), k1=1; end; k1=max(1,k1-1);
k2 = find(y<2e-4, 1 ); if(isempty(k2)), k2=length(y); end
while(y(k2)==0), k2=k2-1; end;
k=k1:k2;
P = polyfit(x(k),log(y(k)),n);
z(k) = exp(polyval(P,x(k)));
z = min(z,1); z=max(z,0);
z(k1) = max(z(k1), y(k1)); % fix for some end-point deviations [ad-hoc]