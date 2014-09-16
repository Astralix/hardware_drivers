function DwPhyTest_SetBand(DualBand)
%DwPhyTest_SetBand(DualBand)
%
%   Selects the operating band
%       0 = 2.4 GHz only
%       1 = Dual band (2.4 + 5 GHz)   

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, DualBand = 0; end

switch upper(DualBand)
    
    case {0, 2, 2.4, 2400, 2412, '2.4GHZ','802.11B','802.11G'},
        DwPhyLab_Parameters('DualBand',0);
        
    case {1, 5, 5200, 'DUALBAND','DUAL BAND','DUAL-BAND','802.11B'}
        DwPhyLab_Parameters('DualBand',1);
        
    otherwise,
        error('Unrecognized Band Identifier');
end
