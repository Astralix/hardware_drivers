function [S,Fs,Prms,TxData] = wiseMex_TxPacket(DataRate, TxData)
% [S,Fs,Prms] = wiseMex_TxPacket(DataRate, TxData)
%      Call wiPHY_TX with the specified DataRate[bps] and the data byte
%      vector TxData. The transmit signal is returned in the matrix S where
%      each column corresponds to a transmit stream. The sample rate is
%      Fs[Hz], and the signal has power Prms (1 Ohm) during the packet.
%
% [S,Fs,Prms,TxData] = wiseMex_TxPacket(DataRate, Length)
%      Operates similar to above except TxData is generated randomly using
%      the specified Length.

Length = length(TxData);
if(Length==1)
   Length = TxData;
   TxData = round(255*rand(Length,1));
end
[S,Fs,Prms] = wiseMex('wiPHY_TX()',DataRate,TxData);

