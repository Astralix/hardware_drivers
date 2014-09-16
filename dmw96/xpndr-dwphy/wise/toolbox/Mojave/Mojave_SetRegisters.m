function Mojave_SetRegisters(RegList)
%Mojave_SetRegisters(RegList)
%
%   Usage:  Sets all values of the the Mojave baseband in the WiSE model
%           to values in the array. The address is the array index taken 
%           as a zero-based value. The DataArray must contain exactly 256 
%           values.

%   Developed by Barrett Brickner
%   Copyright 2002 Bermai, Inc. All rights reserved.

% Error Checking
if(nargin~=1) error('Incorrect number of input parameters'); end
[m,n] = size(RegList);
if(m ~= 256) error('RegList must have 256 rows'); end;
if(n==1)
    DataArray = RegList;
elseif(n==2)
    DataArray = RegList(:,2);
else
    error('Too many columns in RegList');
end
wiseMex('Mojave_ReadRegisterMap()',DataArray);
