function Response = DwPhyLab_QueryScope(h, msg, ResponseLength)
%Response = DwPhyLab_QueryScope(Handle, Message)
%
%   THIS FUNCTION IS DEPRECATED: USE DwPhyLab_SendQuery INSTEAD.

if nargin < 2, msg = []; end
if nargin < 3, ResponseLength = 200; end

Response = DwPhyLab_SendQuery(h, msg, ResponseLength);
