function Ploss_dB = DwPhyLab_TxCableLoss( fcMHz )
% Ploss_dB = DwPhyLab_TxCableLoss(fcMHz)
%    Returns the calibration term for TX power. The value is positive loss with units
%    of dB. If the requested frequency is outside the range of the calibration data,
%    the appropriate end-point value from the calibration is returned.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

CalDataFile = DwPhyLab_Parameters('CalDataFile');

if isempty(CalDataFile),
    Ploss_dB = zeros(size(fcMHz));

else
    S = load(CalDataFile);
    fcMHzCal = S.data.CalData(S.data.iTX).Freqs / 1e6;
    PlossCal = S.data.CalData(S.data.iTX).Loss;
    
    Ploss_dB = interp1(fcMHzCal, PlossCal, fcMHz);
    Ploss_dB( fcMHz < min(fcMHzCal) ) = PlossCal(1);
    Ploss_dB( fcMHz > max(fcMHzCal) ) = PlossCal(length(PlossCal));
end

Ploss_dB = (10.7 + 0.6)*ones(size(Ploss_dB)); % (SM) overwrite