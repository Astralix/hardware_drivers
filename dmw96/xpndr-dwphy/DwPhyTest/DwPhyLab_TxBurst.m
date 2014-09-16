function DwPhyLab_TxBurst(NumPackets, DataRate, Length, PowerLevel, PacketType)
%DwPhyLab_TxBurst( NumPackets, DataRate[Mbps], Length, PowerLevel, PacketType )
%
%   Transmit a burst of NumPackets using the specified parameters. Valid parameter
%   ranges are
%
%       Mbps = {1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54}
%       Length = 28 - 1560
%       PowerLevel = TXPWRLVL = 0 - 63
%       PacketType = {0:Modulo-256 MSDU, 1:Random MSDU, 2:All Ones}
%
%DwPhyLab_TxBurst(0)
%    Stop any on-going burst transmission

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if(nargin < 1), error('    Must specify NumPackets'); end

if(NumPackets == 0)

    %%% Just stop any transmissions
    StopTxBatch;

else

    %%% Start Burst Transmission
    if(nargin < 2), error('    Must specify at least data rate'); end
    if(nargin < 3), Length = [];     end 
    if(nargin < 4), PowerLevel = []; end
    if(nargin < 5), PacketType = []; end

    if isempty(Length),     Length     = 1000; end
    if isempty(PowerLevel), PowerLevel = 0; end
    if isempty(PacketType), PacketType = 0; end
    
    if( (PowerLevel < 0) || (PowerLevel > 255) ), error('    PowerLevel out of range (0 - 255)'); end

    switch(DataRate)
        case {1,2,11, 6,9,12,18,24,36,48,54}, PhyRate = DataRate;
        case  5.5,                            PhyRate = 5;
        case  6.5,                            PhyRate = 128+0;
        case 13,                              PhyRate = 128+1;
        case 19.5,                            PhyRate = 128+2;
        case 26,                              PhyRate = 128+3;
        case 39,                              PhyRate = 128+4;
        case 52,                              PhyRate = 128+5;
        case 58.5,                            PhyRate = 128+6;
        case 65,                              PhyRate = 128+7;
        otherwise, error('    DataRate[Mbps] invalid');
    end

    if((Length < 28) || (Length > 1560))
        error('    Length out of range (28 - 1560 bytes)'); 
    end
    LengthMinusFCS = Length - 4; % Reduce length byte FCS field size to match MAC API defintion

    StopTxBatch;
try    
    DwPhyMex('RvClientStartTxBatch',NumPackets,LengthMinusFCS, PhyRate, PowerLevel, PacketType);
catch
    disp('DwPhyLab_TxBurst: Unable to start TxBatch');
end
    TxBatch.NumPackets = NumPackets;
    TxBatch.Length = Length;
    TxBatch.DataRate = DataRate;
    TxBatch.PowerLevel = PowerLevel;
    TxBatch.PacketType = PacketType;
    DwPhyLab_Parameters('TxBatch',TxBatch);

end


%% StopTxBatch
function StopTxBatch
DwPhyMex('RvClientStopTxBatch'); % stop any on-going batch transmission

%%% Rapid stop/start cycles of TxBatch can cause problems because the mechanism
%%% queues ~100 packets. If these have not all spooled out, a subsequent start
%%% can fail. In turn this may have other undesireable effects such as during
%%% driver stop/start cycles.
%%%
%%% Delay the predicted time to send 100 packets with 50 us spacing
%%% (conservative IFS estimate)
TxBatch = DwPhyLab_Parameters('TxBatch');
if ~isempty(TxBatch),
    switch TxBatch.DataRate,
        case {1}, 
            Tpacket = (192 + 8*TxBatch.Length) * 1e-6;
        case {2, 5.5, 11}, 
            Tpacket = (92 + 8*TxBatch.Length) / TxBatch.DataRate * 1e-6;
        case {6,9,12,18,24,36,48,54,72},
            Tpacket = (20 + 4*ceil(8*TxBatch.Length/TxBatch.DataRate/4)) * 1e-6;
        case {6.5, 13, 19.5, 26, 39, 52, 58.5, 65},
            Tpacket = (36 + 4*ceil(8*TxBatch.Length/TxBatch.DataRate/4)) * 1e-6;
        otherwise, error('Undefined DataRate case');
    end
    T_IFS = 50e-6; % aSIFSTime+2*aSlotTime: OFDM=34e-6, CCK=50e-6
    Tdelay = 100 * (Tpacket + T_IFS); % total delay
%    Tdelay = Tdelay + 0.10; % add 100 ms for good measure
    pause(Tdelay);
end
DwPhyLab_Parameters('TxBatch',[]);

if TxPending,
    t0 = now; dt = 0;
    while (dt < 4) && TxPending
        DwPhyMex('RvClientStopTxBatch'); % stop any on-going batch transmission
        pause (0.1); fprintf('* TxPending: dt = %1.1f\n',dt);
        dt = (now - t0) * 24 * 3600;
    end
    if dt >= 4,
        error('Timeout waiting for TxBatch to end');
    end
end


%% TxPending
%  Determine if TxBatch is still in progress by reading the PRBS register and looking for
%  a change. Generally, this will work for OFDM, but it would be better to query whether
%  there were still packets in the software queue.
function status = TxPending
status = DwPhyMex('RvClientQueryTxBatch');

%% REVISIONS
% 2010-05-18: Added support for 802.11n MCS = 0-7
% 2010-12-18 [SM]: Changed TxPending() to utilize new RvClient function 