function [Status] = viOpen(Session)
%viClose -- Close an open VISA session
%
%   [Status] = viClose(Session) closes an open VISA session.

[Status] = mexVISA('viClose()', Session);