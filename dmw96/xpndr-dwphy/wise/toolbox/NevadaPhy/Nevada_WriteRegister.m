function Nevada_WriteRegister(varargin)
% Write a Nevada baseband register or register field
%
%    Data = NevadaPhy_ReadRegister(Address) returns the value for the
%    specified PHY register where Address is valid for 0 to 1023.
%
%    Data = NevadaPhy_ReadRegister(Address, BitRangeString) returns the
%    value for the specified bit range at the given address. BitRangeString
%    specifies the bit field as in '7:0'.
%
%    Data = NevadaPhy_ReadRegister(Address, BitPosition) returns the value
%    at the specified bit position (7 down to 0) at the given address.

% Written by Barrett Brickner
% Copyright 2009 DSP Group, Inc., All Rights Reserved.

wise_WriteRegisterCore(@ReadWriteFn, varargin{:});

%% ReadWriteFn
function Data = ReadWriteFn(Address, Value)
if nargin == 1,
    Data = wiseMex('Nevada_ReadRegister',Address);
else
    wiseMex('Nevada_WriteRegister',Address, Value);
end
