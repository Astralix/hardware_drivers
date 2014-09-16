function DwPhyCheck( status )
% Error Handler for DwPhyMex Return Status

if nargin == 1,
    if status ~= 0,
        hexstatus = dec2hex(status + 2^31, 8);
        description = DwPhyLab_ErrorDescription(status);
        error(' DwPhy Status = 0x%s: %s',hexstatus, description);
    end
end

