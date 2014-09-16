function [Status, Buffer] = viReadBinary(Session, Count)
%viRead -- Read data from a device
%
%   [Status, Buffer] = viRead(Session, Count) reads up to Count bytes of
%   data from the specified VISA Session. The VISA Status and binary Buffer
%   (class uint8) are returned.

if(nargout ~= 2)
    error('viRead requires two output parameters');
end
if(nargin<2), Count = 4096; end;

[Status, Buffer] = mexVISA('viReadBinary()', Session, Count);