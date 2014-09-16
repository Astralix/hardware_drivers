function h = DwPhyLab_OpenAWG520
% h = DwPhyLab_OpenAWG520

h = [];
param = DwPhyLab_Parameters;
field = 'AWG520_Address';

if ~isfield(param,field)
    error('%s not defined in DwPhyLab_Parameters',field);
else
    addr = param.(field);
end
    
%% Open Connection to the Instrument
if findstr(addr,'.')
    status = mexLab_AWG520('openAWG520',addr);
    if ~status, h = addr; else h = []; end;
elseif findstr(addr,'GPIB:')
    [status, h] = viOpen(addr);
    if status ~= 0, error('Error opening instrument "%s"',addr); end
else
    error('Invalid address "%s"',addr);
end
