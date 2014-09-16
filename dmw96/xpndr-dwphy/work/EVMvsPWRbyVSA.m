starttime=tic;
%%
cd C:\Users\RFIC\RadLab
RadLab_pathdef
% Set RadLab
RadLab_Setup_GeneralInfo
RadLab_Setup_TestEquip
RadLab_RF22_PowerUp
RadLab_RF22_Board_Reset
% Set Ant 2
RadLab_RFSW_State('TX',1);
RadLab_Set_SW1(1);
%%
DwPhyLab_Setup;
DwPhyTest_RunTestCommon;
DwPhyLab_Wake
% PA calibration 
data_cal = DwPhyTest_RunTest('307.1',sprintf('VERBOSE = 1,UserReg = {{32768 - 128 + 219,18*2}}'))
plot_dwpd
% VSA configuration 
MOD = 'OFDM';
VSA_init();
VSA_SetPilotTrackTiming();
RFCH_vec=[1:2:11];%[1 7 13];%[13:-2:1];%[1 7];%[1:1:14];%
FREQ_vec=[2412:10:2462]*1e6;%[2412e6 2442e6 2472e6];%[2472:-10:2412]*1e6;%[2412e6 2442e6];%[[2412:5:2472] 2484]*1e6;%[2412e6 2442e6 2484e6];% [2412 2462]*1e6;
freqsmhz=(FREQ_vec/1e6)';
freqstrn=[];
for fidx=1:length(freqsmhz),freqstrn=[num2str(freqsmhz(fidx));freqstrn];end; 
PWRLVL=33:2:63;%23:10:53;%23:2:53 %PWRLVL=29:2:63;%23:10:53;%23:2:53
MAXPOW=21;
RF22_WriteRegister(219,2*MAXPOW)

clear EVMdB_vec;
clear PWRmeas_vec;
clear PWRreq_vec;

for freq_idx=1:length(FREQ_vec),
    freqnow=FREQ_vec(freq_idx);
    chnow=RFCH_vec(freq_idx);
    VSA_set_freq(freqnow);
%----------    RadLab_Set_Channel(chnow);
    DwPhyLab_TxBurst(0); pause(0.2); % stop any on-going burst transmissions
    DwPhyLab_SetChannelFreq(freqsmhz(freq_idx));
    DwPhyLab_Wake;    
    RF22_ReadRegister(219)% TEST

    if strcmp(MOD,'OFDM')
        Mbps = 6;
        LengthPSDU = 1500;
        PacketType = 0;        
    else
        Mbps = 1;
        LengthPSDU = 1500;
        PacketType = 0;           
    end
    % Initial AGC
    % In case updates only occur on TXPWRLVL = 63, send enough packets to allow the
    % loop to settle.
    DwPhyLab_TxBurst( 1e9, 6, 150, 63, PacketType);  pause(1);
  
%    VSA_set_rate(Mbps);
    pause(1);
%-----------    IQ_LOFT_CAL();
    for idx=1:length(PWRLVL),
        pwrnow=PWRLVL(idx);
        pwrreq=MAXPOW-(63-pwrnow)/2;
%----        RF22_Set_Power(pwrnow);
        DwPhyLab_TxBurst( 0 ); pause(0.3);
        DwPhyLab_TxBurst(15000, Mbps, LengthPSDU, pwrnow, PacketType); 
        pause(0.2); % allow time for AGC to settle
        VSA_AutoRange();
        pause(0.5);
        msr_tmp = VSA_read_data()
        EVMdB_vec(freq_idx,idx)=msr_tmp.EVMdB;
        PWRmeas_vec(freq_idx,idx)=msr_tmp.pwr;
        PWRreq_vec(freq_idx,idx)=pwrreq;
        %plot(PWRmeas_vec',EVMdB_vec');grid on;hold on;drawnow
    end
%    DwPhyLab_TxBurst(0); % stop any on-going burst transmissions
    %plot(PWRmeas_vec',EVMdB_vec');grid on;hold on;drawnow 
end

% figure;plot(PWRmeas_vec,EVMdB_vec,'r*-','LineWidth',2);grid;
% figure;
% 
%RF22_Set_Power(43);
%save YDB40_3CH_NEWSETTINGS_2111218_01
%save YDB40_3CH_OLDSETTINGS_2111218_01
%---------RadLab_Set_Standby
Time_Min=toc(starttime)/60

figure;
plot(4:1:24,-25*ones(1,length(4:1:24)),'b','LineWidth',1.5); hold on;
plot(PWRmeas_vec',EVMdB_vec','*-');grid;
title('EVM vs POUT')
ylabel('EVM [dB]')
xlabel('POUT [dBm]')
legend(freqstrn,'Location','NorthWest');
%% PLOTTING 
if 0
cd C:\Users\RFIC\RadLab
clear all;
load YDB39_151211.mat
figure;
plot(4:1:24,-25*ones(1,length(4:1:24)),'b','LineWidth',1.5); hold on;
plot(PWRmeas_vec',EVMdB_vec','r*-'); hold on;
title('EVM vs POUT YDB39  ')
ylabel('EVM [dB]')
xlabel('POUT [dBm]')
clear all;
load YDB39_120112.mat
plot(PWRmeas_vec',EVMdB_vec','g*-');grid;
axis([5 25 -32 -5]);
%%
clear all
figure;
%load YDB11_151211_1.mat
load YDB11_151211.mat
plot(4:1:24,-25*ones(1,length(4:1:24)),'b','LineWidth',1.5); hold on;
plot(PWRmeas_vec',EVMdB_vec','r*-'); hold on;
title('EVM vs POUT YDB11  ')
ylabel('EVM [dB]')
xlabel('POUT [dBm]')
clear all;
load YDB11_120112.mat
plot(PWRmeas_vec',EVMdB_vec','g*-');grid;
axis([5 25 -32 -5]);
end
% subplot(2,1,1);plot(PWRreq_vec,PWRmeas_vec,'b*-','LineWidth',2);grid;
% subplot(2,1,2);plot(PWRreq_vec,abs(PWRreq_vec-PWRmeas_vec),'m*-','LineWidth',2);grid;


% count2range = 0;
% reg1_range = 0:1:3;
% reg2_range = 0:1:3;
% reg3_range = 0:1:3;
% reg4_range = 0:1:3;
% indx1 = 1;
% for reg1 = reg1_range
%     indx2 = 1;
%     for reg2 = reg2_range
%         indx3 = 1;
%         for reg3 = reg3_range %0:4:15
%             indx4 =1;
%             for reg4 = reg3_range % 0:4:15
% %                w96io('PA1CURTRIM_EVM',reg1);
% %                w96io('PA2CURTRIM_EVM',reg2);
% %                 w96io('VG22_EVM',reg3);
% %                 w96io('VG12_EVM',reg4);
% 
% 
%                 w96io('TXMXOUTTRIMGN',reg1);%    2 bit
%                 w96io('TXMXOUTTRIMFR',reg2);%    2 bit   
%                 w96io('TXMXGMTRIM',reg3);%    2 bit                   
%                 w96io('PLLX3TRIM',reg4);%    2 bit                   
%                 
%                 IQ_LOFT_CAL();
%                 
%                 RF22_Set_Power(53);
%                 pause(1);
%                 VSA_AutoRange();
% %                 VSA_set_range_fast();
%                 msr_tmp = VSA_read_data();
%                 
%                 if abs(msr_tmp.pwr-18)>2
%                     RF22_Set_Power(53);
%                     pause(1);
%                     msr_tmp = VSA_read_data();
%                 end
% 
%                 EVMdB(indx1,indx2,indx3) = msr_tmp.EVMdB;
%                 pwr(indx1,indx2,indx3) = msr_tmp.pwr;
% 
% %                fprintf('TXMXOUTTRIMGN  = %d, TXMXOUTTRIMFR = %d,TXMXGMTRIM = %d   Power : %f, EVM : %f\n',reg1,reg2,reg3,msr_tmp.pwr,msr_tmp.EVMdB);
%                 fprintf('TXMXOUTTRIMGN  = %d, TXMXOUTTRIMFR = %d,TXMXGMTRIM = %d   ,PLLX3TRIM = %d  Power : %f, EVM : %f\n',reg1,reg2,reg3,reg4,msr_tmp.pwr,msr_tmp.EVMdB);
%                indx4 = indx4 + 1;
%             end
%             indx3 = indx3 + 1;
%         end
%         indx2 = indx2 + 1;
% 
%     end
%     indx1 = indx1 + 1;
% end
% 
% pwr2 = squeeze(pwr);
% EVMdB2 = squeeze(EVMdB);
% 
% Met = EVMdB2 - 2*pwr2;
% contourf(reg1_range,reg2_range,Met, 'DisplayName', 'EVMdB2', 'ZDataSource', 'EVMdB2'); colorbar
% ylabel('PA1'); xlabel('PA2');
% figure(gcf)
% 
% 
% % FOR PLOTTING REFERENCE
%---------------------------
% figure(gcf); clf;
% DwPhyPlot_Command(N, 'plot','x{%d}','y{%d}'); grid on;
% P = get(gcf,'Position'); set(gcf,'Position',[P(1) P(2) 620 360]);
% c = get(gca,'Children'); set(c,'LineWidth',2); %set(c,'MarkerSize',14);
% hold on;
% plot(x{1},X{1}.(fieldname).SpecData,'r','LineWidth',2); hold off;
% axis([-50 50 -60 1]);
% set(gca,'XTick',-50:10:50e6);
% set(gca,'YTick',-60:10:0);
% 
% Pout = (X{1}.(fieldname).pwr_mapd_max - (63 - TXPWRLVL))/2;
% ylabel(sprintf('PSD (dBr) @ Pout = %d dBm',Pout)); xlabel(sprintf('Frequency Offset (MHz) at Channel %d',Channel));

