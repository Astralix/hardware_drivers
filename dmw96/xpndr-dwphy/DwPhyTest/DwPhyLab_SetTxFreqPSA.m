function DwPhyLab_SetTxFreqPSA(PSA, FcMHz)
% Set the spectrum anaylzer center frequency
%    DwPhyLab_SetTxFreqPSA(FcMHz) set the center frequency for the spectrum analyzer
%    to the specified center frequency FcMHz.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin == 1,
    DwPhyLab_SendCommand('PSA',':SENSE:FREQUENCY:CENTER %5.3fGHz',PSA/1000);
elseif nargin == 2, 
    DwPhyLab_SendCommand(PSA,  ':SENSE:FREQUENCY:CENTER %5.3fGHz',FcMHz/1000);
else
    error('Incorrect number of arguments');
end