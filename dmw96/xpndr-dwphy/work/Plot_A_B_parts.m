% data = DwPhyTest_RunSuite('FullTxEval96-2','YDB11',1);
% data = DwPhyTest_RunSuite('FullTxEval96-2','YCA02',1);
clear all
% load T:\Ruslan\Perfomance\YDB11-FullTxEval96-REFOSCTUNE_70_OPT\YDB11-FullTxEval96_307_200_1_2_3_4_5
% data_YDB11_307_200_1_2_3_4_5 = data;
% clear data;
% load T:\Ruslan\Perfomance\YDB11-FullTxEval96-REFOSCTUNE_70_OPT\YDB11-FullTxEval96_206_7_8_10_11
% clear data;
load T:\Ruslan\Perfomance\YCA02-FullTxEval96-REFOSCTUNE_53_OPT\YCA02-FullTxEval96_307_200_1_2_3_4_5
data_YCA02_307_200_1_2_3_4_5 = data;
clear data; 
load T:\Ruslan\Perfomance\YCA02-FullTxEval96-REFOSCTUNE_53_OPT\YCA02-FullTxEval96_206_7_8
data_YCA02_206_7_8 = data;
clear data; 
load T:\Ruslan\Perfomance\YCA02-FullTxEval96-REFOSCTUNE_53_OPT\YCA02-FullTxEval96_210_11
data_YCA02_210_11 = data;
clear data; 
load T:\Ruslan\Perfomance\YDB11-FullTxEval96-REFOSCTUNE_70_OPT_POWER_16\YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat 
data_YDB02 = data;
clear data; 
%%	Maximum Output Power   
%   1.1	Maximum Output  
%   1.4 Power OFDM Constellation  Error 
DwPhyPlot_TxEval({'YCA02-FullTxEval96_307_200_1_2_3_4_5.mat','YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat'})
% DwPhyPlot_TxEval({'YDB11-FullTxEval96_307_200_1_2_3_4_5.mat','YCA02-FullTxEval96_307_200_1_2_3_4_5.mat'})
%%
%DwPhyPlot_TxEval(data_YCA02_307_200_1_2_3_4_5)
%%
%DwPhyPlot_TxEval(data_YDB11_307_200_1_2_3_4_5)
%% Output Power Back-Off 
% 1.2     201.1
% DwPhyPlot_TxPower({'YDB11-FullTxEval96_07_200_1_2_3_4_5.mat','YCA02-FullTxEval96_307_200_1_2_3_4_5.mat'})
DwPhyPlot_TxPower({'YCA02-FullTxEval96_307_200_1_2_3_4_5.mat','YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat'})
%%
%DwPhyPlot_TxPower(data_YCA02_307_200_1_2_3_4_5)
%%
%DwPhyPlot_TxPower(data_YDB11_307_200_1_2_3_4_5)
%%  Transmit Spectrum Mask
%%
% DwPhyPlot_TxMask({'YCA02-FullTxEval96_307_200_1_2_3_4_5.mat','YDB11-FullTxEval96_307_200_1_2_3_4_5'})
DwPhyPlot_TxMask({'YCA02-FullTxEval96_307_200_1_2_3_4_5.mat','YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat'})
%DwPhyPlot_TxMask(data_YCA02_307_200_1_2_3_4_5)
%%DwPhyPlot_TxMask(data_YCA02_307_200_1_2_3_4_5)
%%
%DwPhyPlot_TxMask(data_YDB11_307_200_1_2_3_4_5)
%%
DwPhyPlot_TxMask(data_YCA02_210_11)
%%
%DwPhyPlot_TxMask(data_YDB11_206_7_8_10_11)
DwPhyPlot_TxMask(data_YDB02)
%%
DwPhyPlot_TxMask(data_YCA02_307_200_1_2_3_4_5)
%%
%DwPhyPlot_TxMask(data_YDB11_307_200_1_2_3_4_5)
DwPhyPlot_TxMask(data_YDB02)
%% 1.6 EVM Sensitivity to Data rate 
%DwPhyPlot_TxEval({'YCA02-FullTxEval96_206_7_8.mat','YDB11-FullTxEval96_206_7_8_10_11.mat'})
DwPhyPlot_TxEval({'YCA02-FullTxEval96_206_7_8.mat','YDB11-FullTxEval96_206_7_8_10_11.mat'})

%% Phase Noise
DwPhyPlot_TxPhaseNoise({'YCA02-FullTxEval96_307_200_1_2_3_4_5.mat','YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat'})
%DwPhyPlot_TxPhaseNoise(data_YCA02_307_200_1_2_3_4_5)
%legend(data_YCA02_307_200_1_2_3_4_5.PartID)
%%
%DwPhyPlot_TxPhaseNoise(data_YDB11_307_200_1_2_3_4_5)
%legend(data_YDB11_307_200_1_2_3_4_5.PartID)
%% TxEVMvPADGAIN
%%
DwPhyPlot_TxEVMvPADGAIN(data_YCA02_210_11)
%%
DwPhyPlot_TxEVMvPADGAIN(data_YDB11)
%DwPhyPlot_TxEVMvPADGAIN(data_YDB11_206_7_8_10_11)
% for i=1:1,
%         x{i} = data.LengthPSDU;
%         y{i} = data.AvgEVMdB;
%     end