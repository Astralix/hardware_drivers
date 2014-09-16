function Value = RF22_ReadRegister(Address, BitRange)


if Address<128
    if nargin<2
    Value = DwPhyLab_ReadRegister(256 + Address);
    else
        Value = DwPhyLab_ReadRegister(256 + Address, BitRange);
    end
    
else
    if nargin<2
        Value = DwPhyLab_ReadRegister(hex2dec('8000') - 128 + Address);
    else
        Value = DwPhyLab_ReadRegister(hex2dec('8000') - 128 + Address, BitRange);
    end

    
end