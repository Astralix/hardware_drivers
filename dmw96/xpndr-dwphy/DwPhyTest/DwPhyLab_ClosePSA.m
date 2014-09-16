function DwPhyLab_ClosePSA(h)
%DwPhyLab_ClosePSA -- Close an open PSA session
%
%  DwPhyLab_ClosePSA(H) closes the Agilent PSA ession specified by the
%  VISA or ActiveX control handle H.

if(isnumeric(h) && h)
    viClose(h);
else
    clear h;
    clear agt_sgIOmx;
end
