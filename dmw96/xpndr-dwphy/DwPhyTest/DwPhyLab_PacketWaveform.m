function [y,m,PSDU] = DwPhyLab_PacketWaveform(varargin)
% Generate a waveform with one or more packets
%
%    [y,m,PSDU] = DwPhyLab_PacketWaveform(...) generates a waveform in y
%    using supplied string arguments of the form '<ParamName>=<ParamValue>'
%    where valid parameter names include 'Mbps', 'LengthPSDU', 'T_IFS',
%    'NumPkts', 'ShortPreamble', 'Broadcast', 'CleanFCS', and 'options'.
%
%    Alternately, the arguments may be supplied as elements of a structure
%    passed as a single argument, or the function can be called with
%    individual elements in the form 
%    [y,m,PSDU] = DwPhyLab_PacketWaveform(Mbps, Length, T_IFS, NumPkts, ...
%                    ShortPreamble, Broadcast, CleanFCS, options)
%  ,
%    ESG style markers are supplied in m. Marker 1 is a 1 microsecond pulse
%    than may be used as a trigger. Marker 2 is active during the packet
%    and can be used for RF blanking.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc. All Rights Reserved

%% Default Parameters
Baseband = 'Nevada'; %#ok<NASGU>
Mbps = 54;
LengthPSDU = 1000;
FrameBodyType = 0; % {0 = Modulo-256, 1 = Random, 2 = Pseudo-Random}
T_IFS = 34e-6; 
NumPkts = 1;
ShortPreamble = 1;
Greenfield = 0;
Broadcast = 1;
CleanFCS = 1;
DataType = 6;
options = {};
PSDU = [];
UseAMSDU = 0;

%% Parse Argument List
DwPhyLab_EvaluateVarargin( varargin, ...
    {'Mbps','LengthPSDU','T_IFS','NumPkts','ShortPreamble','Broadcast','CleanFCS','options'}, 1 );

if ~isempty(PSDU),
    LengthPSDU = length(PSDU);
end

%% Select Packet Aggregation for Long Packets if Not Defined
if ~exist('UseAMSDU','var')
    if ~exist('UseAMPDU','var') || ~UseAMPDU, %#ok<NODEF>
        if LengthPSDU > 4095,
            UseAMSDU = 1;
        end
    end
end
if ~exist('UseAMSDU','var'), UseAMSDU = 0; end
if ~exist('UseAMPDU','var'), UseAMPDU = 0; end %#ok<NASGU>

%% Configure Waveform Generation
%  Basic configuration is handled by ConfigWiseMex. User supplied options
%  are then supplied to the WiSE command parser as a mechanism to override
%  default behavior
Baseband = ConfigWiseMex(Mbps, LengthPSDU, DataType);
if ~isempty(options),
    if ischar(options),
        wiseMex('wiParse_Line()',options);
    else
        for i=1:length(options),
            wiseMex('wiParse_Line()',options{i});
        end
    end
end

%% Generate Waveform
if isempty(PSDU),
    if UseAMSDU,
        PSDU = wiseMex('wiMAC_AMSDUDataFrame()', LengthPSDU, FrameBodyType, Broadcast);
    elseif UseAMPDU,
        PSDU = wiseMex('wiMAC_AMPDUDataFrame()', LengthPSDU, FrameBodyType, Broadcast);
    else
        PSDU = wiseMex('wiMAC_DataFrame()', LengthPSDU, FrameBodyType, Broadcast);
    end
    if ~CleanFCS,
        k = (-3:0) + length(PSDU);     % FCS field at end of packet
        PSDU(k) = mod(PSDU(k)+1, 256); % create FCS errors
    end
end

[MAC_TX_D, T] = AddHeader(PSDU, Mbps, ShortPreamble,Greenfield);

y = []; m = [];

for i = 1:NumPkts,
    x = GetPacket(MAC_TX_D, T_IFS, T, Baseband);
    m = AddMarker(m, x);
    y = [y; x];  %#ok<AGROW>
end

%% Force Waveform Length to be a Multiple of 4
%  This is for compatibility with the ESG (requires multiple of 2) 
%  and the AWG520 (requires multiple of 4)
PadLength = 4 - mod(length(y), 4);
y = [y; zeros(PadLength, 1)];
m = [m, zeros(2, PadLength)];

%% FUNCTION: AddMarker
function MarkerOut = AddMarker(MarkerIn, x)
[B,A] = butter(7,0.1);
b = filtfilt(B,A,abs(x)); % smooth the amplitude profile
t = 0.05*max(b);          % detection threshold

k1 = find(abs(b)>t, 1 );         % start of packet
k2 = find(abs(b)>t, 1, 'last' ); % end of packet

m = zeros(2,length(x));          % 2 x N marker array
m(2,k1:k2) = 1;                  % set packet boundary marker
m(2,1) = 0; m(2,length(b)) = 0;  % ...but make sure end points are 0

if isempty(MarkerIn),
    m(1,1:80) = 1;               % 1 microsecond trigger pulse on Marker1
end

MarkerOut = [MarkerIn, m];

%% FUNCTION: GetPacket
function x = GetPacket(MAC_TX_D, T_IFS, Tpacket, Baseband)
aPadFront = 80;
aPadBack  = round( 20e6*T_IFS - aPadFront - 82 - 10 + 60);
if aPadBack < 0,
    aPadFront = aPadFront + aPadBack;
    aPadBack = 0;
end

bPadFront = 44;
bPadBack  = round( 11e6*T_IFS - bPadFront + 33);
if aPadBack < 0,
    bPadFront = bPadFront + bPadBack;
    bPadBack = 0;
end

wiParse_Line(sprintf('%s.aPadFront = %d',Baseband,aPadFront));
wiParse_Line(sprintf('%s.aPadBack  = %d',Baseband,aPadBack));

wiParse_Line(sprintf('%s.bPadFront = %d',Baseband,bPadFront));
wiParse_Line(sprintf('%s.bPadBack  = %d',Baseband,bPadBack));

x = wiseMex(sprintf('%s_TxPacket()',Baseband),MAC_TX_D);
n = length(x) - 3*80;
x = x(1:n); % trim 3 usec from end
L = round( 80e6 * (T_IFS + Tpacket) );

if L>length(x)
    warning('DwPhyLab_PacketWaveform:T_IFS', 'T_IFS shortened by %d samples',L-length(x));
    L = length(x);
end
x = x(1:L);


%% FUNCTION AddHeader
function [MAC_TX_D, T] = AddHeader(PSDU, Mbps, shortPreamble, Greenfield)

%%% Encode Data Rate
switch(Mbps)
    case    1, TxMode=2; RATE = 0;
    case    2, TxMode=2; RATE = 1 + 4*(shortPreamble ~= 0);
    case  5.5, TxMode=2; RATE = 2 + 4*(shortPreamble ~= 0);
    case   11, TxMode=2; RATE = 3 + 4*(shortPreamble ~= 0);
    case    6, TxMode=0; RATE = hex2dec('D');
    case    9, TxMode=0; RATE = hex2dec('F');
    case   12, TxMode=0; RATE = hex2dec('5');
    case   18, TxMode=0; RATE = hex2dec('7');
    case   24, TxMode=0; RATE = hex2dec('9');
    case   36, TxMode=0; RATE = hex2dec('B');
    case   48, TxMode=0; RATE = hex2dec('1');
    case   54, TxMode=0; RATE = hex2dec('3');
    case   72, TxMode=0; RATE = hex2dec('A');
    case  6.5, TxMode=3; MCS = 0;
    case   13, TxMode=3; MCS = 1;
    case 19.5, TxMode=3; MCS = 2;
    case   26, TxMode=3; MCS = 3;
    case   39, TxMode=3; MCS = 4;
    case   52, TxMode=3; MCS = 5;
    case 58.5, TxMode=3; MCS = 6;
    case   65, TxMode=3; MCS = 7;
    otherwise, error('Unsupported Data Rate [Mbps]'); 
end

%%% Generate MAC/PHY Header
switch TxMode,
    case {0, 2}, %802.11a/b/g
        MAC_TX_D(1) = 0;
        MAC_TX_D(2) = 64*TxMode;
        MAC_TX_D(3) = RATE*16 + floor(length(PSDU)/256);
        MAC_TX_D(4) = mod(length(PSDU), 256);
        MAC_TX_D(5) = 0;
        MAC_TX_D(6) = 0;
        MAC_TX_D(7) = 0;
        MAC_TX_D(8) = 0;
        
    case 3, % 802.11n
        MAC_TX_D(1) = 0;
        MAC_TX_D(2) = 64*TxMode;
        MAC_TX_D(3) = MCS;
        MAC_TX_D(4) = floor(length(PSDU)/256);
        MAC_TX_D(5) = mod(length(PSDU), 256);
        MAC_TX_D(6) = 8*Greenfield;
        MAC_TX_D(7) = 0;
        MAC_TX_D(8) = 0;
end
k=1:length(PSDU);
MAC_TX_D(8+k) = PSDU(k);

switch Mbps,
    case 1,
        T_usec = 192 + 8*length(PSDU)/Mbps;
    case {2, 5.5, 11},
        if shortPreamble == 0,
            T_usec = 192 + 8*length(PSDU)/Mbps;
        else
            T_usec = 96 + 8*length(PSDU)/Mbps;
        end
        
    case {6, 9, 12, 18, 24, 36, 48, 54, 72},
        T_usec = 20 + 4 * ceil( (8*length(PSDU) + 16 + 6) / 4/ Mbps );
        
    case {6.5, 13, 19.5, 26, 39, 52, 58.5, 65},
        T_usec = 36 + 4 * ceil( (8*length(PSDU) + 16 + 6) / 4/ Mbps );
        
    otherwise, error('Undefined Case');
end
T = T_usec / 1e6;

%% FUNCTION: ConfigWiseMex
function Baseband = ConfigWiseMex(Mbps, LengthPSDU,DataType)
clear wiseMex;

wiParse_Line('wiMAC_SetAddress(1, "000390012345") // DUT MAC Address');
wiParse_Line('wiMAC_SetAddress(2, "204241525420") // " BART "');
wiParse_Line('wiMAC_SetAddress(3, "205465737420") // " Test "');

if Mbps == 72, % allow Bermai proprietary mode using Mojave model
    wiParse_Line('wiPHY_SelectMethodByName("DW52")');
    Baseband = 'Mojave';
    wiParse_Line('Mojave.Register.WakeupTimeH = 0');
    wiParse_Line('Mojave.Register.WakeupTimeL = 8');
    wiParse_Line('Mojave.Register.DelayStdby  = 8');
    wiParse_Line('Mojave.Register.DelayBGEnB  = 6');
    wiParse_Line('Mojave.Register.DelayADC1   = 6');
    wiParse_Line('Mojave.Register.DelayModem  = 4');
    wiParse_Line('Mojave.Register.DelayDFE    = 1');
    wiParse_Line('Mojave.Register.TX_PRBS     = 127');
    wiParse_Line('Mojave.aPadFront = 80');
    wiParse_Line('Mojave.aPadBack  = 80');
else
    wiParse_Line('wiPHY_SelectMethodByName("NevadaPHY")');
    Baseband = 'Nevada';
    wiParse_Line('Nevada.Register.WakeupTimeH = 0');
    wiParse_Line('Nevada.Register.WakeupTimeL = 8');
    wiParse_Line('Nevada.Register.DelayStdby  = 8');
    wiParse_Line('Nevada.Register.DelayBGEnB  = 6');
    wiParse_Line('Nevada.Register.DelayADC1   = 6');
    wiParse_Line('Nevada.Register.DelayModem  = 4');
    wiParse_Line('Nevada.Register.DelayDFE    = 1');
    wiParse_Line('Nevada.Register.TX_PRBS     = 127');
    wiParse_Line('Nevada.aPadFront = 80');
    wiParse_Line('Nevada.aPadBack  = 80');
end
wiParse_Line(sprintf('wiTest.DataType = %d',DataType));
wiParse_Line(sprintf('wiTest.DataRate = %g',Mbps));
wiParse_Line(sprintf('wiTest.Length = %d',LengthPSDU));

%% REVISIONS
% 2010-02-16 BB - Switch from Mojave to Nevada baseband model and add support for
%                 HT-MF single spatial stream 802.11n rates.
% 2011-04-29 BB - Modify to allow T_IFS ~ 2E-6 for RIFS testing
