% % w96io('PA2CURTRIM',17);
% % w96io('PA1CURTRIM',27);
% % w96io('VG22',9); % 14
% % w96io('VG12',13); % 11
% % w96io('DR1CURTRIM',3);
% % w96io('DR2CURTRIM',1);
% 
% %
% % w96io('DR1SYM','3');
% % w96io('DR2SYM','0');
% %
% % w96io('TXMXOUTTRIMGN','3');    2 bit
% % w96io('TXMXOUTTRIMFR','0');    2 bit   
% %  TXMXGMTRIM                    2 bit




%MOD = 'OFDM';
%E4438C_Init();



%VSA_set_freq(2442e6);
%RadLab_Set_Channel(7);

% if strcmp(MOD,'OFDM')
%     VSA_set_rate(6);
%     RadLab_RF22_LoadPacket(6,1500);
% else
%     VSA_set_rate(1);
%     RadLab_RF22_LoadPacket(1,1500);
% end
% 
% clear EVMdB;
% clear pwr;
% 
% 
% RF22_Set_Power(53);
% pause(1);

count2range = 0;
reg1_range = 0:1:2;%0:1:31;
reg2_range = 0:1:2;%0:1:31;
%EVM_Me

indx1 = 1;
for reg1 = reg1_range
    indx2 = 1;
    for reg2 = reg2_range
        indx3 = 1;
        for reg3 = 1 %0:4:15
            indx4 =1;
            for reg4 = 1 % 0:4:15
%                DwPhyLab_TxBurst( 1e9, 6, 1500, 33, 1);
            data = DwPhyTest_TxEVMvPout('fcMHz = 2412', 'TXPWRLVL = 33', ...
                'MeasurePower = 1','Npts=20', 'SetRxSensitivity = -60');%, 'UserReg = {{32768 - 128 + 219,23*2}}');
%            EVM_Me(i)=data.
%             data = DwPhyTest_TxEVMvPout('fcMHz = 2412', ListTXPWRLVL16s, ...
%                 'MeasurePower = 1','Npts=20', SetRxSensitivityString, opt);
%                 w96io('PA1CURTRIM_EVM',reg1);
%                 w96io('PA2CURTRIM_EVM',reg2);
% %                 w96io('VG22_EVM',reg3);
% %                 w96io('VG12_EVM',reg4);
% 
%                 IQ_LOFT_CAL();
%                 
%                 RF22_Set_Power(53);
%                 pause(1);
% 
%                 VSA_set_range_fast();
%                 msr_tmp = VSA_read_data();
%                 
%                 if abs(msr_tmp.pwr-18)>2
%                     RF22_Set_Power(53);
                     pause(5);
%                     msr_tmp = VSA_read_data();
%                 end
% 
                 EVMdB(indx1,indx2,indx3,indx4) = data.AvgEVMdB;
%                 pwr(indx1,indx2,indx3,indx4) = msr_tmp.pwr;
% 
                 fprintf('EVM : %f\n',data.AvgEVMdB);
                indx4 = indx4 + 1;
            end
            indx3 = indx3 + 1;
        end
        indx2 = indx2 + 1;

    end
    indx1 = indx1 + 1;
end

%pwr2 = squeeze(pwr);
EVMdB2 = squeeze(EVMdB);

%Met = EVMdB2 - 2*pwr2;
contourf(reg1_range,reg2_range,EVMdB2, 'DisplayName', 'EVMdB2', 'ZDataSource', 'EVMdB2'); colorbar
ylabel('PA1'); xlabel('PA2');
figure(gcf)



