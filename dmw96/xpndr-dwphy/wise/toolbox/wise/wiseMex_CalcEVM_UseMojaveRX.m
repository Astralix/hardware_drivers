%% wiseMex_CalcEVM_UseMojaveRX
function wiseMex_CalcEVM_UseMojaveRX(UseMojaveRX)
if(nargin<1) UseMojaveRX=1; end
if(UseMojaveRX)
    wiParse_Line('CalcEVM.UseMojaveRX=1');
    wiParse_Line('Mojave_WriteRegister(3,1)');      %PathSel
    wiParse_Line('Mojave_WriteRegister(4,1)');      %RXMode
    wiParse_Line('Mojave_WriteRegister(80,0x12)');  %MaxUpdataCount
    wiParse_Line('Mojave_WriteRegister(88,0x04)');  %DFSPattern
    wiParse_Line('Mojave_WriteRegister(89,0x96)');  %DFSdTPRF
    wiParse_Line('Mojave_WriteRegister(90,0x2A)');  %DFSdTPRF
    wiParse_Line('Mojave_WriteRegister(121,0x5C)'); %DFSdTPRF
    wiParse_Line('Mojave_WriteRegister(209,0x1)');  %WakeupTimeL
    wiParse_Line('Mojave_WriteRegister(210,0x8)');  %DelayStdby
    wiParse_Line('Mojave_WriteRegister(211,0x6)');  %DelayBGEnB
    wiParse_Line('Mojave_WriteRegister(212,0x6)');  %DelayADC1
    wiParse_Line('Mojave_WriteRegister(217,0x3)');  %DelayFCAL1
    wiParse_Line('Mojave_WriteRegister(218,0x3)');  %DelayFCAL2
    wiParse_Line('Mojave_WriteRegister(222,0xF1)'); %DelaySquelchRF
    wiParse_Line('Mojave_WriteRegister(233,0x00)'); %ADCFCAL1, ADCFCAL2
else
  wiParse_Line('CalcEVM.UseMojaveRX=0');  
end


