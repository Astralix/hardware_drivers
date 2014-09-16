function [status, description, result] = DwPhyLab_SendQuery(Address, QueryString, MaxLength)
%[status, description, result] = DwPhyLab_SendCommand(Address, QueryString, MaxLength)
%
%   Send a command to the device specified by Address. Valid address values
%   include 'ESG1','ESG2','E4440A','SCOPE'.
%
%[status, description, result] = DwPhyLab_SendQuery(ConnectionHandle, QueryString, MaxLength)
%
%   Send a command to the instrument addressed by the open ConnectionHandle. This is
%   not valid for the AWG520.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

if nargin < 2, QueryString = []; end
if nargin < 3, MaxLength = 16384; end

if isnumeric(Address),
    switch Address,
        case 1, Address = '1';
        case 2, Address = '2';
    end
end

if ischar(Address),
    [status, description, result] = SendQueryByAddress(Address, QueryString, MaxLength);
else
    [status, description, result] = SendQueryByHandle (Address, QueryString, MaxLength);
    if nargout == 0,
        if status ~= 0,
            error(description);
        end
    end
end

if (nargout < 2) && (status ~= 0)
    error('status = %d; %s',status,description'); % display error if status is not returned
end

if nargout == 1,
    status = result; % handle the case where only the response is requested
end
    

%% SendQueryByHandle
function [status, description, result] = SendQueryByHandle(h, QueryString, MaxLength)

S = whos('h');
switch S.class,

    case 'struct'
        [status,description,result] = agt_query(h, QueryString);
        
    case 'COM.LeCroy_ActiveDSOCtrl_1',
        if ~isempty(QueryString),
            QueryString = sprintf('%s\n',QueryString);

            if ~h.WriteString(QueryString,1),
                error('Write to scope failed.');
            end
        end
        result = h.ReadString(MaxLength);
        status = 0; description = '';
        
    case 'double'
        if ~isempty(QueryString)
            status = viWrite(h, QueryString);
            if status < 0,
                [dummy, description] = viStatusDesc(h, status);
                return
            end
        end
        [status, result] = viRead(h, MaxLength);
        [dummy, description] = viStatusDesc(h, status);
        
    otherwise,
        error('Unrecognized handle type');
end



%% SendQueryByAddress
function [status, description, result] = SendQueryByAddress(Address, QueryString, MaxLength)

switch upper(Address),

    %%% ESG #1 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'ESG1', 'ESG','1'},
        h = DwPhyLab_OpenESG(1);
        if isempty(h),
            error('Unable to open ESG #1');
        else
            [status, description, result] = SendQueryByHandle(h, QueryString, MaxLength);
        end
        DwPhyLab_CloseESG(h);

    %%% ESG #2 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'ESG2','2'},
        h = DwPhyLab_OpenESG(2);
        if isempty(h),
            error('Unable to open ESG #2');
        else
            [status, description, result] = SendQueryByHandle(h, QueryString, MaxLength);
        end
        DwPhyLab_CloseESG(h);
            
    %%% E4440A PSA %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'PSA', 'E4440A', 'SpectrumAnalyzer'},
        h = DwPhyLab_OpenPSA;
        if isempty(h),
            error('Unable to open E4440A PSA');
        else
            [status, description, result] = SendQueryByHandle(h, QueryString, MaxLength);
        end
        DwPhyLab_ClosePSA(h);
        
    %%% AWG520 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'AWG','AWG520'},
        error('Not supported');

    %%% LECROY SCOPE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'SCOPE','LECROY'},
        h = DwPhyLab_OpenScope;
        [status, description, result] = SendQueryByHandle(h, QueryString, MaxLength);
        DwPhyLab_CloseScope(h);
        
    %%% DEFAULT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    otherwise,
        if IgnoreErrors, return; end
        error('Unrecognized Address');
end

