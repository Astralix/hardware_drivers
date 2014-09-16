
%%
data_cal = DwPhyTest_RunTest('307.1',sprintf('VERBOSE = 1,UserReg = {{32768 - 128 + 219,18*2}}'))


plot_dwpd
%%
% DwPhyLab_WriteRegister(32768 - 128 + 185,'7:0',300/2); % addr>=128
%data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 219,23*2},{32768 - 128 + 186,150}}'))
%data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 219,23*2},{32768 - 128 + 186,150}}'))
data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 219,23*2}}'))
%plot_dwpm
plot_dwpm_error

% DwPhyLab_WriteRegister(32768 - 128 + 187,'5:0',Normal_state_attn); % addr>=128
% DwPhyLab_WriteRegister(256 + 21,55); % addr<128
%%
load YCA11_FIXSTATION_EVMvsPOUT.mat 
data_YCA11_REF = data_evm;
%%
data_evm = DwPhyTest_RunTest('204.1',sprintf('UserReg = {{32768 - 128 + 219,18*2},{32768 - 128 + 217,12}}'))


figure;
plot(data_evm.Pout_dBm,data_evm.AvgEVMdB,'*-');%,data_YCA11_REF.Pout_dBm,data_YCA11_REF.AvgEVMdB,'*-'); grid on;
xlabel('Pout (dbm)');
Ylabel('EVM (dbm) 2412 Mhz');
axis([2 20 -31 -22]);
grid on;
%legend('YDB11','YCA11');
title('YDB11 EVMvsPOUT DwPhy:Release');

	
% 
% %% 
% data_mask = DwPhyTest_RunTest('202.2',sprintf('UserReg = {{32768 - 128 + 184,2,1},{32768 - 128 + 219,18*2}},fcMHz = 2442'))

%%
data_mask = DwPhyTest_RunTest('202.2',sprintf('UserReg = {{32768 - 128 + 219,21*2}}'))
data.PartID = 'Y3_1';
data.Test202_2 = data_mask;
figure;
DwPhyPlot_TxMask(data);

%% upper curve

data_cal = DwPhyTest_RunTest('307.2',sprintf('VERBOSE = 1,UserReg = {{32768 - 128 + 184,2,1},{32768 - 128 + 219,18*2}}'))


plot_dwpd
data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 184,2,1},{32768 - 128 + 219,23*2}}'))
plot_dwpm

%%

data_cal = DwPhyTest_RunTest('307.2',sprintf('VERBOSE = 1,UserReg = {{32768 - 128 + 219,18*2}}'))


plot_dwpd

% DwPhyLab_WriteRegister(32768 - 128 + 185,'7:0',300/2); % addr>=128
data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 219,23*2}}'))
plot_dwpm

% DwPhyLab_WriteRegister(32768 - 128 + 187,'5:0',Normal_state_attn); % addr>=128
% DwPhyLab_WriteRegister(256 + 21,55); % addr<128

data_evm = DwPhyTest_RunTest('204.1',sprintf('UserReg = {{32768 - 128 + 219,23*2}}'))

figure;
plot(data_evm.Pout_dBm,data_evm.AvgEVMdB); grid on;



%% antenna symetry

clear DwPhyMex

data_cal = DwPhyTest_RunTest('307.2');

plot_dwpd

data_pout_ant1 = DwPhyTest_RunTest('201.1',sprintf('DiversityMode = 1,UserReg = {{32768 - 128 + 219,23*2}}'));
data_pout_ant2 = DwPhyTest_RunTest('201.1',sprintf('DiversityMode = 2,UserReg = {{32768 - 128 + 219,23*2}}'));

figure;
pmax = DwPhyLab_ReadRegister(32768 - 128 + 219);
plot((pmax - (63-data_pout_ant1.TXPWRLVL))/2, data_pout_ant1.Pout_dBm, '.-');
hold on; grid on;
plot((pmax - (63-data_pout_ant2.TXPWRLVL))/2, data_pout_ant2.Pout_dBm, 'c.-');
legend('DiversityMode = 1','DiversityMode = 2');