function h = DwPhyLab_OpenPSA
%DwPhyLab_OpenPSA -- Open a handle to the PSA

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

h = DwPhyLab_OpenESG('PSA');

% addr = DwPhyLab_Parameters('E4440A_Address');
% if(isempty(addr)), return; end; % no instrument specified
% 
% [status,h] = viOpen(addr);
% if status < 0,
%     [dummy, description] = viStatusDesc(h, status);
%     error('Unabled to open "%s": %s',addr,description');
% end

