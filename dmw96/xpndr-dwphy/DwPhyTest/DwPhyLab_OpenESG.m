function h = DwPhyLab_OpenESG(n)
%DwPhyLab_OpenESG -- Open a handle to the ESG/PSA
% 
%   H = DwPhyLab_OpenESG(n) returns a handle to the Agilent E4438C ESG n. The
%   function will connect to the E4440A PSA if n = 'PSA'.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

h = [];

%% Resolve ESG Addressing
if nargin < 1, n = []; end;
if isempty(n), n = 1; end;

%% Lookup address in parameter structure
param = DwPhyLab_Parameters;
switch upper(n),
    case {1,'ESG1','ESG'}, field = 'E4438C_Address1';
    case {2,'ESG2'      }, field = 'E4438C_Address2';
    case {'PSA','E4440A'}, field = 'E4440A_Address';
    otherwise, error('Illegal value for n');
end
if(isfield(param,field))
    addr = param.(field);
else
    addr = [];
end
    
if(isempty(addr)), return; end; % no instrument specified

%% Open Connection to the Instrument
if DwPhyLab_Parameters('UseAgilentWaveformDownloadAssistent')

    [NewAddr, gpib, IP] = FormatAddress(1, addr);
    
    if isempty(gpib),
        h = agt_newconnection('TCPIP',IP);
    else
        h = agt_newconnection('GPIB',0,gpib);
    end

else

    NewAddr = FormatAddress(2, addr);
    
    [status, h] = viOpen(NewAddr);
    if status < 0,
        M = 2^32;
        error('viOpen(%s) failed with status = 0x%x',NewAddr,dec2hex(mod(status+M,M),8));
    end

end

%% FormatAddress
%  Convert format addresses to use a GPIB or TCPIP prefix followed by n = {1,2} colons
function [NewAddr, GPIB, IP] = FormatAddress(n, OldAddr)
if ~any(n == [1 2]), error('n must be either 1 or 2'); end
colons = '::';

if findstr(upper(OldAddr),'GPIB')
    
    k = 1;
    while isnan(str2double(OldAddr(k))),
        k = k + 1;
    end
    IP = '';
    GPIB = str2double(OldAddr(k:length(OldAddr)));
    NewAddr = sprintf('GPIB%s%d',colons(1:n),GPIB);
    
elseif findstr(OldAddr,'.')
    
    k = min(findstr(OldAddr,'.')) - 1;
    while (k > 0) && ~isnan(str2double(OldAddr(k))),
        k = k - 1;
    end
    IP = OldAddr( (k+1):length(OldAddr) );
    GPIB = [];
    NewAddr = sprintf('TCPIP%s%s',colons(1:n),IP);
    
else
    
    error('Invalid instrument %s = %s',field,addr);

end

%% REVISIONS
% 2010-08-07 BB - Modified to use either the Agilent I/O library or VISA
