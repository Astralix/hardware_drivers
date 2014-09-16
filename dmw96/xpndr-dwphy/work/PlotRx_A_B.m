clear all
load T:\Ruslan\Perfomance\Rx\YDB11-FullRxEval22-111211-204432.mat 
data_YDB11 = data;
save  data_YDB11
clear data 
load T:\Ruslan\Perfomance\Rx\YCA02-FullRxEval22-111212-165937.mat  
data_YCA02 = data;
save data_YCA02
clear data
%%	Maximum Output Power   
%   1.1	Maximum Output  
%   1.4 Power OFDM Constellation  Error 
%   1.6 EVM Sensitivity to Data rate 
DwPhyPlot_RxEval({'data_YCA02','data_YDB11'})
% %%
% %%  Output Power Back-Off 
% % 1.2     201.1
% DwPhyPlot_TxPower({'data_YCA02','data_YDB11'})
% %%
% %%  Transmit Spectrum Mask
% %%
% DwPhyPlot_TxMask({'data_YCA02','data_YDB11'})
% %%
% DwPhyPlot_TxMask(data_YCA02)
% %%
% DwPhyPlot_TxMask(data_YDB02)
% %% Phase Noise
% DwPhyPlot_TxPhaseNoise({'data_YCA02','data_YDB11'})
% %% TxEVMvPADGAIN
% DwPhyPlot_TxEVMvPADGAIN(data_YCA02)
% %%
% DwPhyPlot_TxEVMvPADGAIN(data_YDB11)
