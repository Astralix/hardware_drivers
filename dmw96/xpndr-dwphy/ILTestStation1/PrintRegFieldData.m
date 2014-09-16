function PrintRegFieldData(FileName)

load('RegFieldData.mat'); 
HexPrint = 0;
RegFieldDataLength = length(RegFieldData);
if(nargin <= 0)
    file_RegFieldDataPrint = 'PrintRegFieldDataDwPhy.txt';
%    FileName = 'RegFieldDataListPowerUp_B.txt';    

end
file_RegFieldDataPrint  = fopen(FileName,'w');
%fprintf(file_RegFieldDataPrint,'         Name          Address   Bits      Radlab  DwPhy \n');
fprintf(file_RegFieldDataPrint,'         Name          Address   Bits      Writable  DwPhy \n');
fprintf(file_RegFieldDataPrint,'------------------------------------------------------------------\n');
for i = 1:RegFieldDataLength
%    if (RegFieldData(i).Writable ==1)
            BitLocation = [int2str(RegFieldData(i).HighBit),':',int2str(RegFieldData(i).LowBit)];
 %           if (RegFieldData(i).Value ~=   RF22_ReadRegister(RegFieldData(i).Address,BitLocation))    
                    if  ~HexPrint               
%                        fprintf(file_RegFieldDataPrint,'%-20s\t%4d\t %d:%d\t %5d\t %5d\n',RegFieldData(i).Name,RegFieldData(i).Address ,RegFieldData(i).HighBit,RegFieldData(i).LowBit, RegFieldData(i).Value, RF22_ReadRegister(RegFieldData(i).Address,BitLocation));
                        fprintf(file_RegFieldDataPrint,'%-20s\t%4d\t %d:%d\t %5d\t %5d\n',RegFieldData(i).Name,RegFieldData(i).Address ,RegFieldData(i).HighBit,RegFieldData(i).LowBit, RegFieldData(i).Writable, RF22_ReadRegister(RegFieldData(i).Address,BitLocation));
                    else                
                        fprintf(file_RegFieldDataPrint,'%-20s\t%4d\t %d:%d\t %5x\t %5x\n',RegFieldData(i).Name,RegFieldData(i).Address ,RegFieldData(i).HighBit,RegFieldData(i).LowBit, RegFieldData(i).Value, RF22_ReadRegister(RegFieldData(i).Address,BitLocation));
                    end
%            end
%    end
end
fprintf(file_RegFieldDataPrint,'------------------------------------------------------------------\n');
fclose(file_RegFieldDataPrint); 
edit(FileName);