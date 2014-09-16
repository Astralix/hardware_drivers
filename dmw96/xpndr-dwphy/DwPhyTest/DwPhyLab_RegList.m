function RegList = DwPhyLab_RegList
% RegList = DwPhyLab_RegList
%    Return a list of the current PHY register values. The returned values
%    may include unused address spaces but include all DW52 PHY registers.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if DwPhyLab_RadioIsRF22
    Addr = [1:383, 768:1023, hex2dec('8000'):hex2dec('807D')];
else
    Addr = [1:383, 768:1023];
end

RegList = zeros(length(Addr),2);
RegList(:,1) = Addr;
for i=1:length(Addr),
    RegList(i,2) = DwPhyLab_ReadRegister(Addr(i));
end

%% REVISIONS
% 2010-04-15      Change upper address from 804 to 1023 to include all DMW96 registers
% 2010-12-06 [SM] Increase Addr range to accommodate RF22  