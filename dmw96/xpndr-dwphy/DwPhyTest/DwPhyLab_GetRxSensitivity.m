function dBm = DwPhyLab_GetRxSensitivity
%dBm = DwPhyLab_GetRxSensitivity
%   Retrieve the current programmed sensitivity limit

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

dBm = DwPhyMex('DwPhy_GetRxSensitivity');
