function h = DwPhyLab_OpenScope(addr)
% Handle = DwPhyLab_OpenScope

% Retrieve Address from Parameters
if nargin < 1,
    addr  = DwPhyLab_Parameters('Scope_Address');
    if isempty(addr),
        error('Scope Address not defined in Parameters');
    end
end

% Convert VISA style GPIB address to ActiveDSO format
if findstr(upper(addr),'GPIB::') == 1
    addr = sprintf('GPIB:%s',addr(7:length(addr)));
end

% Open Connection to the Instrument
h = actxserver('LeCroy.ActiveDSOCtrl.1');
pause(1); % [BB110609 - Per ArielF, required to get EVM measurement to work in IL] Should be 1!
ok = h.MakeConnection(addr); 
pause(0.1); % [BB110609 - Per ArielF, required to get EVM measurement to work in IL]
if ~ok, h=[]; end
