function Result = DwPhyLab_RadioIsRF22(data)
% Result = DwPhyLab_RadioIsRF22        Returns 1 if the RF chip is RF22; otherwise returns 0. Read from RF registers.
% Result = DwPhyLab_RadioIsRF22(data)  Returns 1 if the RF chip is RF22; otherwise returns 0. Read from data.RadioID.

% Copyright 2010-2011 DSP Group, Inc., All Rights Reserved.

RF22IDs = 65:72;
if nargin == 0
    if any(DwPhyLab_ReadRegister(256+1) == RF22IDs), Result = 1;
    else                                             Result = 0;
    end
else
    if any(data.RadioID == RF22IDs), Result = 1;
    else                             Result = 0;
    end    
end
