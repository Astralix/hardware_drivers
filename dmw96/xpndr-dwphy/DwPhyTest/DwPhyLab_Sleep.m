function DwPhyLab_Sleep
% Sleep the PHY. This function effectively sets DW_PHYEnB = 1

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

status = DwPhyMex('DwPhy_Sleep');

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,

        hexstatus = dec2hex(status + 2^31, 8);
        description = DwPhyLab_ErrorDescription(status);
        error(' DwPhy_Sleep Status = 0x%s: %s',hexstatus, description);
        
    end
    clear status; % prevent output of a zero to the console
end

%% REVISIONS
% 2008-07-28 Added handling for status codes on call to DwPhy_Sleep