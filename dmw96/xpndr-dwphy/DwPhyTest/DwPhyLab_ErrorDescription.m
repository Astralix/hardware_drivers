function description = DwPhyLab_ErrorDescription(status)
% description = DwPhyLab_ErrorDescription(status)
%
%   Returns an error description for the error code produced by calls to
%   functions in DwPhy.c

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if status == 0,
    description = 'Success';

elseif status < 0

    switch (status - (hex2dec('011B0000') - 2147483647 - 1)),
    
        case hex2dec('0000'), description = 'Generic error';

        case hex2dec('0001'), description = 'Error in parameter 1';
        case hex2dec('0002'), description = 'Error in parameter 2';
        case hex2dec('0003'), description = 'Error in parameter 3';
        case hex2dec('0004'), description = 'Error in parameter 4';
        case hex2dec('0005'), description = 'Error in parameter 5';

        case hex2dec('0011'), description = 'Unsupported platform';
        case hex2dec('0012'), description = 'Unsupported chipset';
        case hex2dec('0013'), description = 'Unsupported baseband';
        case hex2dec('0014'), description = 'Unsupported radio';
        case hex2dec('0015'), description = 'Unsupported channel';
        case hex2dec('0016'), description = 'Undefined case';
        case hex2dec('0020'), description = 'PLL not locked';

        case hex2dec('0030'), description = 'Memory allocation failed';
        case hex2dec('0031'), description = 'OsCopyFromUser failed';
        case hex2dec('0040'), description = 'DwPhyEnable failed';

        case hex2dec('00F1'), description = 'Unknown function';

        otherwise, description = 'Unknown Error';
    end
    
else
    description = 'Position status coding...unknown warning?';
end