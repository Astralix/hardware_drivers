function [Status, Value] = viGetAttribute(Session, Attribute)
%viGetAttribute -- Retrieve an attribute value
%
%   [Status, Value] = viGetAttribute(Session, Attribute) retrieves the named 
%   Attribute for the specified Session. The attribute is the VISA definition
%   expressed as a string, e.g., 'VI_ATTR_GPIB_PRIMARY_ADDR'.

if(nargout ~= 2),
    error('viGetAttribute requires two output paramters');
end

[Status, Value] = mexVISA('viGetAttribute()', Session, Attribute);