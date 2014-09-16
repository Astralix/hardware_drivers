function DwPhyLab_WriteUserReg( UserReg )
% DwPhyLab_WriteUserReg(UserReg)
%
%       Write a list of registers and register fields specified in the cell 
%       UserReg. Each element of UserReg contains either 2 or 3 cells. The
%       cells are {Addr},{Data} for two elements or {Addr},{Field},{Data}
%       for three elements where the bit field is a character array.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, return; end;
if isempty(UserReg), return ; end

% For single cell entries, repack as a cell array
if ~iscell(UserReg{1})
    UserReg = {UserReg};
end

% Loop thru the cell array writing per the supplied fields
for i=1:length(UserReg),

    switch length(UserReg{i}),
        case 2, DwPhyLab_WriteRegister(UserReg{i}{1}, UserReg{i}{2});
        case 3, DwPhyLab_WriteRegister(UserReg{i}{1}, UserReg{i}{2}, UserReg{i}{3});
        otherwise,
            error('Incorrect number of elements in UserReg{%d}',length(UserReg));
    end

end
