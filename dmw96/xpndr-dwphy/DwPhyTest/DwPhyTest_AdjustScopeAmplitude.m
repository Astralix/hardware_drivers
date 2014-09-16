function [VppScope, tracelog] = DwPhyTest_AdjustScopeAmplitude(VppScope,UseC1)
%DwPhyTest_AdjustScopeAmplitude(VppScope,UseC1)
%   Adjust amplitude scaling on the oscilloscope for transmit EVM measurements. 
%   This function assumes a continuous transmit operation is in process, so only 
%   instrument controls and not the DUT are affected.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

if evalin('caller','exist(''Verbose'',''var'')')
    Verbose = evalin('caller','Verbose');
else
    Verbose = 0;
end

LastVpp = NaN;
if nargin<2, UseC1 = 0; end
if UseC1, trace = 'C1'; else trace = 'C3'; end

scope = DwPhyLab_OpenScope;
DwPhyLab_SendCommand(scope, 'COMM_HEADER SHORT');
DwPhyLab_SendCommand(scope, 'AUTO_CALIBRATE OFF'); % No autocalibration on acquisition
DwPhyLab_SendCommand(scope, 'TRIG_MODE AUTO;');
DwPhyLab_SendQuery  (scope,'*OPC?');
DwPhyLab_SendCommand(scope, 'TRIG_MODE STOP;');
DwPhyLab_SendQuery  (scope,'*OPC?');

if nargin<1 || isempty(VppScope), 
    buf = DwPhyLab_SendQuery(scope, sprintf('%s:VDIV?',trace));
    k = strfind(buf,'VDIV ') + 5;
    if ~isempty(k),
        buf = buf(k:length(buf));
        k = min(strfind(buf,' '));
        if ~isempty(k), buf=buf(1:(k-1)); end
    end
    VppScope = 8*str2double(buf);
    vprintf('      AdjustScopeAmplitude: Read VppScope = %1.4f\n',VppScope);
else
    if strcmp(trace,'C1'),
        DwPhyLab_SendCommand(scope, sprintf('C1:VDIV %1.4fV; ATTN 1',VppScope/8));
        DwPhyLab_SendCommand(scope, sprintf('C2:VDIV %1.4fV; ATTN 1',VppScope/8));
    else
        DwPhyLab_SendCommand(scope, sprintf('C3:VDIV %1.4fV',VppScope/8));
    end
    vprintf('      AdjustScopeAmplitude: Set VppScope = %1.4f\n',VppScope);
    DwPhyLab_SendQuery(scope,'*OPC?');
end

n = 0; done = 0;
while ~done,

    n = n + 1;
    [Vpp, buf] = MeasureAmplitude(scope, trace);
    tracelog{n}.Vpp = Vpp;
    tracelog{n}.buf = buf;
    
    for m = 2:5,
        if Vpp == LastVpp,
            vprintf('      AdjustScopeAmplitude: Vpp = LastVpp = %1.4f. Remeasuring...\n',Vpp);
            [Vpp, buf] = MeasureAmplitude(scope, trace);
            tracelog{n,m}.Vpp = Vpp;
            tracelog{n,m}.buf = buf;
        end
    end
    LastVpp = Vpp;
    
        if Vpp < (0.25 * VppScope), VppScope = VppScope/2;
    elseif Vpp < (0.35 * VppScope), VppScope = VppScope/sqrt(2);
    elseif Vpp > (0.98 * VppScope), VppScope = VppScope*sqrt(2);
    else                            VppScope = Vpp/0.95; done = 1; 
        end
    
    tracelog{n}.VppScope = VppScope;
        
    if strcmp(trace,'C1'),
        DwPhyLab_SendCommand(scope, sprintf('C1:VDIV %1.4fV; ATTN 1',VppScope/8));
        DwPhyLab_SendCommand(scope, sprintf('C2:VDIV %1.4fV; ATTN 1',VppScope/8));
        pause(0.5);
    else
        DwPhyLab_SendCommand(scope, sprintf('C3:VDIV %1.4fV',VppScope/8));
    end
    DwPhyLab_SendQuery(scope,'*OPC?');
    vprintf('      AdjustScopeAmplitude: n = %2d, VppScope = %1.4f, Vpp = %1.4f\n',n,VppScope,Vpp);
    
    if n == 20,
        tracelog{n}.Abort = 1;
        warning('DwPhyTest_AdjustTxInstrLevels:Steps','Aborting scope level tuning at step count limit');
        done = 1; 
    end
end

if strcmp(trace,'C1'), DwPhyLab_SendCommand(scope, sprintf('C1:TRIG_LEVEL %1.4fV',VppScope/8));
else                   DwPhyLab_SendCommand(scope, sprintf('TRIG_LEVEL %1.4fV',VppScope/8)); 
end

DwPhyLab_SendCommand(scope, 'AUTO_CALIBRATE ON');
DwPhyLab_SendQuery  (scope,'*OPC?');
DwPhyLab_CloseScope (scope);

%% MeasureAmplitude
function [Vpp, buf] = MeasureAmplitude(scope, trace)
DwPhyLab_SendQuery  (scope,'*OPC?'); % [100219BB]
DwPhyLab_SendCommand(scope, 'TRIG_MODE SINGLE; WAIT 0.5');
DwPhyLab_SendQuery  (scope,'*OPC?'); % [100219BB]
DwPhyLab_SendCommand(scope,'TRIG_MODE SINGLE; WAIT 0.5');
if DwPhyLab_Parameters('ScopeIsWavePro7200'),
    DwPhyLab_SendCommand(scope,'FORCE_TRIGGER; WAIT 0.5');
end
DwPhyLab_SendQuery(scope,'*OPC?'); % [100219BB]
buf = DwPhyLab_SendQuery(scope, sprintf('%s:PARAMETER_VALUE? AMPL',trace));
if strfind(buf,'UNDEF')
    buf = DwPhyLab_SendQuery(scope, sprintf('%s:PARAMETER_VALUE? AMPL',trace));
end    
s = buf( (strfind(buf,'AMPL,')+5):length(buf) );
k = min( [find(s == ' ') find(s == ',') find(s == 'V')] );
s = s(1:(k-1));
Vpp = str2double(s);
if isnan(Vpp),
    fprintf('buf = "%s", s = "%s"\n',buf,s);
end
DwPhyLab_SendQuery(scope,'*OPC?'); % [100222BB]

%% vprintf
function vprintf(varargin)
if evalin('caller','exist(''Verbose'',''var'')') && evalin('caller','Verbose')
    fprintf(varargin{:});
end

%% REVISIONS
% 2008-06-26 - Added checking to verify Vpp changes on subsequent measurements. It is
%              not clear why this is needed, but sometimes the adjustment fails and
%              VDIV is too low so the waveform is clipped. Adding this check appears
%              to fix the problem.
% 2008-06-30 - Added vprintf function; took handling of repeated measurement values 
%              out of the verbose loop. Added LeCroy "WAIT" command instead of pause.
% 2008-10-17 - Added option to adjust C1 and C2 based on C1 (for RX EVM)
% 2010-02-22 - Added WavePro7200 specific code to force an acquisition. Added more OPC
%              checking and increased WAIT timeouts from 0.2 to 0.5 seconds.
% 2010-08-04 - Use DwPhyLab functions to control the scope