function [ok, msg] = DwPhyLab_ReadAWG520(h)
%[ok, msg] = DwPhyLab_ReadAWG520(h)

if nargout<2, error('Insufficient Number of Output Arguments'); end

if(isnumeric(h) && h)
    [status, msg] = viRead(h, 1024);
    ok = (status==0);
elseif(ischar(h))
    [status, msg] = mexLab_AWG520('readAWG520');
    ok = (status>=0);
    if ~ok, warning('status = %d\n',status); end
else
    error('Invalid AWG520 handle');
end
