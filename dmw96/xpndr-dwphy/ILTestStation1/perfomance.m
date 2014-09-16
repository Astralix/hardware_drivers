
%%
%*************** P_OUT vs TXPWRLVL        2         ***********************************************************
data_cal = DwPhyTest_RunTest('307.1',sprintf('VERBOSE = 1,UserReg = {{32768 - 128 + 219,18*2}}'))
plot_dwpd
data_pout = DwPhyTest_RunTest('201.1',sprintf('UserReg = {{32768 - 128 + 219,23*2}}'))
plot_dwpm_error
%%
%*************** EVM vs TXPWRLVL            4          ***********************************************************
data_evm = DwPhyTest_RunTest('204.1',sprintf('UserReg = {{32768 - 128 + 219,23*2},{32768 - 128 + 217,12}}'))
figure;
plot(data_evm.Pout_dBm,data_evm.AvgEVMdB); grid on;
%%   202.1   L-OFDM TxMask at 2412 MHz         3
data_mask = DwPhyTest_RunTest('202.1',sprintf('UserReg = {{32768 - 128 + 219,21*2}}'))
data.PartID = 'YCA02';
data.Test202_1 = data_mask;
figure;
DwPhyPlot_TxMask(data);
%%  202.2   CCK TxMask at 2412 MHz              3
data_mask = DwPhyTest_RunTest('202.2',sprintf('UserReg = {{32768 - 128 + 219,21*2}}'))
data.PartID = 'YCA02';
data.Test202_2 = data_mask;
figure;
DwPhyPlot_TxMask(data);
%%  202.4   HT-OFDM TxMask at 2412 MHz         3
data.MaxPower = 16;
RF22_WriteRegister(219,data.MaxPower*2);
data_mask = DwPhyTest_RunTest('202.4');%,sprintf(['UserReg = {{32768 - 128 + 219,16*2}}'))
data.PartID = 'YCA02';
data.Test202_4 = data_mask;
figure;
DwPhyPlot_TxMask(data);
title(['Test:202.4, HT-OFDM, TxMask at 2412MHz, MaxPower = ',num2str(data.MaxPower)]);
%%
%*************** PhaseNoise vs TXPWRLVL       7          ***********************************************************
data_Noise = DwPhyTest_TxPhaseNoise
DwPhyPlot_TxPhaseNoise(data_Noise)
%%
%*************** Max Power vs chennals       1          ***********************************************************
data_MaxP = DwPhyTest_RunTest('200.1')


