function [nPacketErr, nPackets, nBitErr, nBits] = wiTest_GetStatistics
%  wiTest_GetStatistics
%
%  [nPacketErr, nPackets, nBitErr, nBits] = wiTest_GetStatistics returns the
%  number of bit and packets total and in error.

%   Developed by Barrett Brickner
%   Copyright 2003 Bermai, Inc. All rights reserved.

[nPacketErr, nPackets, nBitErr, nBits] = wiseMex('wiTest_GetStatistics()');
