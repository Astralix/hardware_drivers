function DwPhyLab_CloseAWG520(h)
%DwPhyLab_CloseAWG520(handle)

if(isnumeric(h) && h)
    viClose(h);
elseif(ischar(h))
    dummy_rc = mexLab_AWG520('closeAWG520'); %#ok<NASGU>
end
