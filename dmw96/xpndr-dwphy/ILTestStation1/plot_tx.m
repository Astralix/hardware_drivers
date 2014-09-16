data = DwPhyTest_RunSuite('FullTxEval22','Y01',1,0,sprintf('UserReg = {{32768 - 128 + 219,21*2}}'))

DwPhyPlot_TxEval(data)
DwPhyPlot_TxMask(data)