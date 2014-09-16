function [Status] = viAssertTrigger(Session, Protocol)
%viAssertTrigger -- Trigger a device
%
%   [Status] = viAssertTrigger(Session, Protocol) sends a trigger 
%   signal/command to the device with the specified Session and Protocol.
%   Allowable Protocol strings are 
%
%        {'VI_TRIG_PROT_DEFAULT', 'VI_TRIG_PROT_ON',
%         'VI_TRIG_PROT_OFF', 'VI_TRIG_PROT_SYNC' }.

[Status] = mexVISA('viAssertTrigger()', Session, Protocol);