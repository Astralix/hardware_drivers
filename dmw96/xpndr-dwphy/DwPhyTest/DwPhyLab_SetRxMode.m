function DwPhyLab_SetRxMode(RxMode)
% Select the receiver mode (modulation compatibility)
%    DwPhyLab_SetRxMode(Mode) selects the specified baseband receiver mode.
%    Valid settings include (1 = 802.11a, 2 = 802.11b, 3 = 802.11g, 5 = 802.11n OFDM,
%                            7 = 802.11n OFDM + DSSS/CCK)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% Convert descriptive strings an equivalent enumeration
if ischar(RxMode),
    switch upper(RxMode),
        case '802.11A', RxMode = 1;
        case '802.11B', RxMode = 2;
        case '802.11G', RxMode = 3;
        case '802.11N', RxMode = 7;
        otherwise, error('Unrecognized value');
    end
end

status = DwPhyMex('DwPhy_SetRxMode',RxMode);
if(status ~= 0), 
    description = DwPhyLab_ErrorDescription(status);
    hexstatus = dec2hex(status + 2^31, 8);
    error('DwPhy_SetRxMode() Status = %s: %s\n',hexstatus, description); 
end

%% REVISIONS
% 2010-04-19 Add '802.11N' as a valid RxMode string (maps to RxMode = 7)