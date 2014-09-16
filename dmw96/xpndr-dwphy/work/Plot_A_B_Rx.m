% DwPhyTest_RunSuite('FullRxEval22','YCA02',1,0,sprintf('UserReg = {{65,34}}'));
% DwPhyTest_RunSuite('FullRxEval22','YDB11',1,0,sprintf('UserReg = {{65,35}}'));
% data_RxTiming_YDB11 = DwPhyTest_RunSuite('RxTiming','YDB11',1)
% data_RxTiming_YCA02 = DwPhyTest_RunSuite('RxTiming','YCA02',1)
data_RxTiming_YCA02_DBG = DwPhyTest_RunSuite('RxTiming','YCA02_DBG',1,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}}'))
data_RxTiming_YDB11_DBG = DwPhyTest_RunSuite('RxTiming','YDB11_DBG',1,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}}'));
data_RxTiming_YDB11_IAGAIN_35 = DwPhyTest_RunSuite('RxTiming','YDB11_IAGAIN_35',1,0,sprintf('UserReg = {{65,35}}'))
DwPhyPlot_RxTiming(data_RxTiming_YCA02)
DwPhyPlot_RxTiming(data_RxTiming_YDB11)
clear all
load YDB11-FullRxEval22 _IAGAIN_34-111227-143448.mat
data_Rx_YDB11 = data;
save  data_Rx_YDB11
clear data 
load YCA02-FullRxEval22 _IAGAIN_35-111227-234807.mat 
data_Rx_YCA02 = data;
save data_Rx_YCA02
clear data
%% PRINT   
%   DwPhyPlot_Blocking              - Plot result from DwPhyTest_Blocking
%+   DwPhyPlot_InterferenceRejection - Plot result from DwPhyTest_InterferenceRejection
%   DwPhyPlot_Kvco                  - Plot results from DwPhyTest_Kvco
%   DwPhyPlot_PERvPrdBm             - Plot result from DwPhyTest_PERvPrdBm
%+   DwPhyPlot_RxEval22                - Plot results from DwPhyTest_RunSuite('RxEval22')
%+   DwPhyPlot_RxGain                - Plot results from DwPhyTest_RunTest(300.2)
%+   DwPhyPlot_Sensitivity           - Plot results from Sensitivity (100.X)
%OK 'YDB11_IAGAIN_34_110112-FullRxEval22-120112-225801.mat',... 
%'YDB11-FullRxEval22 _IAGAIN_35-111227-143448.mat',...    
%OK 'YDB11_IAGAIN_35_DBG-FullRxEval22-111230-012109.mat',...    
FileList_B11  = {'YDB11_IAGAIN_34_110112-FullRxEval22-120112-225801.mat',... 
                 'YDB11_IAGAIN_35_DBG-FullRxEval22-120102-035101.mat',...    
                };
FileList_Rx_Timing4Plote  = {'YDB06_IAGAIN_35-RxTiming24-120105-134356.mat',...  
    'YDB06_IAGAIN_35_DBG-RxTiming24-120105-142332.mat',...
    'YDB11_IAGAIN_35-RxTiming24-111229-180648.mat',...
    'YDB11-RxTiming24-111229-113019.mat',...
    'YDB38-RxTiming24-120104-162722.mat',...
    'YDB08 35-RxTiming24-120106-095211.mat',...
    'YDB08-RxTiming24-120106-091124.mat',...
    };


FileList_4Plote  = { 'YCA02-FullRxEval22-111227-014750.mat',...
    'YDB06-FullRxEval22-120104-230820.mat',...    
    'YDB11-FullRxEval22-120102-225217.mat',...
    'YDB38-FullRxEval22-120103-225751.mat',...    
    };

FileList_35_4Plote  = { 'YCA02-FullRxEval22 _IAGAIN_34-111227-234807.mat',...
                    'YDB11-FullRxEval22 _IAGAIN_35-111227-143448.mat',...
                    'YDB06_IAGAIN_35-FullRxEval22-120105-083153.mat',...
                    'YDB38_IAGAIN_35-FullRxEval22-120104-081105.mat',...
                    'YDB08 IAGAIN 35-FullRxEval22-120106-035100.mat',...
                    };
FileList_35_HIPOLEENSR_4Plote = {   'YCA02-FullRxEval22 _IAGAIN_34-111227-234807.mat',...
                                    'YDB11_IAGAIN_35_DBG-FullRxEval22-111230-012109.mat',...
                                    'YDB06_IAGAIN_35-FullRxEval22-120105-083153.mat',...
                                    'YDB38_IAGAIN_35_DBG-FullRxEval22-120104-033339.mat',...
                                    'YDB08 35 DBG-FullRxEval22-120106-083033.mat',...
                                };
FileList_ALL = { 'YCA02-FullRxEval22 _IAGAIN_34-111227-234807.mat',...
                    'YDB11-FullRxEval22 _IAGAIN_35-111227-143448.mat',...
                    'YDB06_IAGAIN_35-FullRxEval22-120105-083153.mat',...
                    'YDB38_IAGAIN_35-FullRxEval22-120104-081105.mat',...
                    'YDB08 IAGAIN 35-FullRxEval22-120106-035100.mat',...                            
                    'YCA02-FullRxEval22 _IAGAIN_34-111227-234807.mat',...
                    'YDB11_IAGAIN_35_DBG-FullRxEval22-111230-012109.mat',...
                    'YDB06_IAGAIN_35-FullRxEval22-120105-083153.mat',...
                    'YDB38_IAGAIN_35_DBG-FullRxEval22-120104-033339.mat',...
                    'YDB08 35 DBG-FullRxEval22-120106-083033.mat',...
                };                    
            
FileList_ProblematicTiming = {...
                    'YDB38_IAGAIN_35-FullRxEval22-120104-081105.mat',...
                    'YDB38_IAGAIN_35_DBG-FullRxEval22-120104-033339.mat',...
                    'YDB11-FullRxEval22_IAGAIN_35-111227-143448.mat',...                               
                    'YDB11_IAGAIN_35_DBG-FullRxEval22-111230-012109.mat',...
                    'YDB11-FullRxEval22-120102-225217.mat',...
                };
FileList_YDB38 = {...            
                    'YDB38_IAGAIN_35_DBG-FullRxEval22-120104-033339.mat',...            
                    'YDB38 IAGAIN 35 110112-FullRxEval22-120112-000958.mat',...            
                  };
%FileList = { FileList_35_4Plote(:),  FileList_35_HIPOLEENSR_4Plote(:)};                          
%FileList = { FileList_35_4Plote(:),  'qqqqqqqq'};                          
FileList = FileList_B11;
%% ALL
DwPhyPlot_RxEval22(FileList);
%% 300.2 Gain and StepLNA
DwPhyPlot_RxGain(FileList);
%%  2.8  RSSI
DwPhyPlot_RSSI(FileList)
%% 2.1 Sensitivity
DwPhyPlot_Sensitivity(FileList);
%% 104.1/2	Interference Rejection, 11 Mbps, 2.4 GHz Band
DwPhyPlot_InterferenceRejection(FileList);
%% 401- 403 Timing
DwPhyPlot_RxTiming(FileList);
%% PER
DwPhyPlot_PERvPrdBm(FileList)


%% FAIL ANALIZE  
TestResultFail = find( data.TestResult == 0);
Fail_TestList =  data.TestList(TestResultFail)
%% 2.1 Sensitivity
%DwPhyPlot_Sensitivity({'data_Rx_YCA02','data_Rx_YDB11'})
DwPhyPlot_Sensitivity(FileList_35_4Plote);
%%  2.8  RSSI
data_Rx_108_1_YDB8_34 = DwPhyTest_RunTest(108.1,sprintf('UserReg = {{65,34}}'));
DwPhyPlot_RSSI(data_Rx_108_1_YDB8_34)
%%
data_Rx_108_1_YDB38_30 = DwPhyTest_RunTest(108.1,sprintf('UserReg = {{65,30}}'));
DwPhyPlot_RSSI(data_Rx_108_1_YDB38_30)

% DwPhyPlot_RSSI('data_Rx_YDB11')    
% DwPhyPlot_RSSI({'YCA02-FullRxEval22 _IAGAIN_35-111227-234807.mat','YDB11-FullRxEval22 _IAGAIN_34-111227-143448.mat'})
% DwPhyPlot_RSSI('YDB38-FullRxEval22-120103-225751.mat');
% DwPhyPlot_RSSI('YDB06-FullRxEval22-120104-230820.mat');
%DwPhyPlot_RSSI(FileList_35_4Plote)
%%  PER
% DwPhyPlot_PERvPrdBm({'data_Rx_YCA02','data_Rx_YDB11'})
DwPhyPlot_PERvPrdBm({'data_Rx_YCA02','data_Rx_YDB11'})
%%	
DwPhyPlot_RxEval22({'data_Rx_YCA02','data_Rx_YDB11'})
%% 104.3	Interference Rejection, 11 Mbps, 2.4 GHz Band
data_Rx_TempTest_YDB11 = DwPhyTest_RunSuite('TempTest','data_Rx_TempTest_YDB11',1,0);
DwPhyPlot_InterferenceRejection(data_Rx_TempTest_YDB11);
%DwPhyPlot_InterferenceRejection({'data_Rx_TempTest_YDB07-TempTest-120103-164852.mat','data_Rx_TempTest_YDB11-TempTest-120103-152215.mat','YDB05-TempTest-120103-174311.mat','YDB38-TempTest-120103-182617.mat','YDB06-TempTest-120104-182733.mat'});
DwPhyPlot_InterferenceRejection({   'YDB07-TempTest-120103-164852.mat',...
                                    'YDB11-TempTest-120103-152215.mat',...
                                    'YDB05-TempTest-120103-174311.mat',...
                                    'YDB38-TempTest-120103-182617.mat',...
                                    'YDB06-TempTest-120104-182733.mat',...
                                    });
%                                    'data_Rx_TempTest_YDB11-TempTest-120208-113354.mat'});
%%
%DwPhyPlot_InterferenceRejection({'data_Rx_YCA02','data_Rx_YDB11'})
DwPhyPlot_InterferenceRejection({'YCA02-FullRxEval22 _IAGAIN_34-111227-234807.mat', 'YDB11_IAGAIN_35_DBG-FullRxEval22-120102-035101.mat'})

DwPhyPlot_RxTiming('YDB06_IAGAIN_35-RxTiming24-120105-134356.mat')

%%
for i=1:20,
    data_Rx_403_YDB38_IAGAIN_35_DBG = DwPhyTest_RunSuite('TempTest','YDB38 35 DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0},{65,35}}'));
    k = i;
%    data_Rx_403_YDB38_IAGAIN_35 = DwPhyTest_RunSuite('TempTest','YDB38 35 DBG',1,0,sprintf('UserReg = {{65,35}}'));
end
%%
%DwPhyPlot_RxEval({'data_Rx_YCA02','data_Rx_YDB11'})
%Maximum Output Power   
%   1.1	Maximum Output  
%   1.4 Power OFDM Constellation  Error 
%   1.6 EVM Sensitivity to Data rate 

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
%%TEMP SCRIPT
data_Rx_YDB11_IAGAIN_35 = DwPhyTest_RunSuite('FullRxEval22','YDB11_IAGAIN_35',1,0,sprintf('UserReg = {{65,35}}'));
data_Rx_YDB11_IAGAIN_35_DBG = DwPhyTest_RunSuite('FullRxEval22','YDB11_IAGAIN_35_DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0},{65,35}}'));
data_RxTiming_YDB11_IAGAIN_35 = DwPhyTest_RunSuite('RxTiming','YDB11_IAGAIN_35',1,0,sprintf('UserReg = {{65,35}}'));
%%
data_RxTiming_YDB06_IAGAIN_35 = DwPhyTest_RunSuite('RxTiming','YDB06_IAGAIN_35',1,0,sprintf('UserReg = {{65,35}}'));
data_RxTiming_YDB06_IAGAIN_35_DBG = DwPhyTest_RunSuite('RxTiming','YDB06_IAGAIN_35_DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0},{65,35}}'));

%%
data_Rx_100_6_YDB11_IAGAIN_35_DBG = DwPhyTest_RunSuite(100.6,'data_Rx_100_6_YDB11_IAGAIN_35_DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_Rx_100_7_YDB11_IAGAIN_35_DBG = DwPhyTest_RunSuite(100.7,'data_Rx_100_7_YDB11_IAGAIN_35_DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
%%
data_Rx_100_6_YDB11_IAGAIN_35_DBG = DwPhyTest_RunTest(100.6,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_Rx_100_7_YDB11_IAGAIN_35_DBG = DwPhyTest_RunTest(100.7,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_Rx_104_3_YDB11_IAGAIN_35_DBG_B = DwPhyTest_RunTest(104.3,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_BasicRX22_YDB11 = DwPhyTest_RunSuite('BasicRX22','BasicRX22_YDB11',1,0)
%%
data_Rx_403_1_YDB38_IAGAIN_35_DBG = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_Rx_403_1_YDB08_IAGAIN_35_DBG = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
data_Rx_403_1_YDB11_IAGAIN_35_151211 = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{65,35}}'));
data_Rx_403_1_YDB11_IAGAIN_35_120202 = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{65,35}}'));
data_Rx_403_1_YDB11_IAGAIN_35_120112 = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{65,35}}'));
%%
for i=1:10,
    data_Rx_401_5_YDB06_IAGAIN_35_DBG = DwPhyTest_RunTest(401.5,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0}},{65,35}'));
    data_Rx_401_5_YDB06_IAGAIN_35_DBG.Result
end
%%
data_10_YDB11 = DwPhyTest_RunTest(10);
%%
data_Rx_YDB11 = DwPhyTest_RunSuite('FullRxEval22','YDB11',1,0);
%%
%data_Rx_TempTest_YDB08 = DwPhyTest_RunSuite('TempTest','YDB08',1,0);
data_Rx_YDB39 = DwPhyTest_RunSuite('FullRxEval22','YDB08',1,0);
data_Rx_YDB39_IAGAIN_35 = DwPhyTest_RunSuite('FullRxEval22','YDB39 IAGAIN 35',1,0,sprintf('UserReg = {{65,35}}'));
data_Rx_YDB39_IAGAIN_35_DBG = DwPhyTest_RunSuite('FullRxEval22','YDB39 35 DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0},{65,35}}'));
data_RxTiming_YDB39 = DwPhyTest_RunSuite('RxTiming','YDB39',1,0);
%%
data_RxTiming_YDB11_IAGAIN_35_DwPhy_120202 = DwPhyTest_RunSuite('RxTiming','YDB11 35 DwPhy 120202 ',1,0,sprintf('UserReg = {{65,35}}'));
data_RxTiming_YDB11_IAGAIN_34_DwPhy_120202 = DwPhyTest_RunSuite('RxTiming','YDB11 34 DwPhy 120202 ',1,0,sprintf('UserReg = {{65,34}}'));
%%
data_RxTiming_YDB38_DwPhy_110112 = DwPhyTest_RunSuite('RxTiming','YDB38 35 DwPhy 110112 ',1,0,sprintf('UserReg = {{65,35}}'));
data_Rx_YDB38_IAGAIN_35_DwPhy_110112 = DwPhyTest_RunSuite('FullRxEval22','YDB38 IAGAIN 35 110112',1,0,sprintf('UserReg = {{65,35}}'));
%data_RxTiming_YDB39 = DwPhyTest_RunSuite('RxTiming','YDB39 35 DBG',1,0,sprintf('UserReg = {{32768 - 128 + 117,7,1},{32768 - 128 + 99,0},{65,35}}'));
%%
temp_data = DwPhyTest_RunTest(10.4,sprintf('UserReg = {{65,34}}'));
temp_data_YDB11_403_1_IAGAIN_35_120202_TMP = DwPhyTest_RunTest(403.1,sprintf('UserReg = {{65,35}}'));
%%
data_Rx_YDB11_IAGAIN_34_DwPhy_120202 = DwPhyTest_RunSuite('FullRxEval22','YDB11 IAGAIN 34 120202',1,0,sprintf('UserReg = {{65,34}}'));
data_Rx_YDB11_IAGAIN_34_DwPhy_120212 = DwPhyTest_RunSuite('FullRxEval22','YDB11 IAGAIN 34 120212',1,0,sprintf('UserReg = {{65,34}}'));
%%
data_RxTiming_YDB11_IAGAIN_34_DwPhy_110112 = DwPhyTest_RunSuite('RxTiming','YDB1',1,0,sprintf('UserReg = {{65,34}}'));
%data_RxTmpTest104_3_YDB11_IAGAIN_34_RxTrim_8_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB11 IAGAIN 34 RxTrim 8 110112 ',1,0,sprintf('UserReg = {{32768 - 128 + 223,5:5,1},{32768 - 128 + 225,7:4,8},{65,34}}'));
data_RxTmpTest104_3_YDB11_IAGAIN_34_RxTrim_8_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB11 IAGAIN 34 RxTrim 8 110112 ',1,0,sprintf('UserReg = {{32768 - 128 + 223,5:5,1},{32768 - 128 + 225,7:7,1},{32768 - 128 + 225,6:6,0},{32768 - 128 + 225,5:5,0},{32768 - 128 + 225,4:4,0},{65,34}}'));
%data_RxTmpTest104_3_YDB11_IAGAIN_34_RxTrim_8_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB11 IAGAIN 34 RxTrim 8 110112 ',1,0,sprintf('UserReg = {{32768 - 128 + 223,5:5,1},{65,34}}'));
%%
data_RxTmpTest_YDB11_IAGAIN_34_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB11 IAGAIN 34 110112 ',1,0,sprintf('UserReg = {{65,34}}'));
data_RxTmpTest_YDB08_IAGAIN_35_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB11 IAGAIN 34 110112 ',1,0,sprintf('UserReg = {{65,35}}'));
data_RxTmpTest_YDB02_IAGAIN_35_DwPhy_110112 = DwPhyTest_RunSuite('TempTest','YDB02 IAGAIN 34 110112 ',1,0,sprintf('UserReg = {{65,35}}'));
%
data_RxTiming_YDB11_IAGAIN_35_DwPhy_120202_TMP = DwPhyTest_RunSuite('RxTiming','YDB11 35 DwPhy 120202 ',1,0,sprintf('UserReg = {{32768 - 128 + 99,0},{65,35}}'));

%
 RF22_WriteRegister(47,67);
 %RF22_WriteRegister(45,64);
 %RF22_WriteRegister(46,0);