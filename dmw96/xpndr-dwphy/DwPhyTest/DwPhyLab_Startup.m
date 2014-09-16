function status = DwPhyLab_Startup
% Start the DwPhy module. This function is not normally used for testing. 
% It is included for the purpose of performing regression testing on the 
% DwPhy module itself.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

status = DwPhyMex('DwPhy_Startup');

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_Startup Status = 0x%s: %s',hexstatus, description);
    end
end

