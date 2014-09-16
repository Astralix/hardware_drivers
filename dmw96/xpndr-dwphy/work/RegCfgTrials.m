function TxCont(P)
% %Regular Table of Radio Modes: 00:Sleep,01:Stby,11:Rx,10:Tx
% DwPhyLab_WriteRegister(224,30);
% 
% %All Table Rx: FF
% DwPhyLab_WriteRegister(224,255);
% 
% %All Table Tx: AA
% DwPhyLab_WriteRegister(224,170)
% 
% %Tx Burst "endless" loop (Default is toggling unless force Table to All-Tx
% DwPhyLab_TxBurst( 1e9, 6, 500, 33, 0);
% 
% %Stop Tx (Default, goes back to Rx unless Table is forced )
% DwPhyLab_TxBurst(0);

%%
%Tx CW
DwPhyLab_TxBurst(0)
DwPhyLab_WriteRegister(224,35);
DwPhyLab_TxBurst( 1e9, 6, 500, 39, 0);
%DwPhyLab_TxBurst( 1e9, 6, 500, P, 0);
DwPhyLab_WriteRegister(224,170);

%%
%RFIC OVERWRITE EXAMPLES

% %Overwrite RF TxIQ_GAIN
% RF22_WriteRegister(134,hex2dec('4F'))
% %Overwrite RF TxIQ_PHASE
% RF22_WriteRegister(135,hex2dec('46'))
% 
% %Overwrite RF Tx_DCOC_DACI
% RF22_WriteRegister(238,hex2dec('61'))
% %Overwrite RF Tx_DCOC_DACQ
% RF22_WriteRegister(239,hex2dec('6F'))
% 
% %Read actual tx_dcoc_dac_q
% dec2hex(RF22_ReadRegister(242))
% 
% %Overwrite RF LOFT DACI
% RF22_WriteRegister(62,hex2dec('be'))
% %Overwrite RF LOFT DACQ
% RF22_WriteRegister(63,hex2dec('ca'))

end
 