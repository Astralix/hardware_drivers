function [Status, Buffer] = viRead(Session, Count)
%viRead -- Read data from a device
%
%   [Status, Buffer] = viRead(Session, Count) reads up to Count bytes of
%   data from the specified VISA Session. The VISA Status and string Buffer
%   are returned.

if(nargout ~= 2)
    error('viRead requires two output parameters');
end
if(nargin<2), Count = 1024; end;

[Status, Buffer] = mexVISA('viRead()', Session, Count);
