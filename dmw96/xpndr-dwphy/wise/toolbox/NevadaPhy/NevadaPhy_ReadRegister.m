function Value = NevadaPhy_ReadRegister(varargin)
% Read a Nevada baseband register or register field
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

Value = wise_ReadRegisterCore(@ReadFn, varargin{:});

%% ReadFn
function Value = ReadFn(Address)
Value = wiseMex('Nevada_ReadRegister',Address);

