function DwPhyLab_EvaluateVarargin(varargin, ArgList, AllowNumeric)
%Evaulate a variable argument list in the calling function's workspace
%
%   DwPhyLab_EvaulateVarargin(varargin, ArgList, AllowNumeric) parses the variable
%   argument cell array and evaulates strings in the caller's workspace. Labels
%   supplied in the optional ArgList cell array are used as variable assignments if
%   they appears as fields in a structure passed in varargin. Numeric values are
%   assigned to the variable labels in ArgList if AllowNumeric = 1. Additionally, any
%   waitbar handle is automatically assigned to hBar.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, varargin = {}; end
if nargin<2, ArgList = {}; end
if nargin<3, AllowNumeric = 0; end

for i=1:length(varargin),

    % handle assignments passed as character strings
    if iscell(varargin{i})
        for j=1:length(varargin{i}),
            evalin('caller', sprintf('%s;',varargin{i}{j}) );
        end
    elseif ischar(varargin{i})
        evalin('caller', sprintf('%s;',varargin{i}) );

    % handle numeric arguments including a waitbar to be assigned to hBar
    elseif isnumeric(varargin{i}),
        ArgumentIsWaitBar = 0;
        if ishandle(varargin{i}),
            if isfield(get(varargin{i}),'Tag'),
                if strcmp( get(varargin{i},'Tag'),'TMWWaitbar');
                    ArgumentIsWaitBar = 1;
                end
            end
        end
        if ArgumentIsWaitBar,
            assignin('caller', 'hBar', varargin{i});
        elseif ~isempty(ArgList) && AllowNumeric,
            if i<=length(ArgList),
                assignin('caller', ArgList{i}, varargin{i});
            else
                error('Too many arguments');
            end
        elseif isempty(varargin{i}),
            % do nothing...this allows empty variables as placeholders for others
        else
            error('Unrecognized numeric argument in position %d',i);
        end

    % handle data passed as element of a struture
    elseif isstruct(varargin{i}) && ~isempty(ArgList),
        for k=1:length(ArgList),
            if isfield(varargin{i},ArgList{k}),
                assignin('caller', ArgList{k}, varargin{i}.(ArgList{k}));
            end
        end

    % format not recognized
    else
        error('Unrecognized argument type in position %d',i);
    end
end

%% REVISIONS
% 2008-06-08 Always empty variable when AllowNumeric = 0 as option placeholder
