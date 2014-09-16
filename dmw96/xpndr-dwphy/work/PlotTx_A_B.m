% data = DwPhyTest_RunSuite('FullTxEval96-2','YDB11',1);
% data = DwPhyTest_RunSuite('FullTxEval96-2','YCA02',1);
%data_Tx_YDB11_DwPhy_110112 = DwPhyTest_RunSuite('FullTxEval96-2','YDB11 110112',1,0,sprintf('UserReg = {{32768 - 128 + 219,16*2}}'))
data_Tx_YDB38_DwPhy_110112 = DwPhyTest_RunSuite('FullTxEval96-2','YDB38 110112',1,0,sprintf('UserReg = {{32768 - 128 + 219,16*2}}'))
%%
clear all
load T:\Ruslan\Perfomance\YDB11-FullTxEval96-REFOSCTUNE_70_OPT_POWER_16\YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat 
data_YDB11 = data;
save  data_YDB11
clear data 
load T:\Ruslan\Perfomance\YCA02-FullTxEval96-REFOSCTUNE_53_OPT\YCA02-FullTxEval96-REFOSCTUNE_53_OPT.mat 
data_YCA02 = data;
save data_YCA02
clear data
%%
FileList_A_B = {...           
                    'YCA02-FullTxEval96-REFOSCTUNE_53_OPT.mat ',...            
                    'YDB11_FullTxEval96_YDB11_FullTxEval96_REFOSCTUNE_70_OPT_POWER_16.mat',...  
                    'YDB11 110112-FullTxEval96-120117-200236.mat',...
                    %'YDB11-FullTxEval96-120115-191850.mat',...  
                  };
%FileList = { FileList_35_4Plote(:),  FileList_35_HIPOLEENSR_4Plote(:)};                          
%FileList = { FileList_35_4Plote(:),  'qqqqqqqq'};                          
FileList = FileList_A_B;

%%	Maximum Output Power   
%   1.1	Maximum Output  
%   1.4 Power OFDM Constellation  Error 
%   1.6 EVM Sensitivity to Data rate 
%DwPhyPlot_TxEval({'data_YCA02','data_YDB11'})
DwPhyPlot_TxEval(FileList_A_B)
%%
%%  Output Power Back-Off 
% 1.2     201.1
%DwPhyPlot_TxPower({'data_YCA02','data_YDB11'})
DwPhyPlot_TxPower(FileList_A_B)
%%
%%  Transmit Spectrum Mask
%%
%DwPhyPlot_TxMask({'data_YCA02','data_YDB11'})
DwPhyPlot_TxMask(FileList_A_B)
%%
%DwPhyPlot_TxMask(data_YCA02)
%%
%DwPhyPlot_TxMask(data_YDB02)
%% Phase Noise
%DwPhyPlot_TxPhaseNoise({'data_YCA02','data_YDB11'})
DwPhyPlot_TxPhaseNoise(FileList_A_B)
%% TxEVMvPADGAIN
DwPhyPlot_TxEVMvPADGAIN(data_YCA02)
%%
DwPhyPlot_TxEVMvPADGAIN(data_YDB11)
%DwPhyPlot_TxEVMvPADGAIN(data_Tx_YDB11_DwPhy_110112)
%%
%% FAIL ANALIZE  
TestResultFail = find( data.TestResult == 0);
Fail_TestList =  data.TestList(TestResultFail)
