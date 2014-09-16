function s = DwPhyPlot_Command(N, plotstr, xstr, ystr, fmt)
% s = DwPhyPlot_Command(N, plotstr, xstr, ystr, fmt)
%   Returns an expression for plotting N (x,y) curves using the plotting
%   function specified in plotstr. The x and y terms are specified by xstr
%   and ystr respectively where each may optionally contain %d to be
%   replaced by an index from 1:N. If not format string fmt is specfied,
%   the default is '.-'.
%
% The same expression called without an output argument evaluate in the
% caller's workspace

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<5, fmt='.-'; end

fmt = sprintf('''%s''',fmt);
xi = length(findstr(xstr,'%d')); % does xstr contain %d
s = sprintf('%s(',plotstr);
for i=1:N,
    if i<N, comma = ','; else comma = ''; end
    s = sprintf('%s%s,%s,%s%s',s,xstr,ystr,fmt,comma);
    if xi,
        s = sprintf(s,i,i);
    else
        s = sprintf(s,i);
    end
end
s = sprintf('%s);',s);

% Evaluate plotting expression in the calling workspace if caller does not
% request the expression string "s"
if nargout == 0,
    evalin('caller', s);
    clear s;
end