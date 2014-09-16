function CheckRegFieldData()

load('RegFieldData.mat'); 
HexPrint = 0;
RegFieldDataLength = length(RegFieldData);
file_RegFieldDataCheck  = fopen('RegFieldDataCheck.txt','w');
fprintf(file_RegFieldDataCheck,'         Name          Address   Bits      Radlab  DwPhy \n');
fprintf(file_RegFieldDataCheck,'------------------------------------------------------------------\n');
for i = 1:RegFieldDataLength
    if (RegFieldData(i).Writable ==1)
            BitLocation = [int2str(RegFieldData(i).HighBit),':',int2str(RegFieldData(i).LowBit)];
            if (RegFieldData(i).Value ~=   RF22_ReadRegister(RegFieldData(i).Address,BitLocation))    
                    if  ~HexPrint               
                        fprintf(file_RegFieldDataCheck,'%-20s\t%4d\t %d:%d\t %5d\t %5d\n',RegFieldData(i).Name,RegFieldData(i).Address ,RegFieldData(i).HighBit,RegFieldData(i).LowBit, RegFieldData(i).Value, RF22_ReadRegister(RegFieldData(i).Address,BitLocation));
                    else                
                        fprintf(file_RegFieldDataCheck,'%-20s\t%4d\t %d:%d\t %5x\t %5x\n',RegFieldData(i).Name,RegFieldData(i).Address ,RegFieldData(i).HighBit,RegFieldData(i).LowBit, RegFieldData(i).Value, RF22_ReadRegister(RegFieldData(i).Address,BitLocation));
                    end
            end
    end
end
fprintf(file_RegFieldDataCheck,'------------------------------------------------------------------\n');
fclose(file_RegFieldDataCheck); 
edit RegFieldDataCheck.txt