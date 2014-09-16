function [S,Y] = wiseMex_GetTraceState
% [S,Y] = wiseMex_GetTraceState;
%         Retrieve the PHY-specific trace and state data

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

S = []; Y = [];

Method = wiseMex('wiPHY_ActiveMethod');

switch Method,
    case 'NevadaPHY', Method = 'NevadaPhy';
    case 'TamalePHY', Method = 'TamalePhy';
end

CommandStr = sprintf('[S,Y] = %s_GetTraceState;', Method);
eval(CommandStr);

