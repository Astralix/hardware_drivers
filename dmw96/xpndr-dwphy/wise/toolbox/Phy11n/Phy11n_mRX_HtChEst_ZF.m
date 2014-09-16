function H = Phy11n_mRX_HtChEst_ZF(HT_LTF, N_SS, varargin)
% Phy11n_mRX_HtChEst_ZF -- Zero-forcing channel estimate for 20 MHz OFDM HT-LTF
%
%   H = Phy11n_mRX_HtChEst_ZF(HT_LTF, N_SS) returns the channel estimate in H with
%   dimensions H(NumRx, N_SS, 52) where NumRx is the number of receive antennas and
%   parameter N_SS is the number of spatial streams. HT_LTF is a cell array containing
%   the 52 data subcarriers produced by an FFT of the HT-LTF fields; each cell element
%   corresponds to a seperate symbol. Each symbol in HT_LTF is a NumRx x NumSubcarriers
%   matrix.

%   Written by Barrett Brickner
%   Copyright 2011 DSP Group, Inc. All rights reserved.

P_HTLTF = [1 -1 1 1; 1 1 -1 1; 1 1 1 -1; -1 1 1 1];

X52 = [ 1, 1, 1, 1,-1,-1, 1,-1, 1,-1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1,-1, 1, 1, 1, 1, ...
        1,-1,-1, 1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1, 1,-1,-1, 1,-1,-1, 1, 1, 1, 1,-1,-1];

[NumRx, NumSubcarriers] = size(HT_LTF{1});
if NumSubcarriers ~= 52,
    error('This function can only process data subcarriers (52 per HT-OFDM symbol)');
end

P = P_HTLTF(1:N_SS, 1:N_SS) * 2/N_SS;

Y = zeros(NumRx, N_SS, NumSubcarriers);
H = zeros(NumRx, N_SS, NumSubcarriers);

for i = 1:N_SS,
    for j = 1:NumRx,
        Y(j,i,:) = HT_LTF{i}(j,:);
    end
end
for i = 1:NumSubcarriers,
    Z = X52(i)*P;
    H(:,:,i) = Y(:,:,i) * Z';
end
disp('hi');

