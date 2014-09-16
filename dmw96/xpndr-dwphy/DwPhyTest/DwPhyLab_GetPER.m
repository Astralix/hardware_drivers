function data = DwPhyLab_GetPER
%data = DwPhyLab_GetPER
%   Trigger the ESG sequence and get the ReceivedFragmentCount and other data.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc. All Rights Reserved

%% Receive Packets and Get Results
% Packet counters are not reset, so they are read before and after receiving
% packets so an incremental value can be obtained. After triggering the
% signal generator, there is a delay corresponding to 50 ms longer than the
% actual waveform duration. This value accomodates hardware and software
% processing of the packets before retrieving the final counter values.
T = DwPhyLab_Parameters('TsequencePER') + 0.050;

% initial counter state
C0 = GetPerCounters;

% trigger the ESG DualARB
DwPhyLab_SendCommand('ESG','*TRG');

if T < 3,
    pause(T);
else
    h = waitbar(0.0,'Receiving packets...','Name','PER','WindowStyle','modal');
    P = get(h,'Position');
    set(h,'Position',[P(1)+40, P(2)-60, P(3), P(4)]);
    pause(T-floor(T));
    for i=1:floor(T),
        if ishandle(h), waitbar(i/T, h); end
        pause(1.0);
    end
    if ishandle(h), close(h); end
end

C = GetPerCounters; % get the counter state after the packet sequence

% compute incremental counts
data.ReceivedFragmentCount = C.ReceivedFragmentCount - C0.ReceivedFragmentCount;
data.FCSErrorCount         = C.FCSErrorCount         - C0.FCSErrorCount;

% handle wrap-around of 32-bit counters
if data.ReceivedFragmentCount < 0,
    data.ReceivedFragmentCount = data.ReceivedFragmentCount + 2^32;
end
if data.FCSErrorCount < 0,
    data.FCSErrorCount = data.FCSErrorCount + 2^32;
end

% The underlying software returns parameters corresponding to the last
% received path (not necessarily from the latest trigger). If no (valid)
% packets are received, clear the values to avoid confusion.
if data.ReceivedFragmentCount ~= 0,
    data.Length    = C.Length;
    data.RxBitRate = C.RxBitRate;
    data.RSSI0     = C.RSSI0;
    data.RSSI1     = C.RSSI1;
else
    data.Length = 0;
    data.RxBitRate = 0;
    data.RSSI0 = 0;
    data.RSSI1 = 0;
end

% Compute RSSI as the maximum of both receive paths. If a signle path is
% enabled, the unused path will return 0 so max(A,B) will select the
% correct result
data.RSSI = max(data.RSSI0, data.RSSI1);


%% GetPerCounters
function data = GetPerCounters

% Retrieve the raw data values
data = DwPhyMex('RvClientGetPerCounters');

% Correct the packet length. The MAC reports a value not including the FCS
% field (4 bytes). After correction, the value is the PSDU length
if data.Length > 0, 
    data.Length = data.Length + 4; % add 4 bytes for FCS
end

% RSSI is the maximum RSSI over both receive paths. This works regardless
% of whether one or both is enabled.
data.RSSI = max([data.RSSI0 data.RSSI1]);
       
