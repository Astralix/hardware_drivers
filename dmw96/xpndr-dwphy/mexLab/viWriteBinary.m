function [Status, ReturnCount] = viWriteBinary(Session, Buffer)
%viWriteBinary -- Write data to a device
%
%   [Status, ReturnCount] = viWrite(Session, Buffer) write the contents of
%   the byte Buffer to the specified VISA session. Buffer is cast as type
%   uint8. The VISA Status and actual number of bytes written (ReturnCount) 
%   is returned.

if(~isvector(Buffer))
    error('Input buffer must be an array');
end

[Status, ReturnCount] = mexVISA('viWriteBinary()', Session, uint8(Buffer));