function Lp = Phy11n_mRX_HardDemapper(H, R, Kmod, N_BPSCS, varargin)
% Phy11n_mRX_HardDemapper -- Simple hard demapper for 20 MHz MIMO-OFDM

%   Written by Barrett Brickner
%   Copyright 2011 DSP Group, Inc. All rights reserved.

[NumRx, N_SS, NumSubcarriers] = size(H);
G = zeros([N_SS, NumRx, NumSubcarriers]); % swap the first two dimensions relative to H

for i = 1:NumSubcarriers,
    G(:,:,i) = 2*pinv(H(:,:,i))/Kmod;
end

t     = 1:N_BPSCS; % enumerate bits/subcarrier/stream
N_SYM = length(R); % number of OFDM symbols to process

Lp = zeros(N_SS, N_SYM*NumSubcarriers*N_BPSCS);
Y  = zeros(N_SS, NumSubcarriers);

for n = 1:N_SYM,

    for k = 1:NumSubcarriers,

        Y(:,k) = G(:,:,k) * R{n}(:,k); % equalizer
        y = Y(:,k);                    % extract per-subcarrier observation

        p0 = real(y);      L0 = p0;
        p1 = 4 - abs(p0);  L1 = p1;
        p2 = 2 - abs(p1);  L2 = p2;

        p3 = imag(y);      L3 = p3;
        p4 = 4 - abs(p3);  L4 = p4;
        p5 = 2 - abs(p4);  L5 = p5;

        switch N_BPSCS,
            case 1, L =  L0;
            case 2, L = [L0       L3];
            case 4, L = [L0   -L2 L3   -L5];
            case 6, L = [L0 L1 L2 L3 L4 L5];
            otherwise, error('Unsupported Case');
        end    

        Lp(:,t) = sign(L);
        t = t + N_BPSCS;
    end
    
end

