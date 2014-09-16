function DwPhyAddress =  RF22_GetAddress(Address)
if Address<128
    DwPhyAddress = dec2hex(256 + Address)
else
    DwPhyAddress = dec2hex(hex2dec('8000') - 128 + Address)
end
