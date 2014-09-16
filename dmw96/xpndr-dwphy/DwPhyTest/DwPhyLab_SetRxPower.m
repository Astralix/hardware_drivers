function DwPhyLab_SetRxPower(SigGen, dBm)
% Set the (ESG) signal generator output power for receiver tests
%
%    DwPhyLab_SetRxPower(dBm) sets the output power on the primary signal
%    generator to have the specified level. If the output is disabled, it
%    is automatically enabled. The function includes a delay to account for
%    amplitude settling.
%
%    DwPhyLab_SetRxPower(SigGen, dBm) sets the output power for the
%    selected signal generator. Generally used with SigGen = 2.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<2, 
    dBm = SigGen;
    SigGen = [];
end

% set the amplitude and enable the RF output
ESG = DwPhyLab_OpenESG(SigGen);
if isempty(ESG), error('Unable to open connection to ESG'); end

DwPhyLab_SendCommand(ESG, ':SOURCE:POWER:LEVEL:IMMEDIATE:AMPLITUDE %1.1fdBm',dBm);
DwPhyLab_SendCommand(ESG, ':OUTPUT:STATE ON');
DwPhyLab_CloseESG(ESG);

% delay 119 ms which is the maximum settling time for gain changes with
% automatic power search. ALC settling
pause(0.119);


