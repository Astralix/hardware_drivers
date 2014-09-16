function DwPhyTest_Setup
%DwPhyTest_Setup
%   This is the general setup function used to initialize the test platform
%   and configure instruments to run "DwPhyTest" functions.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

% Get the Test Station Parameters
param = DwPhyLab_Parameters;

% Configure the interface between DwPhyMex and the driver
DwPhyLab_Setup;

% Set Signal Generators to Very Low Output
% This is done instead of simply disabling them so that if two are
% connected via a combiner, a 50 Ohm source will be present on each side of
% the combiner (when disabled, the ESG output is an open)
if isfield(param,'E4438C_Address1') && ~isempty(param.E4438C_Address1), 
    DwPhyLab_SendCommand('ESG1','*RST');
end
if isfield(param,'E4438C_Address2') && ~isempty(param.E4438C_Address2),
    DwPhyLab_SendCommand('ESG2','*RST');
    DwPhyLab_SendCommand('ESG2',':OUTPUT:STATE ON');
end

% AWG520 Configuration
if isfield(param, 'AWG520_Address') && ~isempty(param.AWG520_Address), 
    DwPhyLab_SendCommand('AWG520','*RST');
end

% Spectrum Analyzer
if isfield(param,'E4440A_Address') && ~isempty(param.E4440A_Address),
    DwPhyLab_SendCommand('PSA','*RST');
end

% Initialize the PHY
DwPhyLab_Sleep;
DwPhyLab_Initialize;
