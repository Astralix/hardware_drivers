function DwPhyLab_WriteAWG520(h, msg)
%DwPhyLab_WriteAWG520(h, msg)

if(isnumeric(h) && h)
    ok = (viWrite(h,msg) >= 0);
elseif(ischar(h))
    status = mexLab_AWG520('writeAWG520',msg);
    ok = (status==0);
else
    error('Invalid handle');
end

if ~ok, error('Instrument Write Failed'); end
