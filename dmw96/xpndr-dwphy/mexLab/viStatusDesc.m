function [Status,Description] = viStatusDesc(Session, StatusValue)
%viStatusDesc -- Return a description of a VISA status code
%
%   [Status,Description] = viStatusDesc(Session, StatusValue) returns a 
%   text description of the VISA status code in StatusValue.

if(nargout ~= 2)
    error('viStatusDesc requires two output parameters');
end

[Status, Description] = mexVISA('viStatusDesc()', Session, StatusValue);