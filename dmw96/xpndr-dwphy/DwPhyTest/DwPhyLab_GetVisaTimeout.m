function [status, description, timeout_setting] = DwPhyLab_GetVisaTimeout(h)
% [status, description] = DwPhyLab_SetVisaTimeout(h, timeout_setting)

S = whos('h');
switch S.class,

    case 'double'
        [status, timeout_setting] = viGetAttribute(h, 'VI_ATTR_TMO_VALUE');
        if nargout > 1,
            [dummy, description] = viStatusDesc(h, status);
        end
    
    case 'struct'
        [status, description, timeout_setting] = agt_getVisaTimeout(h);
        
    otherwise,
        error('Unrecognized handle type');
end

if nargout < 2,
    if status ~= 0, error('Status = %d: %s',status,description); end
    if nargout ==1,
        status = timeout_setting; 
    end
end
    
    
