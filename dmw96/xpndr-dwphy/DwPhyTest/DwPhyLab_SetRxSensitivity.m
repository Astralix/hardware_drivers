function status = DwPhyLab_SetRxSensitivity(PrdBm)
%Status = DwPhyLab_SetRxSensitivity(PrdBm)
%   Set the minimum sensitivity limit to PrdBm.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

status = DwPhyMex('DwPhy_SetRxSensitivity',PrdBm);

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_SetRxSensitivity Status = 0x%s: %s',hexstatus, description);
    end
end
