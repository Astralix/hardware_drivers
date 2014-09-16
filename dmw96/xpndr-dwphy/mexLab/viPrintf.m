function [Status, ReturnCount] = viPrintf(Session, varargin)
%viPrintf -- Write data to a device
%
%   [Status, ReturnCount] = viPrintf(Session, Format, ...) prints a string to the
%   specified VISA session. Format and the subsequent variable arguments follow the
%   convention for fprintf.

if numel(varargin) < 1, error('Insufficient number of arguments'); end

Buffer = sprintf(varargin{:});
[Status, ReturnCount] = mexVISA('viWrite()', Session, Buffer);