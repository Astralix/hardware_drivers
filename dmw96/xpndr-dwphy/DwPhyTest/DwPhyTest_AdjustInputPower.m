function [Pout_dBm, RSSI, MsrPwr] = DwPhyTest_AdjustInputPower(varargin)
% [Pout_dBm, RSSI, MsrPwr] = DwPhyTest_AdjustInputPower(varargin)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include 'Pout_dBm','AbsPwrH','Tolerance','Limits',
%    and 'Verbose'.
%
%    Pout_dBm is the starting and ending output power for the default ESG.
%    AbsPwrH is the target power measurement (same units as baseband, default
%    is 56). Tolerance is the gain error, default is 0.5 (dB). Limits define
%    the lower and upper output power limits for the ESG.
%
%    This routine is intended to be run where the input is a constant signal
%    such as the L-LTF waveform.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

Pout_dBm = -65;
AbsPwrH = 56;
Tolerance = 0.5;
Limits = [-80 -10];
MaxSteps = 20;
SetRxPower = 1;
Verbose = 1;

%% Parse Argument List
DwPhyLab_EvaluateVarargin(varargin, ...
    {'Pout_dBm','AbsPwrH','Tolerance','Limits'}, 1);

Pmin = min(Limits);
Pmax = max(Limits);
Pwr100dBm = DwPhyLab_ReadRegister(77);

done = 0; steps = 0;
if Verbose, fprintf('Adjust Input Power ...\n'); end

%% Gain Adjustment Loop
while ~done,

    % Limit output power range
    Pout_dBm = min(Pmax, Pout_dBm);
    Pout_dBm = max(Pmin, Pout_dBm);

    % Set output power, a delay here is needed to insure the ESG ALC has
    % settled. The default ALC bandwidth is 100 Hz, so 50 ms delay
    % corresponds to 5 time constants.
    if SetRxPower || steps,
        DwPhyLab_SetRxPower(Pout_dBm); 
        pause(0.050);
    end
    
    % Measure power and compute the gain adjustment. Power is expressed in
    % units of 3/4 dB for consistency with the baseband AGC. The gain step
    % is simply the negative of the gain error.
    RSSI = DwPhyLab_AverageRSSI;
    MsrPwr = 4/3 * max(RSSI) + Pwr100dBm; % units are 3/4 dB
    dBstep = 0.75 * (AbsPwrH - MsrPwr(1));
    
    if Verbose,
        fprintf('   %d Pout_dBm = %1.1f, MsrPwr = %1.1f (%g)\n',steps,Pout_dBm,MsrPwr,AbsPwrH);
    end

    % Gain adjustment and completion check. If the gain step (error)
    % magnitude is less than the required tolerance, the adjustment is
    % complete. The loop will also stop if the adjustment takes the output
    % power to one of the range limits.
    if abs(dBstep) < Tolerance,
        done = 1;
    else
        Pout_dBm = max(Pmin, min(Pmax, Pout_dBm + dBstep));
    end
    
    if (Pout_dBm == Pmax) || (Pout_dBm == Pmin),
        done = 1;
    end
    
    % Count the number of gain updates. If it is too large, assume the loop
    % is not converging and exit to avoid hanging the calling function
    steps = steps + 1;
    if steps > MaxSteps,
        warning('DwPhyTest_AdjustInputPower:GainSteps',...
            'Aborting because number of gain steps > 20'); 
        done = 1;
    end
end

