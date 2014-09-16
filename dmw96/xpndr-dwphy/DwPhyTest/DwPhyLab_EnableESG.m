function DwPhyLab_EnableESG(SigGen)

if nargin<1, SigGen = 1; end
DwPhyLab_SendCommand(SigGen, ':OUTPUT:STATE ON');
