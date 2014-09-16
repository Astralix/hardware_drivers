function [V,T,T0] = ReadLeCroyWaveform(viResource, trace)
% [V,T,T0] = ReadLeCroyWaveform(RESOURCE, TRACE)
%
%       Read the specified TRACE (e.g., 'C1','C2',etc.) from the LeCroy
%       oscilloscope at the address in RESOURCE (e.g., 'GPIB::5'). The
%       amplitude is returned in V, the sample period in T, and the
%       time offset relative to the trigger in T0.

% Written by Barrett Brickner
% This function is part of the mexLab toolbox.

[V, T, T0] = mexVISA('mexVISA_ReadLeCroyWaveform()',viResource,trace);