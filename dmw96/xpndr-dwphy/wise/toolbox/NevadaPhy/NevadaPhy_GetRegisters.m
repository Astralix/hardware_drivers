function RegList = NevadaPhy_GetRegisters
%RegList = NevadaPhy_SetRegisters
%
%   Returns Nevada baseband registers (0-255) in RegList where the first
%   column is the PHY address, and the second is the data.

%   Developed by Barrett Brickner
%   Copyright 2009 DSP Group, Inc. All rights reserved.

% Error Checking
if nargin~=0, error('Incorrect number of input parameters'); end

DataArray = wiseMex('Nevada_WriteRegisterMap()');
RegList(:,2) = DataArray;
RegList(:,1) = (1:length(DataArray))'-1;
