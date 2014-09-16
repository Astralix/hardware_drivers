function DwPhyLab_WriteRegister(Address,Value,Value2)
% Write a PHY register or register field
%
%    DwPhyLab_WriteRegister(Address, Data) writes the Data value to the
%    specified PHY register where Address is valid for 0 to 1023.
%
%    DwPhyLab_WriteRegister(Address, BitRangeString, Data) writes the Data
%    values to the specified bit range at the given address. BitRangeString
%    specifies the bit field as in '7:0'.
%
%    DwPhyLab_WriteRegister(Address, BitPosition, Data) writes the Data bit
%    at the specified bit position (7 down to 0) at the given address.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin == 1,

    % For a single argument, assume an array of address, data pairs
    for i = 1:size(Address, 1),
        DwPhyMex('OS_RegWrite', Address(i,1), Address(i,2));
    end
        
elseif nargin == 2,
    
    if ischar(Value), Value=bin2dec(Value); end
    DwPhyMex('OS_RegWrite', Address, Value);
    
elseif nargin == 3, 
    
    BitRange = Value; Value = Value2;
    if ischar(Value), Value=bin2dec(Value); end
    
    if ischar(BitRange),
        k = strfind(BitRange,':');
        if isempty(k),
            BitH = str2double(BitRange);
            BitL = str2double(BitRange);
        else
            BitH = str2double( BitRange(1:(k-1)) );
            BitL = str2double( BitRange(k+1:length(BitRange)) );
        end
    else
        BitH = BitRange;
        BitL = BitRange;
    end

    Mask = 2^(BitH - BitL + 1) - 1;
    if Value > Mask,
        error('Value too large for field');
    end
   
    NumBytes = ceil((1+BitH)/8);
    ReadMask = bitxor(2^(8*NumBytes)-1, bitshift(Mask, BitL));

    X = bitand(DwPhyLab_ReadRegister(Address), ReadMask);
    X = bitor (X, bitshift(Value, BitL));
    
    for i=1:NumBytes,
        x = bitand(255, bitshift(X, -8*(NumBytes-i)) );
        DwPhyMex('OS_RegWrite', Address+i-1, x);
    end
    
else
    
    error('Incorrect number of arguments');

end
