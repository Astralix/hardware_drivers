function RF22_WriteRegister(Address,Value,Value2)



if Address<128
    
    if nargin<3
        DwPhyLab_WriteRegister(256 + Address,Value);
    else
        DwPhyLab_WriteRegister(256 + Address,Value,Value2);
    end
    
else
    
    if nargin<3
        DwPhyLab_WriteRegister(hex2dec('8000') - 128 + Address,Value);
    else
        DwPhyLab_WriteRegister(hex2dec('8000') - 128 + Address,Value,Value2);
    end
    
end