function data = DwPhyTest_TxCal(varargin)
% data = DwPhyTest_TxCal(...)
%    Calibrate TX output power.

% If you have problems with the calibration, it is recommended to run in
% VERBOSE
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

%% Default and User Parameters
Mbps = 6;
LengthPSDU = 1500;
fcMHz = 2442;
PacketType = 0;
AutoRange = 0;
DetectorTrigger = 1;
VERBOSE = 0;
curve_sweep_values = 2:2:23;  % Default measuring points
curve_sweep_values = 1:4:25;  % Default measuring points
pp = 5;

max_power_level_calib_dBm = 25;

DwPhyLab_EvaluateVarargin(varargin, ...
    {'Mbps', 'LengthPSDU', 'fcMHz', 'PacketType', ...
    'AutoRange', 'VGA_CAL_INTER', 'DetectorTrigger', ...
    'VERBOSE','Power_low_limit', 'curve_sweep_values', 'det_div_vovr', 'pp'});


%% Run Test
DwPhyTest_RunTestCommon;
data.Ploss = DwPhyLab_TxCableLoss(fcMHz);

% PAC parameters
Normal_state_attn = DwPhyLab_ReadRegister(32768 - 128 + 217,'5:0');
det_div_vovr = DwPhyLab_ReadRegister(32768 - 128 + 184,2);

PSA = DwPhyLab_OpenPSA;

% save the original timeout value and increase to cover the expected test time so the
% *OPC? query does not cause an error message
[status, description, Timeout] = DwPhyLab_GetVisaTimeout(PSA);
if status ~= 0, error(description); end
DwPhyLab_SetVisaTimeout(PSA, 10000);

% DwPhyLab_SendCommand(PSA,':DISPLAY:ENABLE OFF');
DwPhyLab_SendCommand(PSA,':SYST:PRES');
DwPhyLab_SendCommand(PSA,':INST SA'); % mode = Spectrum Analysis
DwPhyLab_SendCommand(PSA,':CONF:SAN'); % Measure Off
DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON; *WAI'); % continuous sweep
DwPhyLab_SendCommand(PSA,':CALIBRATION:AUTO ALERT');
DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO:RULES NORMAL');

DwPhyLab_SendCommand(PSA,':SENSE:RADIO:STANDARD:EAMEAS YES');
DwPhyLab_SendCommand(PSA,':CONFIGURE:CHPOWER');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:BANDWIDTH:INTEGRATION 20.0 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:FREQ:SPAN 40 MHz');
DwPhyLab_SendCommand(PSA,':SENSE:CHPOWER:AVERAGE:STATE OFF');

if AutoRange,
    DwPhyLab_TxBurst(1e9, Mbps, LengthPSDU, MaxTXPWRLVL, PacketType)
    DwPhyLab_SendCommand(PSA,':INITIATE:CONTINUOUS ON; *WAI'); % continuous sweep
    DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:RANGE:AUTO ONCE; *WAI');
else
    Atten = round( 30 - min(data.Ploss) );
    RefLevel = 23;
    DwPhyLab_SendCommand(PSA,':DISPLAY:WINDOW:TRACE:Y:SCALE:RLEVEL %d dBm',RefLevel);
    DwPhyLab_SendCommand(PSA,':SENSE:POWER:RF:ATTENUATION %d',Atten);
end


DwPhyLab_TxBurst(0); % stop any on-going burst transmissions
DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME:AUTO:RULES ACCURACY; *WAI');
pause(0.5);
DwPhyLab_SendCommand(PSA, ':SENSE:FREQUENCY:CENTER %5.3fMHz',fcMHz);

DwPhyLab_TxBurst(0); pause(0.2); % stop any on-going burst transmissions
DwPhyLab_SetChannelFreq(fcMHz);

% igor
DwPhyLab_SendCommand(PSA,':INIT:CHPOWER');
switch DetectorTrigger
    case 1,
        DwPhyLab_SendCommand(PSA,':TRIG:SEQ:SOUR IMM');     % Free Run
        DwPhyLab_SendCommand(PSA,':SENSE:DET AVER');        % Avr Detector
        DwPhyLab_SendCommand(PSA,':SENSE:SWEEP:TIME 500 ms');
        DwPhyLab_SendCommand(PSA,'INIT:CONT ON');
    case 2,
        DwPhyLab_SendCommand(PSA,':TRIG:SEQ:SOUR IMM');         % Free Run
        DwPhyLab_SendCommand(PSA,':SENSE:DET SAMP');            % Avr Detector
        DwPhyLab_SendCommand(PSA,':TRIG:SEQ:SOUR VID');         % Video
        DwPhyLab_SendCommand(PSA,':TRIG:SEQ:VID:LEV -70 dBm');  % Trigger Level

    otherwise
        % DO NOTHING
        % keeping the current settings
end

DwPhyLab_Wake;



% fetching the current maximum power level mapped to 63 and keepig it
% aside
regular_max_power = DwPhyLab_ReadRegister(32768 - 128 + 219,'6:0');
DwPhyLab_WriteRegister(32768 - 128 + 219, max_power_level_calib_dBm*2);

if RadioIsRF22B()
    Reg182 = DwPhyLab_ReadRegister(32768 - 128 + 182, '7:7');
    Reg80  = DwPhyLab_ReadRegister(256 + 80, '1:1');
    DwPhyLab_WriteRegister(32768 - 128 + 182, '7:7',0);
    DwPhyLab_WriteRegister(256 + 80, '1:1',0);
end

%% Power detector Calibration
vga_switching_sequence(1) = 0;
vga_switching_sequence(2) = DwPhyLab_ReadRegister(32768 - 128 + 191,'3:0');
vga_switching_sequence(3) = DwPhyLab_ReadRegister(32768 - 128 + 191,'7:4');
vga_switching_sequence(4) = DwPhyLab_ReadRegister(32768 - 128 + 192,'3:0');
vga_switching_sequence(5) = DwPhyLab_ReadRegister(32768 - 128 + 192,'7:4');
vga_switching_sequence(6) = DwPhyLab_ReadRegister(32768 - 128 + 193,'3:0');
vga_switching_sequence(7) = 15;

rgf_curve_values = 1:4:25;  % Default measuring points
vga_switching_value = ConvertVgaIndex2Value(vga_switching_sequence);

DwPhyLab_WriteRegister(32768 - 128 + 184,4,1); %det_high_set_eovr


DwPhyLab_WriteRegister(32768 - 128 + 184,2,det_div_vovr); %det_div_vovr
if det_div_vovr
    DwPhyLab_WriteRegister(32768 - 128 + 186,150); %det_div_vovr
else
    DwPhyLab_WriteRegister(32768 - 128 + 186,75); %det_div_vovr
end

Pmeas0 = [];
Pmeas1 = [];
dpl0 = [];
dpl1 = [];


%% VGA calibration
DwPhyLab_WriteRegister(32768 - 128 + 187,'5:0',Normal_state_attn); % if_attn_vovr
DwPhyLab_WriteRegister(32768 - 128 + 204,1,1);  % vga_state_eovr
DwPhyLab_WriteRegister(32768 - 128 + 187,6,1); % if_attn_eovr

VGA_Pmeas = 0;
DwPhyLab_TxBurst(1e10, Mbps, LengthPSDU, 63, PacketType);

for vga_state=3:7
    DwPhyLab_WriteRegister(32768 - 128 + 201,vga_switching_value(vga_state)); % vga_state_vovr
    p = GetTxPower(PSA,data.Ploss);
    prev_VGA_Pmeas = VGA_Pmeas;
    VGA_Pmeas = p(end);
    % Write to registers
    if (vga_state == 7) && (prev_VGA_Pmeas > VGA_Pmeas)
        prev_VGA_Pmeas = prev_VGA_Pmeas + 3;
        if VERBOSE
            fprintf('\nVGA state %x has been artifically modified from %f to %f\n\n', vga_switching_value(vga_state), ...
                VGA_Pmeas, prev_VGA_Pmeas);
        end
        VGA_Pmeas = prev_VGA_Pmeas;
    end
    DwPhyLab_WriteRegister(32768 - 128 + 193 + vga_state,'6:0',max([round(VGA_Pmeas*2) 0]));
    if VERBOSE
        fprintf('VGA state : %x, Power : %f\n',vga_switching_value(vga_state),VGA_Pmeas);
    end
end

DwPhyLab_WriteRegister(32768 - 128 + 204,1,0);  % vga_state_eovr
DwPhyLab_WriteRegister(32768 - 128 + 187,6,0); % if_attn_eovr

%% PD curves mapping
DwPhyLab_WriteRegister(32768 - 128 + 204,2,1); % force measurment
DwPhyLab_WriteRegister(32768 - 128 + 202,0,0); % cl_en

for indx=1:length(curve_sweep_values)
    Ptarget = curve_sweep_values(indx);
    DwPhyLab_TxBurst(1e10, Mbps, LengthPSDU, 63-2*(max_power_level_calib_dBm-Ptarget), PacketType);
    if VERBOSE
        vga_value = DwPhyLab_ReadRegister(32768 - 128 + 222);
        attn_value = DwPhyLab_ReadRegister(32768 - 128 + 188, '5:0');
        fprintf('state: %d, vga : %x, attn: %d\n',indx,vga_value, attn_value)
    end
    DwPhyLab_WriteRegister(32768 - 128 + 184,5,0);  % det_high_set_vovr
    pause(0.1);
    p = GetTxPower(PSA,data.Ploss);
    DwPhyLab_TxBurst(0, Mbps, LengthPSDU, 63-2*(max_power_level_calib_dBm-Ptarget), PacketType);
    dpl0(indx) = get_dpl_out(); %#ok<AGROW>
    Pmeas0(indx) = p(end); %#ok<AGROW>
    
    DwPhyLab_WriteRegister(32768 - 128 + 184,5,1);  % det_high_set_vovr
    DwPhyLab_TxBurst(1e10, Mbps, LengthPSDU, 63-2*(max_power_level_calib_dBm-Ptarget), PacketType);
    pause(0.1);
    DwPhyLab_TxBurst(0, Mbps, LengthPSDU, 63-2*(max_power_level_calib_dBm-Ptarget), PacketType);
    dpl1(indx) = get_dpl_out(); %#ok<AGROW>
    Pmeas1(indx) = p(end);  %#ok<AGROW> % Get last measrment
    if VERBOSE
        fprintf('Ptarget %d,Pmeas0 : %f, dpl0 : %d, Pmeas1 : %f, dpl1 : %d\n',Ptarget,Pmeas0(indx),dpl0(indx),Pmeas1(indx),dpl1(indx));
    end
end

DwPhyLab_WriteRegister(32768 - 128 + 204,2,0); % force measurment
DwPhyLab_WriteRegister(32768 - 128 + 202,0,1); % cl_en
DwPhyLab_WriteRegister(32768 - 128 + 184,4,0); %det_high_set_eovr

DwPhyLab_TxBurst(0); % stop any on-going burst transmissions

%% Post processing

data.dpl0_pp = post_process(Pmeas0, dpl0, rgf_curve_values, pp, 3,VERBOSE);
data.dpl1_pp = post_process(Pmeas1, dpl1, rgf_curve_values, pp, 5,VERBOSE);


data.Pmeas0 = Pmeas0;
data.Pmeas1 = Pmeas1;
data.dpl0 = dpl0;
data.dpl1 = dpl1;
data.curve_sweep_values = curve_sweep_values;
data.rgf_curve_values = rgf_curve_values;

% Write to registers
DwPhyLab_WriteRegister(32768 - 128 + 205,floor(data.dpl0_pp(2))); % CURVE0_0
DwPhyLab_WriteRegister(32768 - 128 + 206,floor(data.dpl0_pp(3))); % CURVE0_1

for ii=4:7
    DwPhyLab_WriteRegister(32768 - 128 + 203 + ii,floor(data.dpl0_pp(ii)/2)); % CURVE0_ii (MSB)
    DwPhyLab_WriteRegister(32768 - 128 + 220, ii, bitand(floor(data.dpl0_pp(ii)),1)); % CURVE0_ii (LSB)
end

DwPhyLab_WriteRegister(32768 - 128 + 211,floor(data.dpl1_pp(1))); % CURVE1_0
DwPhyLab_WriteRegister(32768 - 128 + 212,floor(data.dpl1_pp(2))); % CURVE1_1
for ii=3:6
    DwPhyLab_WriteRegister(32768 - 128 + 210 + ii,floor(data.dpl1_pp(ii)/2)); % CURVE1_ii (MSB)
    DwPhyLab_WriteRegister(32768 - 128 + 220, ii-3, bitand(floor(data.dpl1_pp(ii)),1)); % CURVE1_ii (LSB)
end

% restoring the previously fetched maximum power level
DwPhyLab_WriteRegister(32768 - 128 + 219, regular_max_power);

if RadioIsRF22B()
    DwPhyLab_WriteRegister(32768 - 128 + 182, '7:7',Reg182);
    DwPhyLab_WriteRegister(256 + 80, '1:1',Reg80);
end


% Save to user space (Can use safed data from previous parts
reg2save = 32768 - 128 + [196:200 205:216 220];
for ii=1:length(reg2save)
    pwr_reg.Addr(ii) =  reg2save(ii);
    pwr_reg.Data(ii) = DwPhyLab_ReadRegister(reg2save(ii));
    data.CalParam(1, ii) = pwr_reg.Addr(ii);
    data.CalParam(2, ii) = pwr_reg.Data(ii);
end

% $$$ save pwr_reg.mat pwr_reg
DwPhyLab_SendCommand(PSA,':SYST:PRES');
DwPhyLab_SetVisaTimeout(PSA, Timeout);
DwPhyLab_ClosePSA(PSA);
data.Runtime = 24*3600*(now - datenum(data.Timestamp));

%% REVISION HISTORY



function Value = ConvertVgaIndex2Value(indx)

VGATAble = hex2dec([
    '11',
    '21',
    '41',
    '81',
    '12',
    '22',
    '42',
    '82',
    '14',
    '24',
    '44',
    '84',
    '18',
    '28',
    '48',
    '88'
    ]); %#ok<COMNL>

Value = VGATAble(indx+1);

function p = GetTxPower(PSA, Ploss)

Mpts = 1;

p = zeros(1,Mpts);

pause(1); % allow time for PSA to settle
for j=1:Mpts,
    result = DwPhyLab_SendQuery(PSA,':FETCH:CHPOWER:CHPOWER?');
    p(j) = str2double(result) + Ploss;
end


function dpl = get_dpl_out()


% for ii=1:20
%     DwPhyLab_TxPacket(Mbps, LengthPSDU, TXPWRLVL, PacketType);
%     dpl_tmp(ii) = DwPhyLab_ReadRegister(32768 - 128 + 171)*256+DwPhyLab_ReadRegister(32768 - 128 + 170);
% end
% dpl = floor(median(dpl_tmp));
dpl = DwPhyLab_ReadRegister(32768 - 128 + 171)*256+DwPhyLab_ReadRegister(32768 - 128 + 170);


function outvec = post_process(Pmeas, dpl, pp_values, interp_switch, NP,VERBOSE)

% Pmeas = Pmeas';
% dpl = dpl';

[Pmeas, ind] = sort(Pmeas);
dpl = dpl(ind);

% Remove non monotinic measurments
dpl_diff = diff(dpl);
inddif = [1 find(dpl_diff>0)+1];
tmpdpl = dpl(inddif);
tmpPmeas = Pmeas(inddif);

% [tmpPmeas, ind] = sort(tmpPmeas);
% tmpdpl = tmpdpl(ind);

switch interp_switch
    case 1,
        outvec = interp1(tmpPmeas(1:end),tmpdpl(1:end),pp_values,'spline');
        % outvec(outvec<0) = 511;
    case 2,
        outvec = interp1(tmpPmeas,tmpdpl,pp_values,'pchip');
        outvec(outvec<0) = 0;
    case 3,
        outvec = interp1(tmpPmeas,tmpdpl,pp_values,'linear');
        for ii=1:length(outvec)
            if isnan(outvec(ii))
                outvec(ii) = 511;
            end
        end
    case 4,
        P = polyfit(tmpPmeas(1:end), tmpdpl(1:end), NP);
        outvec = polyval(P, pp_values);
        outvec(outvec<0) = 511;
    case 5,
        rgfn = pp_values(pp_values>tmpPmeas(1) & (pp_values<tmpPmeas(end)));
        rgfs = pp_values(pp_values<tmpPmeas(1));
        rgfg = pp_values(pp_values>tmpPmeas(end));

        ps = polyfit(tmpPmeas(1:2), tmpdpl(1:2), 1);
        pg = polyfit(tmpPmeas(end-1:end), tmpdpl(end-1:end), 1);

        vecn = interp1(tmpPmeas, tmpdpl, rgfn, 'pchip');
        vecs = polyval(ps, rgfs);
        vecg = polyval(pg, rgfg);

        outvec = [vecs vecn vecg];
end

if outvec(1)<0
    outvec(1) = 0;
end

if VERBOSE
    if isempty(outvec(outvec<0))==0
        fprintf('Negetive values were cliped by post processing\n');
    end
    if isempty(outvec(outvec>511))==0
        fprintf('High values were cliped by post processing\n');
    end
end    
outvec(outvec<0) = 0;
outvec(outvec>511) = 511;

% force monotinicity on curves
for ii=2:length(outvec)
    if outvec(ii)<= outvec(ii-1)
        outvec(ii)= outvec(ii-1) + 1;
    end
end

function y = RadioIsRF22B()

y = any(DwPhyLab_ReadRegister(256+1) == [69 70 71 72]);

