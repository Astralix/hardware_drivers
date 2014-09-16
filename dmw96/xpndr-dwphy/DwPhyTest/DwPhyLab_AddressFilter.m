function status = DwPhyLab_AddressFilter(Enable)
%Status = DwPhyLab_AddressFilter(Enable)
%   Enable or disable the baseband nuisance packet address filter. When enabled,
%   unicast packets with addresses not matching the value set with
%   DwPhyLab_SetStationAddress may be filtered by the baseband.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

status = DwPhyMex('DwPhy_AddressFilter',Enable);

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_AddressFilter Status = 0x%s: %s',hexstatus, description);
    end
end
