function RegList = Dakota_GetRegisters
%RegList = Dakota_SetRegisters
%
%   Usage:  Returns Dakota PHY registers (0-255) in the RegList where the
%   first column is the PHY address and the second is the data.

%   Developed by Barrett Brickner
%   Copyright 2002 Bermai, Inc. All rights reserved.

% Error Checking
if(nargin~=0) error('Incorrect number of input parameters'); end

DataArray = wiseMex('Dakota_WriteRegisterMap()');
RegList(:,2) = DataArray;
RegList(:,1) = (1:length(DataArray))'-1;
