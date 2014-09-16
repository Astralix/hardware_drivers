function DwPhyLab_Setup
%Basic Setup for DwPhyLab
%
%   This function establishes communication with the remote VWAL server on the device
%   under test using DwPhyMex as a client. It programs the MAC address to match that
%   specified in the parameter file. Additionally, it configures the driver for
%   "test" mode so that it does not try to initiate activity.

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

% Get the Test Station Parameters
param = DwPhyLab_Parameters;

% Connect to the test devices
status = DwPhyMex('RvClientConnect',param.STA.IP,param.STA.Port);
if status~=0, error('Problem Connecting to STA. Status = %d\n',status); end

% Select "PHY Test Mode"
DwPhyCheck( DwPhyMex ('RvClientPhyTestMode') );

% Set the station MAC address
DwPhyLab_SetMacAddress(param.STA.WiFiMAC);


% Set the station ACK TXPWRLVL
if isfield(param.STA,'ACK_PWRSETTING'),
    Address = DwPhyLab_Parameters('BaseAddress') + hex2dec('9074');
    DwPhyLab_WriteRegister(Address, param.STA.ACK_PWRSETTING)
end

% Determine whether an existing handle to an instrument session can be shared on a
% subsequent attempt to open without first closing the handle.
if exist('mexVISA','file') == 3,
    mexVISA('EnableSharedSessions', any(DwPhyLab_Parameters('mexVISA_EnableSharedSessions') ~= 0));
end

%% REVISIONS
% 2008-08-07 Added setting for ACK_PWRSETTING
% 2010-04-02 BaseAddress for ACK_PWRSETTING is now a parameter
% 2010-08-05 Allow mexVISA sessions to be shared by default