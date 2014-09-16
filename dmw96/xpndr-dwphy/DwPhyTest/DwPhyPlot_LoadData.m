function [X, N] = DwPhyPlot_LoadData( arg )
% DwPhyPlot_LoadData - Load an array of data structures. This function is
% typically called from one of the other DwPhyPlot functions.
%
% X = DwPhyPlot_LoadData(data)
% X = DwPhyPlot_LoadData(filename)
% X = DwPhyPlot_LoadData(filelist)

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if isstruct(arg),
    X{1} = arg;
    
elseif ischar(arg),
    S = load(arg);
    X{1} = S.data;
    
elseif iscell(arg),
    X = cell(length(arg), 1);
    for i=1:length(arg),
        S = load(arg{i});
        X{i} = S.data;
    end
    
else
    error('Unrecognized data type');
end

N = length(X);