% Bermai/DSP Group mexLab Toolbox
%
% VISA Instrument Control
%   viOpen             - Open a VISA session to a resource (instrument)
%   viClose            - Close a VISA session
%   viClear            - Send a clear command
%   viWrite            - Write a text buffer
%   viRead             - Read a text data buffer
%   viWriteBinary      - Write binary data
%   viReadBinary       - Read binary data
%   viAssertTrigger    - Trigger a device
%   viSetAttribute     - Set a named attribute
%   viGetAttribute     - Get a named attribute
%   viReadSTB          - Read the device status byte
%   viStatusDesc       - Return a text description of a VISA status code
%
% Instrument Specific Operations
%   ReadLeCroyWaveform - Read a waveform from a LeCroy oscilloscope
%   WriteAWG520        - Write an AWG520 waveform file
%
% Library Options
%   mexVISA_DisplaySessionList - Display a list of all open sessions

% Developed by Barrett Brickner
% Copyright 2007 DSP Group, Inc. All Rights Reserved.