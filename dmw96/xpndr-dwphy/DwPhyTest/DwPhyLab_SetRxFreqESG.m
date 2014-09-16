function DwPhyLab_SetRxFreqESG(SigGen, FcMHz)
% Set the signal generator channel frequency
%    DwPhyLab_SetRxFreqESG(FcMHz) set the center frequency for the primary signal
%    generator to the specified center frequency FcMHz.
%
%    DwPhyLab_SetRxFreqESG(n, FcMHz) sets the center frequency for ESG n

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<2, 
    FcMHz = SigGen;
    SigGen = []; 
end

ESG = DwPhyLab_OpenESG(SigGen);
DwPhyLab_SendCommand(ESG, ':SOURCE:FREQUENCY:OFFSET:STATE OFF');
DwPhyLab_SendCommand(ESG, ':SOURCE:FREQUENCY:CW %5.9fGHz',FcMHz/1000);
DwPhyLab_SendQuery(ESG,'*OPC?');
DwPhyLab_CloseESG(ESG);
pause(0.06); % freq/amplitude settling on ESG
