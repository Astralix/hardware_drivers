function wiParse_ScriptFile(filename)
%wiParse_ScriptFile -- Parses a script file
%
%   wiParse_ScriptFile(filename) passes the named file through the WiSE
%   script parser/interpreter.

%   Developed by Barrett Brickner
%   Copyright 2001 Bergana Communications, Inc. All rights reserved.

if nargin ~= 1, error('Incorrect number of arguments.'); end
wiseMex('wiParse_ScriptFile()',filename);
