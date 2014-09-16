function [status, description] = DwPhyLab_SetVisaTimeout(h, timeout_setting)
% [status, description] = DwPhyLab_SetVisaTimeout(h, timeout_setting)

S = whos('h');
switch S.class,

    case 'double'
        [status] = viSetAttribute(h, 'VI_ATTR_TMO_VALUE', timeout_setting);
        if nargout > 1,
            [dummy, description] = viStatusDesc(h, status);
        end
    
    case 'struct'
        [status, description] = agt_setVisaTimeout(h, timeout_setting);
        
    otherwise,
        error('Unrecognized handle type');
end
