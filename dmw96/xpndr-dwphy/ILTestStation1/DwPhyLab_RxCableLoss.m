function Ploss_dB = DwPhyLab_RxCableLoss( fcMHz, Filter )
% Ploss_dB = DwPhyLab_RxCableLoss(fcMHz, Filter)
%    Returns the calibration term for RX power. The value is positive loss with units
%    of dB. If the requested frequency is outside the range of the calibration data,
%    the appropriate end-point value from the calibration is returned.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

CalDataFile = DwPhyLab_Parameters('CalDataFile');

if isempty(CalDataFile),
    Ploss_dB = zeros(size(fcMHz));
else
    S = load(CalDataFile);
    fcMHzCal = S.data.CalData(S.data.iRX).Freqs / 1e6;
    PlossCal = S.data.CalData(S.data.iRX).Loss;
    
    Ploss_dB = interp1(fcMHzCal, PlossCal, fcMHz);
    Ploss_dB( fcMHz < min(fcMHzCal) ) = PlossCal(1);
    Ploss_dB( fcMHz > max(fcMHzCal) ) = PlossCal(length(PlossCal));
end

if nargin>1 && ~isempty(Filter),
    S = load(Filter);
    fcMHzCal = S.CalData.Freqs / 1e6;
    PlossCal = S.CalData.Loss;
    
    PlossFilter_dB = interp1(fcMHzCal, PlossCal, fcMHz);
    PlossFilter_dB( fcMHz < min(fcMHzCal) ) = PlossCal(1);
    PlossFilter_dB( fcMHz > max(fcMHzCal) ) = PlossCal(length(PlossCal));
    
    Ploss_dB = Ploss_dB + PlossFilter_dB;
end


Ploss_dB = (4.2)*ones(size(Ploss_dB));