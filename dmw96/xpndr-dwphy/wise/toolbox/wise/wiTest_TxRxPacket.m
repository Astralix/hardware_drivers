function wiTest_TxRxPacket(Pr_dBm, bps, nBytes)
%  wiTest_TxRxPacket(Pr_dBm, BPS, nBytes)
%
%  Execute a single Tx/Rx cycle for a packet at BPS bits-per-second data
%  rate and having a length of nBytes. The received signal is scale to give
%  a nonminal receive power of Pr_dBm.

%   Developed by Barrett Brickner
%   Copyright 2002 Bermai, Inc. All rights reserved.

if(nargin~=3), error('Incorrect number of input arguments.'); end
wiseMex('wiTest_TxRxPacket()',Pr_dBm, bps, nBytes);
