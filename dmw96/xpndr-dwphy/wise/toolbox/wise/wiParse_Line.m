function wiParse_Line(text)
%wiParse_Line -- Parse and interpret a line of script text
%
%   wiParse_Line(text) runs a line of text through the wiParse line
%   parser and interprets/executes the commands or assignments contained
%   within.

%   Developed by Barrett Brickner
%   Copyright 2001 Bergana Communications, Inc. All rights reserved.

if nargin ~= 1, error('Incorrect number of arguments.'); end
wiseMex('wiParse_Line()',text);
