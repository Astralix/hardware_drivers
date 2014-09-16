function status = DwPhyLab_Initialize(PlatformID)
% status = DwPhyLab_Initialize(PlatformID)
%     Call DwPhy_Initialize which resets the PHY and loads the default 
%     register values. The available PlatformID values are
%           0 : DW52/74 ASIC (default)
%           1 : ARM Versatile + DMW96 FPGA

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin < 1, 
    PlatformID = DwPhyLab_Parameters('PlatformID');
    if isempty(PlatformID), PlatformID = 0; end;
end

status = DwPhyMex('DwPhy_Initialize', PlatformID);

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,

        hexstatus = dec2hex(status + 2^31, 8);
        description = DwPhyLab_ErrorDescription(status);
        error(' DwPhy_Initialize Status = 0x%s: %s',hexstatus, description);
        
    end
    clear status; % prevent output of a zero to the console
end

%% REVISIONS
% 100402BB - Add PlatformID