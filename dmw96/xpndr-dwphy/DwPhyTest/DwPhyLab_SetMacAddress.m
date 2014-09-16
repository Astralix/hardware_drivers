function DwPhyLab_SetMacAddress( Addr )
% DwPhyLab_SetMACAddress(Station, Addr)
%     Addr can be numerical or a hex string

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

DwPhyMex('RvClientWriteMacAddress',MacAddr);
