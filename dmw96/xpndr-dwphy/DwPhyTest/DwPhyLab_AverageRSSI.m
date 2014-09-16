function AvgRSSI = DwPhyLab_AverageRSSI(FilterType)
% AvgRSSI = DwPhyLab_AverageRSSI(FilterType)
%
%      Returns an array of two values, AvgRSSI = [RSSI0 RSSI1],
%      corresponding to paths 0 and 1 (RXA and RXB). Units are dB.
%
%      The measurement uses the inherent calibration values from the 
%      baseband (Pwr100dBm). If AGC is enabled, it will also reflect 
%      predicted gain steps. The units are independent of the baseband
%      RSSIdB parameter. The basic measurement collect multiple consecutive
%      raw power measurements (not synchronized to packet activity) and
%      performs an averge.
%
%      The following filters may be applied
%      0 = none (default)
%      1 = remeasure if RXA+RXB > AvgRSSI + 12 dB

% Retrieve raw RSSI measurements from the R-VWAL server and de-interleave
% the RXA and RXB measurements.

if nargin<1, FilterType = 0; end

% Retrieve the baseband RSSI scaling
RSSIdB = DwPhyLab_ReadRegister(197, 4);
A = 3/4 + 1/4*RSSIdB; % units are 3/4 dB for RSSIdB=0; otherwise 1 dB

done = 0; count = 0;

while ~done,

    % Read raw RSSI values
    X = DwPhyMex('RvClientRxRSSIValues');
    X0 = X(1:2:length(X)); % Path 0 (RXA)
    X1 = X(2:2:length(X)); % Path 1 (RXB)
    count = count + 1;

    switch FilterType,
        case 0, done = 1;
        case 1, Z = X0+X1; done = all(Z <= mean(Z)+12);
        otherwise, error('Unsupported FilterType');
    end
    if ~done, 
        fprintf('!done: max(Z)-mean(Z) = %1.1f\n',max(Z)-mean(Z)); 
    end
    if count == 20 && ~done,
        done = 1;
        warning('DwPhyLab_AverageRSSI:Count','Unable to meet filter requirements in 20 tries');
    end
end

% Compute average RSSI as a true power measure (not a dB average)
AvgRSSI(1) = 10*log10( mean( 10.^(A*X0/10) ) );
AvgRSSI(2) = 10*log10( mean( 10.^(A*X1/10) ) );

%% REVISIONS
% 2008-06-10 Read register 197 before retrieving RSSI values. This just allows more
%            settling time for any preceding operation.
%            Added FilterType=1 to allow retry on large deviations in RSSI. This is
%            useful for noise power or continuous signal measurements
             