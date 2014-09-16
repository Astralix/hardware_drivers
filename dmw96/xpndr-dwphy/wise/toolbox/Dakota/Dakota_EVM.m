function [evm, y0, e0] = Dakota_EVM()
%Dakota_EVM - Compute a packet Error Vector Magnitude (EVM)
%
%   [evm, y0] = Dakota_EVM.
%
%   Inputs: none, but Dakota_RX2() must be run first.
%
%   Outputs:evm -- Error vector magnitude per IEEE Std 802.11a-1999 S17.3.9.7
%           y0  -- Equalized data subcarriers, path 0
%           e0  -- Error vector for data subcarriers, path 0-
%
%   Usage:  This function uses residual data from a call to Dakota_RX2() to
%           compute the EVM for the packet.

%   Developed by Barrett Brickner
%   Copyright 2002 Bermai, Inc. All rights reserved.

[evm, y0, e0] = wiseMex('Dakota_EVM()');
