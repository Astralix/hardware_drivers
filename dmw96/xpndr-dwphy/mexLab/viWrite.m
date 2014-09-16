function [Status, ReturnCount] = viWrite(Session, Buffer)
%viWrite -- Write data to a device
%
%   [Status, ReturnCount] = viWrite(Session, Buffer) write the contents of
%   the character Buffer to the specified VISA session. The VISA Status and
%   actual number of bytes written (ReturnCount) is returned.

[Status, ReturnCount] = mexVISA('viWrite()', Session, Buffer);