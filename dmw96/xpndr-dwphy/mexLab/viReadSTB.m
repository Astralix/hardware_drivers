function [Status, STB] = viReadSTB(Session)
%viReadSTB -- Read the status byte/word
%
%   [Status, STB] = viReadSTB(Session) reads the status byte/word from the
%   device addressed with the specified Session. STB should be interpretted
%   as a 16-bit unsigned integer.

if(nargout ~= 2)
    error('viReadSTB requires two output parameters');
end

[Status, STB] = mexVISA('viReadSTB()', Session);