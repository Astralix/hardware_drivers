function status = DwPhyLab_SetChannelFreq(fcMHz, RunIQCal)
% Set the PHY operating channel
%    Status = DwPhyLab_SetChannelFreq(fcMHz) selects the channel specified
%    by center frequency in MHz. Only channels supported in the DwPhy
%    driver module may be selected.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin == 1
    if isempty( DwPhyLab_Parameters('RunIQCal') )
        RunIQCal = 1; % default to run RF22 calibration
    else
        RunIQCal = DwPhyLab_Parameters('RunIQCal');
    end
end

status = DwPhyMex('DwPhy_SetChannelFreq', fcMHz);
if status == 0,
    DwPhyLab_Parameters('fcMHz',fcMHz); % record current operating channel
end

if DwPhyLab_RadioIsRF22
    DwPhyCheck( DwPhyMex('DwPhy_CalibrateTXDCOC_RF22') );
    DwPhyCheck( DwPhyMex('DwPhy_CalibrateRxLPF') );
    DwPhyCheck( DwPhyMex('DwPhy_CalibrateLOFT_RF22') );
    switch RunIQCal
        case 1, DwPhyCheck( DwPhyMex('DwPhy_CalibrateIQ') );
        case 2, DwPhyCheck( DwPhyMex('DwPhy_CalibrateIQ_RF22') ); 
        case 3, DwPhyCheck( DwPhyMex('DwPhy_CalibrateIQ_DMW96') ); 
    end
    DwPhyLab_Parameters('RunIQCal', RunIQCal); % record the IQ calibration setting            
end

% Handle errors if no status return is requested
if nargout < 1,
    if status ~= 0,
        description = DwPhyLab_ErrorDescription(status);
        hexstatus = dec2hex(status + 2^31, 8);
        error(' DwPhy_SetChannelFreq Status = 0x%s: %s',hexstatus, description);
    end
end

%% REVISIONS
% 2010-12-01 [SM]: Execute calibration routines for RF22
% 2011-03-07 [SM]: Add 'RunIQCal' argument to run/bypass calibration routine on RF22
% 2011-04-12 [SM]: Decide IQ-Calibration scheme based on 'RunIQCal' value