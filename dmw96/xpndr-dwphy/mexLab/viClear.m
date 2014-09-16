function [Status] = viClear(Session)
%viClear -- Send a device clear
%
%   [Status] = viClear(Session) sends a command to clear the device with the
%   specified VISA session. For GPIB devices, this operation sends a GPIB clear.

[Status] = mexVISA('viClear()', Session);