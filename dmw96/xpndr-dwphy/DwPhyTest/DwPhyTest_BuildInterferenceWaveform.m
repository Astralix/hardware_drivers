function data = DwPhyTest_BuildInterferenceWaveform(InterferenceType)
%DwPhyTest_BuildInterferenceWaveform(InterferenceType)
%   Generate a waveform to be used for interference testing. InterfereceType values
%   include 'ACI-OFDM','ACI-CCK','DECT'

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.

if nargin<1, InterferenceType='ACI-OFDM'; end

y = []; m = [];

switch upper(InterferenceType),

    % 54 Mbps, Bad FCS Packets for Step-up/down restart, 50% duty cycle
    case '54MBPS-BADFCS-T_IFS=172E-6',
        y = [y; DwPhyLab_PacketWaveform('Mbps = 54','LengthPSDU = 1000','T_IFS = 172e-6','CleanFCS = 0')];
        m = zeros(size(y));
        m(1:80) = 1;
        if nargout == 0,
            DwPhyLab_WriteWaveformAWG520('54MBPS-BADFCS-T_IFS=172E-6-I.wfm', real(y)+512, 80, m, m);
            DwPhyLab_WriteWaveformAWG520('54MBPS-BADFCS-T_IFS=172E-6-Q.wfm', imag(y)+512, 80, m, m);
        end
    
    % OFDM Linear Packets
    case 'ACI-OFDM',
        % The duty cycle of the packets will cause the ESG to produce a signal that
        % is +0.25 dB stronger than the target value
        y = [y; DwPhyLab_PacketWaveform('Mbps =  6','LengthPSDU =  150','T_IFS = 19e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps =  9','LengthPSDU =  300','T_IFS = 21e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 12','LengthPSDU =  450','T_IFS = 24e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 18','LengthPSDU =  600','T_IFS = 27e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 24','LengthPSDU =  750','T_IFS = 30e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 36','LengthPSDU =  900','T_IFS = 33e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 48','LengthPSDU = 1050','T_IFS = 36e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 54','LengthPSDU = 1500','T_IFS = 45.7e-6','CleanFCS = 0')];
        m = zeros(size(y));
        m(1:80) = 1;
        
        if nargout == 0,
            DwPhyLab_WriteWaveformAWG520('ACI-OFDM-I.wfm', real(y)+512, 80, m, m);
            DwPhyLab_WriteWaveformAWG520('ACI-OFDM-Q.wfm', imag(y)+512, 80, m, m);
        end

    % DSSS/CCK Linear Packets
    case 'ACI-CCK',
        % The duty cycle of the packets will cause the ESG to produce a signal that
        % is +0.25 dB stronger than the target value
        y = [y; DwPhyLab_PacketWaveform('Mbps = 11','LengthPSDU = 1000','T_IFS = 50e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps = 11','LengthPSDU =   50','T_IFS = 47e-6','CleanFCS = 0')];
        y = [y; DwPhyLab_PacketWaveform('Mbps =  2','LengthPSDU =  264','T_IFS = 50e-6','CleanFCS = 0')];

        if nargout == 0,
            DwPhyLab_WriteWaveformAWG520('ACI-CCK-I.wfm', real(y)+512, 80, m, m);
            DwPhyLab_WriteWaveformAWG520('ACI-CCK-Q.wfm', imag(y)+512, 80, m, m);
        end
        
    % DECT Transmit Spectrum
    case 'DECT',
        
        Nb = 20e3;  % number of data bits
        Fb = 1.152; % data bit rate (Mbps)
        M = 10;     % oversampling
        FsMHz = M*Fb;

        %%% Gaussian filter
        BT = 0.5;
        k = -2:(1/M):2;
        alpha = sqrt(log(2)/2)/(BT);
        h = (sqrt(pi)/alpha) * exp(-(k*pi/alpha).^2); 
        h = h./sum(h);

        %%% Oversampled random data waveform
        a = 1;
        while( mod(sum(a), 4) )           % condition so endpoints match
            a = 2*round(rand(Nb,1)) - 1;
        end
        x = upsample(a,M);
        x = conv(ones(M,1),x);

        %%% Baseband equivalent modulation
        b = (1/M) * conv(h,x);
        c = pi/2 * cumsum(b);
        p = c;
        y = exp(j*p);
        m = mod(length(y), 4);
        y = y(1:(length(y) - m)); % make length a multiple of 4 per AWG520 requirements
        
        % [P,F] = psd(y,100*M,FsMHz); plot(F,10*log10(P/max(P))); grid on; axis([0 2 -60 0]);

        m = zeros(size(y));
        m(1:80) = 1;
        
        if nargout == 0,
            DwPhyLab_WriteWaveformAWG520('DECT-I.wfm', real(y), FsMHz, m, m);
            DwPhyLab_WriteWaveformAWG520('DECT-Q.wfm', imag(y), FsMHz, m, m);
        end

    otherwise,
        error('Undefined case');
end

%%% Output if requested
if nargout > 0,
    data.y = y;
    data.m = m;
    data.InterferenceType = InterferenceType;
    if ~isempty(who('FsMHz'))
        data.FsMHz = FsMHz;
    else
        data.FsMHz = 80;
    end
end