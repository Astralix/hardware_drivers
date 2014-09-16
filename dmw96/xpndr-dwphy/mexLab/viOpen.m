function [Status, Session] = viOpen(Resource, Timeout)
%viOpen -- Open a VISA session to the named resource
%
%   [Status, Session] = viOpen(Resource) opens a VISA session to the named
%   resource and returns the session handle in Session. The Status code is
%   a VISA status code. A typical resource string might be 'GPIB0::1::INSTR'.

if(nargin == 1),  Timeout = 0; end;
if(nargout ~= 2),
    error('viOpen requires two output parameters');
end

% S = dbstack('-completenames');
% n = length(S);
% if(n > 1)
%     StackTrace = '';
%     for i=n:-1:2,
%         StackTrace = sprintf('%sline%4d in %s\n',StackTrace, S(i).line, S(i).file);
%     end
% else
%     StackTrace = sprintf('viOpen called from command prompt\n');
% end
% 
% [Status, Session] = mexVISA('viOpen()', Resource, Timeout, StackTrace);

[Status, Session] = mexVISA('viOpen()', Resource, Timeout);