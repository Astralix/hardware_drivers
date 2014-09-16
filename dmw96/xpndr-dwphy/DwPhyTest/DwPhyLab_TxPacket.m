function DwPhyLab_TxPacket(DataRate, Length, PowerLevel, PacketType)
%DwPhyLab_TxPacket( DataRate[Mbps], Length, PowerLevel, PacketType )
%
%   Transmit a single packet using the specified parameters. Valid parameter ranges are
%
%       Mbps = {1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54}
%       Length = 28 - 1562
%       PowerLevel = TXPWRLVL = 0 - 63
%       PacketType = {0:Modulo-256 MSDU, 1:Random MSDU}

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

%%% Start Burst Transmission
if(nargin < 1), error('    Must specify at least data rate'); end
if(nargin < 2), Length = [];     end 
if(nargin < 3), PowerLevel = []; end
if(nargin < 4), PacketType = []; end

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

if((Length < 28) || (Length > 1562))
    error('    Length out of range (28 - 1562 bytes)'); 
end
LengthMinusFCS = Length - 4; % Reduce length byte FCS field size to match MAC API defintion

DwPhyMex('RvClientTxSinglePacket', LengthMinusFCS, PhyRate, PowerLevel, PacketType);
TxPacket.Length     = Length;
TxPacket.DataRate   = DataRate;
TxPacket.PowerLevel = PowerLevel;
TxPacket.PacketType = PacketType;
DwPhyLab_Parameters('TxPacket',TxPacket);

%% REVISIONS
% 2010-05-18: Added support for 802.11n MCS = 0-7
