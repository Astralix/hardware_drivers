function Value = DwPhyLab_ReadRegister(Address, BitRange)
% Read a PHY register or register field
%
%    Data = DwPhyLab_ReadRegister returns the value for baseband registers
%
%    Data = DwPhyLab_ReadRegister(Address) returns the value for the
%    specified PHY register where Address is valid for 0 to 1023.
%
%    Data = DwPhyLab_ReadRegister(Address, BitRangeString) returns the
%    value for the specified bit range at the given address. BitRangeString
%    specifies the bit field as in '7:0'.
%
%    Data = DwPhyLab_ReadRegister(Address, BitPosition) returns the value
%    at the specified bit position (7 down to 0) at the given address.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

if nargin == 0,
    
    switch DwPhyLab_ReadRegister(1)
        case {5,6}, A = 0:255;
        case {6,7}, A = [0:255; 512:1023];
        otherwise, error('Unsupported baseband ID');
    end
    
    Value = zeros(numel(A), 2);
    for i = 1:numel(A),
        Value(i,1) = A(i);
        Value(i,2) = DwPhyMex('OS_RegRead',A(i));
    end
    
elseif nargin == 1,
    
    Value = DwPhyMex('OS_RegRead',Address);
    
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
        x = DwPhyMex('OS_RegRead', Address+i-1);
        X = bitor( X, bitshift(x, 8*(NumBytes-i)) );
    end
    Mask = bitshift(2^(BitH-BitL+1) - 1, BitL);
    Value = bitshift( bitand(X, Mask), -BitL);
else
    
    error('Incorrect number of arguments');
    
end
