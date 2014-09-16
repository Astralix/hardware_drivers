function DwPhyLab_CloseESG(h)
%DwPhyLab_CloseESG -- Close an open ESG/PSA session
%
%  DwPhyLab_CloseESG(H) close the Agilent ESG/PSA ession specified by the
%  VISA or ActiveX control handle H.

if(isnumeric(h) && h)
    viClose(h);
else
    clear h;
    clear agt_sgIOmx;
end
