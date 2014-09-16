function status = DwPhyLab_SetParameterData(Parameter, Data)
% status = DwPhyLab_SetParameterData(Parameter, Data)
%   Supplies user program data to the DwPhy module using the interface used for
%   device-specific parameters stored in non-volatile memory

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% Allow character version of the DwPhy.c enumeration labels
if ischar(Parameter)
    switch upper(Parameter),
        case 'DWPHY_PARAM_CHIPSET_ALIAS',      Parameter = 1;
        case 'DWPHY_PARAM_DEFAULTREGISTERS',   Parameter = 2;
        case 'DWPHY_PARAM_REGISTERSBYBAND',    Parameter = 3;
        case 'DWPHY_PARAM_REGISTERSBYCHANL',   Parameter = 4;
        case 'DWPHY_PARAM_REGISTERSBYCHANL24', Parameter = 5;
        case 'DWPHY_PARAM_MSRPWROFS56',        Parameter = 6;
        case 'DWPHY_PARAM_RXSENSITIVITY',      Parameter = 7;
        otherwise, error('Unrecognized Parameter Type');
    end
end

status = DwPhyMex('DwPhy_SetParameterData',Parameter, Data);

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_SetParameterData Status = 0x%s: %s',hexstatus, description);
    end
end
