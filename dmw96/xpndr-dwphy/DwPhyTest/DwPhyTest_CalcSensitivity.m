function x0 = DwPhyTest_CalcSensitivity(x, y, y0)
%Calculate the sensitivity level
%
%   x0 = DwPhyTest_CalcSensitivity(x, y, y0) finds x0 where y0 = f(x0) for a function
%   computed from the data where y = f(x).
%
%   The typical use for this is to compute a power or SNR at which a particular error
%   rate occurs where x has logarithmic units (dB) and y is linear (BER/PER). For
%   most cases of interest, the function is exponential near the sensitivity
%   threshold, y0. As a result, curve fitting is done on z = log(f(x)).

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% if the first input is a structure, assume it is a data structure and attempt to
% extract the appropriate parameters
if nargin == 1 && isstruct(x),
    y0 = 0.1;
    y = x.PER;
    x = x.PrdBm;
    if isfield(x,'SensitivityLevel'), y0=x.SensitivityLevel; end
end

% if none of the points are below the sensitivity level, return NaN
y = reshape(y,size(x));
if min(y) > y0,
    x0 = NaN;
    return;
end

% if there are no point below the sensitivity level but above 0% PER, return the
% first power below sensitivity
k = find(y < y0, 1);
if y(k) == 0,
    x0 = x(k);
    return;
end

% if all the points are below the sensitivity level, return the first power level
if max(y) <= y0,
    x0 = x(1);
    return;
end

% convert to log domain to faciliate polynomial curve fitting
y  = reshape(y,size(x));
z  = log10(y); 
z0 = log10(y0);

% get rid of PER = 0 points at the end
while (z(length(z)) == -Inf) && length(z) > 1,
    z = z(1:(length(z)-1));
end

% find the point closest to the desired sensitivity level with an adjacent point 
% on the opposite side of the level
s = sign(z - z0);
d = diff(sign(z-z0));
k0 = find(abs(d)>0, 1) - 1;
k = k0 + (1:2);
d = abs(z(k) - z0);
k0 = k0 + round(mean( find(d == min(d)) ));

% if a point at 100% PER was chosen because there is a large step in PER across the
% sensitivity point, increment k0 until it is a the last 100% PER point
if y(k0) == 1,
    while (k0+1<length(y)) && (y(k0+1)==1)
        k0 = k0 + 1;
    end
end

while any(y(k0+2:length(y)) == 1),
    k0 = k0 + 1;
end

% Select points to use for curve fitting. Take up to 5 if possible, but
% otherwise take as many as possible. Do no use more points after k0 than
% from before (points after tend to introduce more error because the number
% of errors counted is less).
w = min(3, k0) - 1;
k = (k0-w) : min(k0+w, length(z));

% Fit a linear or quadratic polynomial and find the sensitivity level.
% Curve fitting with more than two points provides some smoothing of
% statistical noise in the results.
if length(k) > 1,
    P = polyfit(x(k),z(k)-z0, min(2,length(k)-1));
    r = roots(P);
    d = abs(r-x(k0));
    x0 = r(find(d == min(d), 1));
    if ~isreal(x0), x0 = x(k0); end
else
    x0 = x(k);
end

%% REVISIONS
% 2008-06-13 "while any(y(k0+2:length(y))) == 1" --> "while any(y(k0+2:length(y)) == 1)"
% 2008-11-25 Modified code to insure interpolation occurs at points that stradle y0
