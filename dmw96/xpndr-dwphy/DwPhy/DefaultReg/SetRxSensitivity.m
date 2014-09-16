function SetRxSensitivity
% Generate register tuning for receiver sensitivity

DetectorLimit = -78;
dAGainLimit = -round((-40 - -65)/1.5); % stay above LNA switch point at -40 dB

dBm = -94:3:-50;
PndBm = -90; % noise floor for SigDetTh1 calculation
x = min(DetectorLimit, dBm);
s = 80 * 10.^((x+65)/20);
CQOut = 2*(11/4*s).^2;
q = round(log2(CQOut));
CQthreshold = 2 + 4*(q-1);
SigDetTh1 = min(32, max(4, floor(2*10.^((dBm-PndBm)/10))));
SigDetTh2 = round(2*s.^2 * 10^(0.5/10));
dAGain = floor((x - dBm)/1.5);
dAGain = max(dAGain, dAGainLimit);

tabspace = '        ';

i = 1;
fprintf('%s     if (dBm <= %d) { dBm = %d; SigDetTh1=%2d; SigDetTh2=%3d; CQthreshold=%2d; dAGain=%3d; }\n',...
    tabspace,dBm(i)-3,dBm(i)-3,SigDetTh1(i),SigDetTh2(i),CQthreshold(i),+2);

for i=1:(length(dBm)-1),
    fprintf('%selse if (dBm <= %d) { dBm = %d; SigDetTh1=%2d; SigDetTh2=%3d; CQthreshold=%2d; dAGain=%3d; }\n',...
        tabspace,dBm(i),dBm(i),SigDetTh1(i),SigDetTh2(i),CQthreshold(i),dAGain(i));
end

i = length(dBm);
fprintf('%selse                 { dBm = %d; SigDetTh1=%2d; SigDetTh2=%3d; CQthreshold=%2d; dAGain=%3d; }\n',...
    tabspace,       dBm(i),SigDetTh1(i),SigDetTh2(i),CQthreshold(i),dAGain(i));

% for i=1:length(dBm),
%     fprintf('%1.0f dBm CQthreshold=%2d, SigDetTh2=%4d, dAGain=%2d\n',dBm(i),CQthreshold(i),SigDetTh2(i),dAGain(i));
% end

