function [x,T] = DwPhyLab_GetScopeWaveform(trace)
%[x,T] = DwPhyLab_GetScopeWaveform(Trace)
%
%   Retrieves the waveform x with sample period T for the specified Trace where Trace
%   is 'C1','C2','C3',C4' for channels 1-4, respsectively.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

scope = DwPhyLab_OpenScope;
if isempty(scope), 
    pause(0.5);
    scope = DwPhyLab_OpenScope;
    if isempty(scope),error('Unable to connect to to the oscilloscope'); end
end

x  = double(scope.GetScaledWaveform(trace, 4e6, 0));
x = reshape(x,length(x),1); % make x a column vector

if nargout > 1, 
    buf = DwPhyLab_SendQuery(scope, sprintf('%s:INSP? ''HORIZ_INTERVAL''',trace));

    k = findstr(buf,'HOR'); buf=buf( k :length(buf));
    k = findstr(buf,':');   buf=buf(k+1:length(buf));
    T = sscanf(buf,'%f');
end
DwPhyLab_CloseScope(scope);

%% REVISIONS
% 2010-02-19 BB - Added 2nd attempt to open scope on failure. This seems to
%                 work-around a timing issue with the WavePro 7200.
% 2010-08-04 BB - Use DwPhyLab functions to control the scope
%                 Only get horizontal interval if "T" output is requested
