function StationName = PhyLab_StationName(UseComputerNameForUnknown)
% StationName = PhyLab_StationName
%
%       Return the common name for the test station. This is used to map from the IT
%       centric naming convention to a format more useful to the end user.

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

if nargin<1, UseComputerNameForUnknown = 1; end

ComputerName = upper(getenv('COMPUTERNAME'));

switch (ComputerName)
    
    case 'IL-PCXP-10788', StationName = 'ILTestStation1';
%    case 'MN-PCXP-07048', StationName = 'MNTestStation2';
%    case 'MN-PCXP-07076', StationName = 'MNTestStation3';
%    case 'MN-LTXP-07006', StationName = 'MNPacketSniffer';

    otherwise, 
        if UseComputerNameForUnknown,
            StationName = ComputerName;
        else
            StationName = '';
        end

end

