%WISEMEX -- Wireless Simulation Environment
%
%   wiseMex is the Matlab executable form of WiSE. It contains the same
%   coded as the console executable version, but can be accessed directly
%   from within Matlab.
%
%   Calls to the library generally take the form
%
%       [vargout] = wiseMex(<Command>, [vargin])
%
%   where Command is a character string that specifies what command to
%   execute and determines the number of input and output arguments. For
%   example, the script file wise.txt would be processed via the call
%
%       wiseMex('wiParse_ScriptFile','wise.txt');
%
%   Calling wiseMex without arguments will print version information to the
%   Matlab console. The build version and version string can be retreived
%   with commands 'WISE_BUILD_VERSION' and 'WISE_VERSION_STRING'
%   respectively.
%
%   wiParse string substitutions are applied to wiseMex commands by
%   default. Calling wiseMex('wiseMex_AllowSubstitutions',<state>) will
%   change this behavior to <state> = {0:off, 1:on}.

