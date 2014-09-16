function ChannelEncoder
% Generate the switch-case FcMHz to channel number mapping for DwPhy

Fc = [2412:5:2472, 2484, 4920:20:4980, 5040:20:5080, 5170:10:5240, 5260:20:5320, 5500:20:5700, 5745:20:5805];

for i=1:length(Fc),
    if     (Fc(i) <  2484), n(i) = (Fc(i)-2407)/5 + 240;
    elseif (Fc(i) == 2484), n(i) = 14 + 240;
    elseif (Fc(i) <  5000), n(i) = (Fc(i)-4000)/5;
    elseif (Fc(i) <  6000), n(i) = (Fc(i)-5000)/5;
    else error(sprintf('Undefined case, Fc(%d) = %d MHz',i,Fc(i)));
    end
end

fprintf('switch( FcMHz )\n');
fprintf('{\n');
for i=1:length(Fc),
    fprintf('    case %d: n = %3d; break;\n',Fc(i),n(i));
end
fprintf('    default: return DWPHY_ERROR_UNSUPPORTED_CHANNEL;\n');
fprintf('}\n');
