function DwPhyLab_DisableESG(SigGen)
% DwPhyLab_Disable(SigGen)

if nargin<1, SigGen = 'ESG1'; end
DwPhyLab_SendCommand(SigGen, ':OUTPUT:STATE OFF');
