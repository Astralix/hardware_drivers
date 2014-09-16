function Data = wiPHY_ReadRegister(Address, BitRange)
% Read a PHY/baseband register or register field
%
%    Data = wiPHY_ReadRegister(Address) returns the value for the specified register.
%
%    Data = wiPHY_ReadRegister(Address, BitRangeString) returns the values for the
%    specified bit range at the given address. BitRangeString specifies the bit field as 
%    in '7:0'.
%
%    Data = wiPHY_ReadRegister(Address, BitPosition) returns the value at the specified
%    bit position (0 is LSB) at the given address.

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

if nargin == 1,
    Data = wiseMex('wiPHY_ReadRegister',Address);
    
elseif nargin == 2,
    if ischar(BitRange),
        k = strfind(BitRange,':');
        BitH = str2double( BitRange(1:(k-1)) );
        BitL = str2double( BitRange(k+1:length(BitRange)) );
    else
        BitH = BitRange;
        BitL = BitRange;
    end

    X = 0;
    NumBytes = ceil((1+BitH)/8);
    for i = 1:NumBytes,
        x = wiseMex('wiPHY_ReadRegister',Address+i-1);
        X = bitor( X, bitshift(x, 8*(NumBytes-i)) );
    end
    Mask = bitshift(2^(BitH-BitL+1) - 1, BitL);
    Data = bitshift( bitand(X, Mask), -BitL);
else
    error('Incorrect number of arguments');
end
