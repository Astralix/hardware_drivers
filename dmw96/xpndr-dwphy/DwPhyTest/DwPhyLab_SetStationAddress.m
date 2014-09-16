function status = DwPhyLab_SetStationAddress(Addr)
%Status = DwPhyLab_SetStationAddress(Addr)
%   Set the station MAC address to be used by the baseband address filter

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% if address is supplies in hex format, convert it to decimal
if(ischar(Addr))
    Addr = hex2dec(Addr);
end

% convert the an array of unsigned bytes to match the DwPhyMex convention
MacAddr = zeros(6,1,'uint8');
for i=1:6,
    MacAddr(i) = uint8( mod(floor(Addr / 2^(8*(i-1))), 256) );
end

status = DwPhyMex('DwPhy_SetStationAddress',MacAddr);

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_AddressFilter Status = 0x%s: %s',hexstatus, description);
    end
end

        