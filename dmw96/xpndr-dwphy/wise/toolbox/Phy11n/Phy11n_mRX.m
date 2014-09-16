function PSDU = Phy11n_mRX(R, HtChEstFn, HtDemapperFn)
% Phy11n_mRX -- Matlab receiver model for Phy11n -- 20 MHz HT-MF OFDM
%
%   PSDU = Phy11n_mRX(R, HtChEstFn, HtDemapperFn) processes receive time-domain array R 
%   and returns decoded data array PSDU.

%   Written by Barrett Brickner
%   Copyright 2011 DSP Group, Inc. All rights reserved.

    %%% Nested functions to perform FFT and return 48 (L-OFDM) or 52 (HT-OFDM) subcarriers
    function Y = GetSymbol48(n)
        Y = fftshift(fft(R(n+(1:64),:)));
        Y = Y(kData48,:).';
    end

    function Y = GetSymbol52(n)
        Y = fftshift(fft(R(n+(1:64),:)));
        Y = Y(kData52,:).';
    end

% kData48 and kData52 are indices to the data subcarriers in the L-OFDM and HT-OFDM
% symbols, respectively.
%
kData48 = logical([0 0 0 0 0 0 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 0 ...
                   1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 0 0 0 0 0]);

kData52 = logical([0 0 0 0 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 0 ...
                   1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 0 0 0]);

% If methods for HT-OFDM channel estimation and MIMO demapping are not specified, select
% default methods.
if nargin < 2 || isempty(HtChEstFn),
    HtChEstFn = @Phy11n_mRX_HtChEst_ZF;
end

if nargin < 3 || isempty(HtDemapperFn),
    HtDemapperFn = @Phy11n_mRX_HardDemapper;
end

PSDU = [];

dn = -4; % offset in FFT position (model nominal symbol sync position)
n_L_LTF  = 160 + 32 + dn;       % start of L-LTF
n_L_SIG  = n_L_LTF + 2*64 + 16; % start of L-SIG
n_HT_SIG = n_L_SIG + 80;        % start of HT-SIG
n_HT_LTF = n_HT_SIG + 3*80;     % start of HT-LTF

% L-OFDM Channel Estimation
X = { GetSymbol48(n_L_LTF), GetSymbol48(n_L_LTF+64) };
[~, H2, G] = GetChannelEstimate_L_LTF(X);

% Estimate noise power
% The two L-LTFs should be identical, so noise can be estimated as the difference.
% Assume zero-mean and measure average power; ignores the usual (n-1)/n bias correction
Pn = mean(mean(abs(X{1} - X{2}).^2));

% Check L-SIG (Must be 6 Mbps)
Valid = ProcessLSIG(GetSymbol48(n_L_SIG), G, H2);
if ~Valid, return; end

% HT-SIG Processing
X = {GetSymbol48(n_HT_SIG), GetSymbol48(n_HT_SIG+80)};
[MCS, LENGTH, Valid] = ProcessHTSIG(X, G, H2);
if ~Valid, return; end
[N_SYM, N_SS, CodeRate12, N_BPSCS, N_CBPSS, Kmod] = GetHTParameters(MCS, LENGTH);

N_LTF = 2^ceil(log2(N_SS));    % number of HT-LTF fields (ignore N_ESS)
n_DATA = n_HT_LTF + 80*N_LTF;  % start of DATA

% HT Channel Estimation
X = cell(1, N_LTF);
for i = 1:N_LTF,
    X{i} = GetSymbol52(n_HT_LTF + 80*(i-1));
end
H = HtChEstFn(X, N_SS, Pn);

% HT-DATA Processing
X = cell(1, N_SYM);
for i = 1:N_SYM,
    X{i} = GetSymbol52(n_DATA + 80*(i-1));
end
Lp = HtDemapperFn(H ,X, Kmod, N_BPSCS, Pn);

Ls = zeros(size(Lp));    
for i = 1:N_SS,
    Ls(i,:) = wiseMex('Phy11n_RX_Deinterleave',N_CBPSS, N_BPSCS, i-1, Lp(i,:));
end

Lc = StreamDeparser(Ls, N_BPSCS, LENGTH, CodeRate12);
b  = wiseMex('Phy11n_RX_ViterbiDecoder',Lc,CodeRate12,63);
a  = wiseMex('Phy11n_RX_Descrambler',b);

% PSDU serial-to-parallel conversion
w = 2.^(0:7)';
k = 16 + (1:8*LENGTH);
PSDU = reshape(a(k),8,LENGTH)' * w;
    
end

%% SUBFUNCTIONS

function Valid = ProcessLSIG(R, G, H2)
% Process the 6 Mbps L-SIG symbol in R using equalizer G and weights H2. The purpose is to
% verify the symbol has a valid format (parity check) and that RATE indicates 6 Mbps are
% required for HT-MF packets.

    Y = G .* R;
    Lp = sum(real(Y) .* H2);

    [j,k] = SignalInterleaverMap;
    
    Lc(k+1) = Lp(j+1);
    x = wiseMex('Phy11n_RX_ViterbiDecoder',Lc,6,64);
    Valid = mod(sum(x),2) == 0; % Parity check

    RATE = x(1:4);
    if ~all(RATE == [1 1 0 1]')
        Valid = 0; % L-SIG does not indicate 6 Mbps
    end
end

function [MCS, LENGTH, Valid] = ProcessHTSIG(R, G, H2)
% Process the QBPSK HT-SIG symbol pairs in R using equalizer G and weights H2. The symbol
% is decoded and MCS and LENGTH are extracted. Valid indicates whether the symbol is valid
% based on the CRC and whether fields are set to values supported by this simulator.

    Y1 = G .* R{1};
    Y2 = G .* R{2};

    Lp1 = sum(imag(Y1) .* H2); % demapper (LLR) output for QBPSK
    Lp2 = sum(imag(Y2) .* H2);

    [j,~] = SignalInterleaverMap;

    Lc = [Lp1(j+1) Lp2(j+1)];
    x = wiseMex('Phy11n_RX_ViterbiDecoder',Lc,6,64);
    MCS    = GetBinaryField(x(1:7));
    LENGTH = GetBinaryField(x(9:24));
    Valid = CheckCRC(x);
    if MCS > 31, Valid = 0; end % only support MCS = {0..31}
    if LENGTH == 0, Valid = 0; end % do not support NULL packets
end

function [N_SYM, N_SS, CodeRate12, N_BPSCS, N_CBPSS, Kmod] = GetHTParameters(MCS, LENGTH)
% Returns parameters to control demodulation/decoding of the HT-OFDM data symbols. Only
% MCS 0 - 31 are supported.

    switch mod(MCS, 8),
        case 0, CodeRate = 1/2; N_BPSCS = 1;
        case 1, CodeRate = 1/2; N_BPSCS = 2;
        case 2, CodeRate = 3/4; N_BPSCS = 2;
        case 3, CodeRate = 1/2; N_BPSCS = 4;
        case 4, CodeRate = 3/4; N_BPSCS = 4;
        case 5, CodeRate = 2/3; N_BPSCS = 6;
        case 6, CodeRate = 3/4; N_BPSCS = 6;
        case 7, CodeRate = 5/6; N_BPSCS = 6;
        otherwise, error('Undefined case');
    end

    switch N_BPSCS,
        case 1, Kmod = 1;
        case 2, Kmod = 1/sqrt(2);
        case 4, Kmod = 1/sqrt(10);
        case 6, Kmod = 1/sqrt(42);
        otherwise, error('Undefined Case');
    end

    CodeRate12 = 12 * CodeRate;
    N_SS = floor(MCS/8) + 1;
    N_CBPSS = 52 * N_BPSCS;
    N_CBPS = N_CBPSS * N_SS;
    N_DBPS = N_CBPS * CodeRate;

    N_SYM = ceil((8*LENGTH + 16 + 6)/N_DBPS);
end


function Lc = StreamDeparser(Ls, N_BPSCS, LENGTH, CodeRate12)
% Input Ls contains demapper output data in rows corresponding to each spatial stream.
% This function merges the data into a single vector in output Lc.

    s = max(1, N_BPSCS/2);
    k = [];
    for i = 1:size(Ls,1),
        k = [k (1:s)+(i-1)*size(Ls,2)]; %#ok<AGROW>
    end
    N = numel(Ls) / numel(k);
    K1 = ones(N,1) * k;
    C = s*((1:N)'-1) * ones(1, numel(k));
    K2 = K1 + C;
    k = reshape(K2', 1, numel(K2));
    Ls = Ls';
    Lc = Ls(k);

    Nc = ceil((8*LENGTH + 16 + 6) * 12 / CodeRate12);
    Lc = Lc(1:Nc);
end

function Pass = CheckCRC(x)
% Check the 8-bit CRC used in HT-SIG

    c = ones(1,8);
    for k = 1:34,
        y = xor(x(k), c(8));
        c = [y c(1:7)];
        c(2) = xor(c(2), y);
        c(3) = xor(c(3), y);
    end
    z = reshape(x(42:-1:35), size(c));
    Pass = ~any(c == z);
end


function y = GetBinaryField(x)
% Convert a binary array in x to an integer where the binary data are arranged with the
% LSB at position x(1).

    n = length(x);
    x = reshape(x,1,n);
    y = sum( x .* 2.^(0:(n-1)) );
end


function [H, H2, G] = GetChannelEstimate_L_LTF(L)
% Channel estimation based on the L-LTF symbols in L. The result is used to process both
% the L-SIG and HT-SIG fields.

    X48 = [ 1  1 -1 -1  1 -1  1 -1  1  1  1  1  1  1 -1 -1  1  1  1 -1  1  1  1  1 ...
            1 -1 -1  1  1 -1 -1  1 -1 -1 -1 -1 -1  1  1 -1 -1  1 -1 -1  1  1  1  1];

    NumRx = size(L{1},1);
    X = ones(NumRx, 1) * X48;
    
    H = X .* (L{1} + L{2}) / 2;
    H2 = abs(H).^2;
    G = 1 ./ H;
end


function [j,k] = SignalInterleaverMap
% Interleaver mapping for L-SIG and HT-SIG (48 subcarriers at 6 Mbps)

    N_CBPS = 48;
    NCol = 16;
    s = 1;
    k = 0:(N_CBPS-1);
    i = floor(N_CBPS/NCol) * mod(k,NCol) + floor(k/NCol);
    j = s*floor(i/s) + mod(i + N_CBPS - (NCol * floor(i/N_CBPS)), s);
end

% function Lp = SingleStreamDemapper(H,r,n,N_SYM,Kmod,N_BPSC)
% %
% H2 = abs(H).^2;
% G = 1./ Kmod ./ H; 
% 
% Lp = [];
% 
% Window = n + (1:64);
% 
% for Symbol = 1:N_SYM,
%     
%     Y = G .* GetSymbol52(r(Window, :));
%     Window = Window + 80;
%     
%     p0 = real(Y);      L0 = sum(H2.*p0, 2);
%     p1 = 4 - abs(p0);  L1 = sum(H2.*p1, 2);
%     p2 = 2 - abs(p1);  L2 = sum(H2.*p2, 2);
%     
%     p3 = imag(Y);      L3 = sum(H2.*p3, 2);
%     p4 = 4 - abs(p3);  L4 = sum(H2.*p4, 2);
%     p5 = 2 - abs(p4);  L5 = sum(H2.*p5, 2);
%     
%     switch N_BPSC,
%         case 1, L =  L0;
%         case 2, L = [L0       L3];
%         case 4, L = [L0   -L2 L3   -L5];
%         case 6, L = [L0 L1 L2 L3 L4 L5];
%         otherwise, error('Unsupported Case');
%     end    
%     
%     L = reshape(transpose(L), 1, numel(L));
%     Lp = [Lp L]; %#ok<AGROW>
% end
% end

