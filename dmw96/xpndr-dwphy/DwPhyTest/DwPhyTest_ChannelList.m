function [fcMHz, Band] = DwPhyTest_ChannelList(DualBand)
%[fcMHz, Band] = DwPhyTest_ChannelList(DualBand)
%
%   Return a list of supported channels and the corresponding band enumeration. This
%   function is not authoritative. It is configured to match the driver channel
%   support but must be modified in case of changes.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1,
    DualBand = DwPhyLab_Parameters('DualBand');
    if isempty(DualBand), DualBand = 1; end
end

if DualBand,
    fcMHz = [
        2412:5:2472, 2484, ...
        4920:20:4980, 5040:20:5080, 5170:10:5240, 5260:20:5320, 5500:20:5700, 5745:20:5805
        ];
else
    fcMHz = [ 
        2412:5:2472, 2484, ...
        ];
end

Band  = zeros(size(fcMHz));
k = find(               fcMHz <=2484); Band(k) = 8;
k = find(fcMHz>= 4920 & fcMHz <=4980); Band(k) = 0;
k = find(fcMHz>= 5040 & fcMHz <=5080); Band(k) = 1;
k = find(fcMHz>= 5170 & fcMHz <=5240); Band(k) = 2;
k = find(fcMHz>= 5260 & fcMHz <=5320); Band(k) = 3;
k = find(fcMHz>= 5500 & fcMHz <=5500); Band(k) = 4;
k = find(fcMHz>= 5520 & fcMHz <=5620); Band(k) = 5;
k = find(fcMHz>= 5640 & fcMHz <=5700); Band(k) = 6;
k = find(fcMHz>= 5745               ); Band(k) = 7;
