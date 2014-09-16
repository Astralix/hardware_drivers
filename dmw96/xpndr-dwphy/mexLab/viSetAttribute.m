function [Status] = viSetAttribute(Session, Attribute, Value)
%viSetAttribute -- Set an attribute value
%
%   [Status] = viSetAttribute(Session, Attribute, Value) sets the named
%   Attribute for the specified Session. The attribute is the VISA definition
%   expressed as a string, e.g., 'VI_ATTR_GPIB_UNADDR_EN'.

[Status] = mexVISA('viSetAttribute()', Session, Attribute, Value);