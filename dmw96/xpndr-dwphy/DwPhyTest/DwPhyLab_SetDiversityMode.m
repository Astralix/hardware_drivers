function DwPhyLab_SetDiversityMode(PathSel)
% Select the PHY diversity mode
%    DwPhyLab_SetDiversityMode(Mode) selects the specified diversity mode.
%    Valid settings include (1 = RXA, 2 = RXB, 3 = Both receivers, MRC).

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% Convert descriptive strings an equivalent enumeration
if ischar(PathSel),
    switch upper(PathSel),
        case {1,'A'}, PathSel = 1;
        case {2,'B'}, PathSel = 2;
        case {3,'Both','A+B','AB','MRC','FULL','DUALRX'}, PathSel = 3;
        otherwise, error('Unrecognized value');
    end
end

status = DwPhyMex('DwPhy_SetDiversityMode',PathSel);
if(status ~= 0), 
    error('DwPhy_SetDiversityMode() Status = %s\n',dec2hex(status + 2^31,8)); 
end
