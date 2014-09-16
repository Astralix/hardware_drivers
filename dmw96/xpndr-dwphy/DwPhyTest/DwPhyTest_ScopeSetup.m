function DwPhyTest_ScopeSetup(Reset, Vpp, Vtrigger, TrigMode)
%DwPhyTest_ScopeSetup(Reset, Vpp, Vtrigger, TrigMode)

if nargin<4, TrigMode = []; end
if nargin<3, Vtrigger = []; end
if nargin<2, Vpp = []; end
if nargin<1,
    Reset = 1;
    Vpp = 0.8;
    Vtrigger = 0.05;
    TrigMode = 'AUTO';
end
    
scope = DwPhyLab_OpenScope;
if isempty(scope), error('Unable to open oscilloscope'); end

if Reset,
    DwPhyLab_SendCommand(scope, '*RST');
end

DwPhyLab_SendCommand(scope, 'C1:TRACE OFF');
DwPhyLab_SendCommand(scope, 'C2:TRACE OFF');
DwPhyLab_SendCommand(scope, 'C3:TRACE ON');
DwPhyLab_SendCommand(scope, 'C4:TRACE OFF');

if ~isempty(Vpp),
    DwPhyLab_SendCommand(scope, sprintf('C3:VDIV %1.4fV',Vpp/8));
end
DwPhyLab_SendCommand(scope, 'C3:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C3:BWL 200MHz');
DwPhyLab_SendCommand(scope, 'C3:COUPLING D50');

DwPhyLab_SendCommand(scope, 'C4:OFFSET 0.0');
DwPhyLab_SendCommand(scope, 'C4:COUPLING D1M');
DwPhyLab_SendCommand(scope, 'C4:VDIV 1V');

DwPhyLab_SendCommand(scope, 'TIME_DIV 200E-6');
DwPhyLab_SendCommand(scope, 'MEMORY_SIZE 2.5MA'); % [100218BB] - Add "A" to 2.5M

if ~isempty(Vtrigger),
    DwPhyLab_SendCommand(scope, sprintf('TRIG_LEVEL %1.4fV',Vtrigger));
end
DwPhyLab_SendCommand(scope, 'C3:TRIG_COUPLING DC');

if DwPhyLab_Parameters('ScopeIsWavePro7200'),
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY -800e-6');
    DwPhyLab_SendCommand(scope, 'TRIG_SELECT WIDTH,SR,C3,HT,PL,HV,5E-6 S');
    DwPhyLab_SendCommand(scope, 'TRIG_SLOPE NEG');
else   
    DwPhyLab_SendCommand(scope, 'TRIG_SELECT GLIT, SR,C3,HT,PL,HV,5E-6');
    DwPhyLab_SendCommand(scope, 'TRIG_DELAY 10.0 PCT');
end

if ~isempty(TrigMode),
    DwPhyLab_SendCommand(scope, sprintf('TRIG_MODE %s',TrigMode));
end

% wait for configuration to complete
tic; opc = 0;
while (isempty(opc) || ~(strcmp(opc,'1') || strcmp(opc,'*OPC 1'))) && (toc < 5),
    opc = DwPhyLab_SendQuery(scope,'*OPC?');
end
if toc >= 5, error('Timeout waiting for scope OPC'); end

DwPhyLab_CloseScope(scope);

%% REVISIONS
% 2010-02-18 Change memory depth setting from 'MEMORY_SIZE 2.5M' to
%            'MEMORY_SIZE 2.5MA'. Wait of OPC at the end.
% 2010-05-25 Handle *OPC? query result as either '1' or '*OPC 1'
