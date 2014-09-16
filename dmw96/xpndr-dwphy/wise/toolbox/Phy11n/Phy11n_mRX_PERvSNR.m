function data = Phy11n_mRX_PERvSNR(varargin)
% Phy11n_mRX_PERvSNR -- PER vs. SNR using the Phy11n_mRX environment
%
%   data = Phy11n_mRX_PERvSNR(varargin) performs a PER vs. SNR simulation using
%   Phy11n_mRX. Simulation conditions are set by a combination of the existing state of
%   wiseMex (e.g., parameters set via a script file before calling this function) and by
%   parameter assignements passed as strings in the variable argument list. Parameters for
%   the simulation loop include {'MinPER', 'MinFail', 'MaxPackets', 'MinSNR','MaxSNR',
%   'StepSNR', 'RndSeed', 'SNR'}.
%
%   Example: to perform a simulation where SNR and the minimum packet failure are
%   specified as arguments, call data = Phy11n_mRX_PERvSNR('SNR = 10:3:30','MinFail=50');

%   Written by Barrett Brickner
%   Copyright 2011 DSP Group, Inc. All rights reserved.

MinPER     = wiTest_GetParameter('MinPER');
MinFail    = wiTest_GetParameter('MinFail');
MaxPackets = wiTest_GetParameter('MaxPackets');
MinSNR     = wiTest_GetParameter('MinSNR');
MaxSNR     = wiTest_GetParameter('MaxSNR');
StepSNR    = wiTest_GetParameter('StepSNR');
RndSeed    = wiTest_GetParameter('RndSeed');

% Update rate is the frequency with which the waitbar is updated. A value of 1 makes for a
% nice presentation but costs ~5% in execution speed. 5-10 is a good nominal value.
UpdateRate = 5;

SNR = MinSNR:StepSNR:MaxSNR;

EvaluateVarargin(varargin); % populate user parameters

data.Description = mfilename;
data.Timestamp   = datestr(now);
data.Result      = 1;

data.MCS      = wiseMex('Phy11n_GetTxParameter','MCS');
data.N_SS     = ceil(data.MCS/8);
data.NumRx    = [];
data.Mbps     = GetDataRate(data.MCS);
data.Length   = wiTest_GetParameter('Length');
data.DataType = wiTest_GetParameter('DataType');

wiseMex('wiRnd_Init',RndSeed,0,0);

% Create the waitbar
% This serves two purposes. First it provides a simulation status indications and second,
% closing the box provides the user a wait to stop the simulation but allow data to be
% returned.
hBar = waitbar(0.0,{'',''}, 'Name',mfilename, 'WindowStyle','modal');
WaitString1 = sprintf('MCS = %d (%g Mbps)', data.MCS, data.Mbps);

for i = 1:numel(SNR),
    
    nPackets = 0;
    nFail = 0;
    
    while (nPackets < MaxPackets) && ((nFail < MinFail) || (nPackets < MinPackets))
        
        % Transmit a packet at receive R at the channel output
        [R,Fs] = wiseMex('wiTest_TxPacket()',0,SNR(i));
        if Fs ~= 20e6, error('Can only operate on 20 MHz channel, not oversampled'); end

        % Process received matrix with Phy11n_mRX
        RxData = Phy11n_mRX(R);
        
        % Compare the TX and RX data and increment counters
        TxData = wiTest_GetParameter('TxData');
        if length(TxData) ~= length(RxData) || any(TxData ~= RxData), 
            nFail = nFail + 1; 
        end
        nPackets = nPackets + 1;
        PER = nFail / nPackets;
        
        % Compute MinPackets to reduce bias introduced by stopping immediately after the
        % packet error that causes nFail == MinFail.
        if nFail == (MinFail - 1),
            MinPackets = nPackets * (nFail + 1) / nFail;
        end
        
        if ~ishandle(hBar),
            data.Result = -1;
            break;
        elseif mod(nPackets, UpdateRate) == 0,
            t = min(1, max([nPackets/MaxPackets nFail/MinFail]));
            waitbar( t, hBar, {WaitString1, sprintf('SNR = %1.1f dB, PER = %1.2f%%',...
                SNR(i), PER)} );
        end
                
    end
    
    data.SNR = SNR(1:i);
    data.PER(i) = PER;
    data.nFail(i) = nFail;
    data.nPackets(i) = nPackets;
    data.NumRx = size(R,2);
    
    fprintf('SNR = %1.1f  PER = %2d/%3d =%5.1f%%\n',SNR(i),nFail,nPackets,100*data.PER(i));
    if data.PER(i) < MinPER, break; end
    if ~ishandle(hBar), break; end
    
end

data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%%%  SUBFUNCTIONS  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [Mbps, N_SS] = GetDataRate(MCS)

    StreamDataRate = [6.5 13 19.5 26 39 52.5 58.5 65];
    N_SS = ceil(MCS/8);
    Mbps = N_SS * StreamDataRate(1+mod(MCS,8));

%%% Evaluate commands passed as strings or cell arrays. The strings are evaluated in the
%%% caller's address space so they can manipulate data elements in its workspace.
function EvaluateVarargin(varargin)

    for i = 1:length(varargin),

        % handle assignments passed as character strings
        if iscell(varargin{i})
            for j=1:length(varargin{i}),
                evalin('caller', sprintf('%s;',varargin{i}{j}) );
            end
        elseif ischar(varargin{i})
            evalin('caller', sprintf('%s;',varargin{i}) );

        % format not recognized
        else
            error('Unrecognized argument type in position %d',i);
        end
    end

