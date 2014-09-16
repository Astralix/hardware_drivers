function [status, description] = DwPhyLab_SendCommand(Address, varargin)
%DwPhyLab_SendCommand(Address, Command)
%
%   Send a command to the device specified by Address. Valid address values
%   include 'ESG1','ESG2','E4440A','AWG520','SCOPE'.
%
%[status, description] = DwPhyLab_SendCommand(ConnectionHandle, Command)
%   Send a command to the instrument addressed by the open ConnectionHandle. This is
%   only valid for ESG, PSA, and LeCroy scope devices

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

if nargin < 2, error('Incorrect number of arguments'); end
if numel(varargin) < 1, error('Insufficient number of arguments'); end

Command = sprintf(varargin{:});

if isnumeric(Address),
    switch Address,
        case 1, Address = '1';
        case 2, Address = '2';
    end
end

if ischar(Address),
    SendCommandByAddress(Address, Command);
else
    [status, description] = SendCommandByHandle(Address, Command);
    if nargout == 0,
        if status ~= 0,
            error(description);
        end
    end
end

%% SendCommandByHandle
function [status, description] = SendCommandByHandle(h, Command)

S = whos('h');
switch S.class,

    case 'struct'
        [status, description] = agt_sendcommand(h, Command);
        
    case 'COM.LeCroy_ActiveDSOCtrl_1',
        result = h.WriteString(sprintf('%s\n',Command), 1);
        if result == 0,
            status = -1; description = 'WriteString to LeCroy scope failed';
        else
            status = 0; description = '';
        end
        
    case 'double'
        status = viWrite(h, Command);
        [dummy, description] = viStatusDesc(h, status);
        
    otherwise,
        error('Unrecognized handle type');
end



%% SendCommandByAddress
function SendCommandByAddress(Address, Command)

switch upper(Address),

    %%% ESG #1 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {1, 'ESG1', 'ESG','1'},
        h = DwPhyLab_OpenESG(1);
        if isempty(h),
            error('Unable to open ESG #1');
        else
            [status, description] = SendCommandByHandle(h, Command);
            if status ~= 0,
                error('ESG1: status = %d, "%s"',status,description);
            end
        end
        DwPhyLab_CloseESG(h);

    %%% ESG #2 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {2, 'ESG2','2'},
        h = DwPhyLab_OpenESG(2);
        if isempty(h),
            error('Unable to open ESG #2');
        else
            [status, description] = SendCommandByHandle(h, Command);
            if status ~= 0,
                error('ESG2: status = %d, "%s"',status,description);
            end
        end
        DwPhyLab_CloseESG(h);
            
    %%% E4440A PSA %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'PSA', 'E4440A', 'SpectrumAnalyzer'},
        h = DwPhyLab_OpenPSA;
        if isempty(h),
            error('Unable to open E4440A PSA');
        else
            [status, description] = SendCommandByHandle(h, Command);
            if status ~= 0,
                error('E4440A: status = %d, "%s"',status,description);
            end
        end
        DwPhyLab_ClosePSA(h);
        
    %%% AWG520 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'AWG','AWG520'},
        h = DwPhyLab_OpenAWG520;
        DwPhyLab_WriteAWG520(h, Command);
        DwPhyLab_CloseAWG520(h);

    %%% LECROY SCOPE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case {'SCOPE','LECROY'},
        h = DwPhyLab_OpenScope;
        SendCommandByHandle(h, Command);
        DwPhyLab_CloseScope(h);
        
    %%% DEFAULT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    otherwise,
        if IgnoreErrors, return; end
        error('Unrecognized Address');
end

