function Xout = WriteDwPhyDefaultRegRF22

RF22WriteableList = 2:253;

%X = RF22A0(RF22WriteableList+256);
%X = RF22A1(RF22WriteableList+256);
%X = RF22A0_NevadaFPGA(RF22WriteableList+256);
%X = RF22A1_NevadaFPGA(RF22WriteableList+256);
X = RF22A1_Nevada(RF22WriteableList+256);

if nargout>0,
    Xout = X;
end

%%
%% FUNCTION: RF22A1_NevadaFPGA
%%
function X = RF22A1_NevadaFPGA(RF22WritableList)
X1 = LoadRegisters('NevadaFPGA_DefaultSystemReg.txt', []);
X2 = LoadRegisters('RF22RegDefaultsDW52RF22A12Yarden_17Mar2011.txt', RF22WritableList);
X = [X1; X2];
GenerateC([X1; X2], 'NevadaFPGA_RF22A12');

%%
%% FUNCTION: RF22A0_NevadaFPGA
%%
function X = RF22A0_NevadaFPGA(RF22WritableList)
X1 = LoadRegisters('NevadaFPGA_DefaultSystemReg.txt', []);
X2 = LoadRegisters('RF22RegDefaultsDW52RF22A02Yarden_02Dec2010.txt', RF22WritableList);
X = [X1; X2];
GenerateC([X1; X2], 'NevadaFPGA_RF22A02');

%%
%% FUNCTION: RF22A0
%%
function X = RF22A0(RF22WritableList)
X1 = LoadRegisters('DW52_MojaveReg_SandyD_B31.txt', []);
X2 = LoadRegisters('RF22RegDefaultsDW52RF22A02Yarden_02Dec2010.txt', RF22WritableList);
X = [X1; X2];
GenerateC([X1; X2], 'Mojave_RF22A02');

%%
%% FUNCTION: RF22A1
%%
function X = RF22A1(RF22WritableList)
X1 = LoadRegisters('DW52_MojaveReg_SandyD_B31.txt', []);
X2 = LoadRegisters('RF22RegDefaultsDW52RF22A12Yarden_17Mar2011.txt', RF22WritableList);
X = [X1; X2];
GenerateC([X1; X2], 'Mojave_RF22A12');

function X = RF22A1_Nevada(RF22WritableList)
X1 = LoadRegisters('Nevada_SandyD_B31.txt', []);
X2 = LoadRegisters('RF22RegDefaultsDW52RF22A12Yarden_09May2011.txt', RF22WritableList);
X = [X1; X2];
GenerateC([X1; X2], 'Nevada_RF22A12');

%%
%% FUNCTION: GenerateC
%%
function GenerateC(X ,arrayname)
m = 8; % {Addr,Data} pairs per line
n = length(X);
indent = '    ';
fprintf('\n');
fprintf('%sstatic dwPhyRegPair_t DefaultReg_%s[] = \n',indent,arrayname);
fprintf('%s{\n',indent);
for i=1:n,
    if mod(i,m)==1, fprintf('%s    ',indent); end
    
    if (X(i,1) >= (256 + 128)) && (X(i,1) <= (256 + 255))
        addr = (X(i,1)-256-128) + hex2dec('8000'); % to indicate need of indirect read/write
    else                     
        addr = X(i,1); 
    end    
    data = X(i,2);
    
    fprintf('{0x%s,0x%s}',dec2hex(addr,4),dec2hex(data,2));
    if i<n, fprintf(', '); end
    if mod(i,m)==0, fprintf('\n'); end
end
fprintf('\n%s}; // Generated %s',indent,datestr(now));
fprintf('\n\n');
  
%%
%% FUNCTION: LoadRegisters
%%
function outputRegList = LoadRegisters(filename, writableList)

outputRegList = [];

F = fopen(filename,'r');
if F<1, error('Unable to load file %s',filename); end

n = 0; k = 0;
while(~feof(F))
    s = fgets(F);
    t = regexp(s, '(?<!.*//.*)Reg\[ *(\d+)\] *= *8''d(\d+)', 'tokens');
    k = k + 1;
    try        
        if ~isempty(t)                
            Addr = str2num(t{1}{1});           % convert to address
            if(Addr>0 && Addr<=1023)           % if a valid PHY address...                
                if ~isempty(writableList)      % skip read-only registers if a check list exists
                    if isempty(find(writableList == Addr)) continue; end
                end
                
                n = n + 1;                     % count number of registers
                Data = str2num(t{1}{2});
                if( ~(Data>=0 && Data<256) )
                    msg = sprintf('Data out of range on line %d\n\n%s',k,s);
                    uiwait(errordlg(msg,'Load Registers'));
                    fclose(F); return;
                end
                RegList(n,1) = Addr;
                RegList(n,2) = Data;
            else
                msg = sprintf('Address out of range on line %d\n\n%s',k,s);
                uiwait(errordlg(msg,'Load Registers'));
                fclose(F); return;
            end
        end
    catch
        err = lasterror;
        msg = sprintf('Unable to process line %d\n%s\n\n%s',k,err.message,s);
        uiwait(errordlg(msg,'Load Registers'));
        fclose(F); return;
    end
end
fclose(F);

outputRegList = RegList;
    
