function RegField = wise_GetRegisterField(ReadFn, FieldName)
% RegFieldStruct = wise_GetRegisterField(ReadFn, FieldName)
%
%     Returns the parameter structure for a register field using one of the baseband
%     register map functions selected based on part ID (read of register 1).

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

BasebandID = ReadFn(1);
switch BasebandID,
    case {4   }, RegMapFn = @Dakota_RegisterMap;
    case {5, 6}, RegMapFn = @Mojave_RegisterMap;
    case {7, 8}, RegMapFn = @Nevada_RegisterMap;
    otherwise, error('No register mapped defined for baseband ID %d',BasebandID);
end

RegField = RegMapFn(FieldName);
