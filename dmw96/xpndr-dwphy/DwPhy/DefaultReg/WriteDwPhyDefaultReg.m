function Xout = WriteDwPhyDefaultReg

%X = RF52A321;
%X = RF52A120;
%X = RF52A321_TXALC;
%X = RF52B11_NoLNA;
%X = RF52B21;
%X = RF52B31;
%X = NevadaFPGA;
X = NevadaRF52B31;

if nargout>0,
    Xout = X;
end

%% NevadaRF52B31
function X = NevadaRF52B31
X1 = LoadRegisters('Nevada_SandyD_B31.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Sandy_external_08Apr2009.txt');
X2 = LoadRegisters('RegDefaultsRF52B31_SandyD_110407.txt');
X = [X1; X2];
GenerateC([X1; X2], 'Nevada_RF52B31');

%% NevadaFPGA
function X = NevadaFPGA
X1 = LoadRegisters('NevadaFPGA_DefaultSystemReg.txt');
X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Sandy_external_08Apr2009.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Debby_external_19Aug2009.txt'); % updated 5 GHz values
X = [X1; X2];
GenerateC([X1; X2], 'NevadaFPGA_RF52B31');

%% RF52B31
function X = RF52B31
X1 = LoadRegisters('DW52_MojaveReg_SandyD_B31.txt');

% X2a = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_open9_01Aug2008.txt');
% X2b = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_internal9_01Aug2008.txt');
% X2c = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_01Aug2008.txt');
% X = [X1; X2a];
% GenerateC([X1; X2a], 'Mojave_RF52B21_0801_OpenLoop');
% GenerateC([X1; X2b], 'Mojave_RF52B21_0801_IntTxALC');
% GenerateC([X1; X2c], 'Mojave_RF52B21_0801_ExtTxALC');

%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_18Aug2008.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_27Aug2008.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Sandy_external_08Apr2009.txt');
X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Sandy_external_08Apr2009.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B31Debby_external_19Aug2009.txt'); % updated 5 GHz values
X = [X1; X2];
GenerateC([X1; X2], 'Mojave_RF52B31');


%% RF52B21
function X = RF52B21
X1 = LoadRegisters('DW52_MojaveReg_SandyC_B21.txt');

% X2a = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_open9_01Aug2008.txt');
% X2b = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_internal9_01Aug2008.txt');
% X2c = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_01Aug2008.txt');
% X = [X1; X2a];
% GenerateC([X1; X2a], 'Mojave_RF52B21_0801_OpenLoop');
% GenerateC([X1; X2b], 'Mojave_RF52B21_0801_IntTxALC');
% GenerateC([X1; X2c], 'Mojave_RF52B21_0801_ExtTxALC');

%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_18Aug2008.txt');
%X2 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_27Aug2008.txt');
X2 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_140_07Nov2008.txt');
X = [X1; X2];
GenerateC([X1; X2], 'Mojave_RF52B21');


%% RF52B11 (DebbyD - No External LNA)
function X = RF52B11_NoLNA
X1 = LoadRegisters('DW52_BringupSystemReg_DebbyC_A321.txt');
X2 = LoadRegisters('RegDefaultsDW52RF52B11Alberto_07May2008.txt');
X = [X1; X2];
GenerateC(X, 'Mojave_RF52B11_NoLNA');

%% RF52A321 (DebbyC)
function X = RF52A321_TXALC
X1 = LoadRegisters('DW52_BringupSystemReg_DebbyC_A321.txt');
X2 = LoadRegisters('RegDefaultsDW52RF52A321Alberto_16Jan2008 (TX ALC).txt');
X = [X1; X2];
GenerateC(X, 'Mojave_RF52A321_TXALC');


%% RF52A321 (DebbyC)
function X = RF52A321
X1 = LoadRegisters('DW52_BringupSystemReg_DebbyC_A321.txt');
X2 = LoadRegisters('RegDefaultsDW52RF52A321Alberto_05Dec2007.txt');
X = [X1; X2];
GenerateC(X, 'Mojave_RF52A321');


%% RF52A120 (DebbyA)
function X = RF52A120
X1 = LoadRegisters('DW52_BringupSystemReg.txt');
X2 = LoadRegisters('RegDefaultsDW52RF52A120Alberto_02Oct2007.txt');
X = [X1; X2];
GenerateC(X, 'Mojave_RF52A120');


%% FUNCTION: GenerateC
function GenerateC(X ,arrayname)
m = 8; % {Addr,Data} pairs per line
n = length(X);
indent = '    ';
fprintf('\n');
fprintf('%sstatic dwPhyRegPair_t DefaultReg_%s[] = \n',indent,arrayname);
fprintf('%s{\n',indent);
for i=1:n,
    if mod(i,m)==1, fprintf('%s    ',indent); end
    fprintf('{0x%s,0x%s}',dec2hex(X(i,1),3),dec2hex(X(i,2),2));
    if i<n, fprintf(', '); end
    if mod(i,m)==0, fprintf('\n'); end
end
fprintf('\n%s}; // Generated %s',indent,datestr(now));
fprintf('\n\n');
        
%% FUNCTION: LoadRegisters
%
% function outputRegList = LoadRegisters(filename)
% % RegList = BERLAB_LoadRegisters(filename)
% 
% outputRegList = [];
% 
% F = fopen(filename,'r');
% if F<1, error('Unable to load file %s',filename); end
% 
% n = 0; k=0;
% while(~feof(F))
%     s = fgets(F); k=k+1;
%     try
%         if(sum(s(1:2)=='//') < 2)                % skip comment lines
%             k1 = min(strfind(s,'//'));           % find first comment
%             if ~isempty(k1), s=s(1:(k1-1)); end; % remove comments
%             n = n + 1;                           % count number of registers
%             k1 = min(strfind(s,'Reg['))+4;       % skip over "Reg["
%             while s(k1)==' ', k1=k1+1; end       % skip whitespace
%             k2 = min(strfind(s,']')) - 1;        % find the end of Reg[ ]
%             Addr = str2double(s(k1:k2));         % ...and convert to address
%             if(Addr>0 && Addr<=1023)             % if a valid PHY address...
%                 k1 = strfind(s,'=8''d') + 4;
%                 if(isempty(k1)),
%                     msg = sprintf('Format error on line %d\n\n%s',k,s);
%                     uiwait(errordlg(msg,'Load Registers'));
%                     fclose(F); return;
%                 end
%                 k2 = length(s);
%                 if(isempty(k2)), k2 = length(s); end;
%                 Data = str2double(s(k1:k2));
%                 if( ~(Data>=0 && Data<256) )
%                     msg = sprintf('Data out of range on line %d\n\n%s',k,s);
%                     uiwait(errordlg(msg,'Load Registers'));
%                     fclose(F); return;
%                 end
%                 RegList(n,1) = Addr;
%                 RegList(n,2) = Data;
%             else
%                 msg = sprintf('Address out of range on line %d\n\n%s',k,s);
%                 uiwait(errordlg(msg,'Load Registers'));
%                 fclose(F); return;
%             end
%         end
%     catch
%         err = lasterror;
%         msg = sprintf('Unable to process line %d\n%s\n\n%s',k,err.message,s);
%         uiwait(errordlg(msg,'Load Registers'));
%         fclose(F); return;
%     end
% end
% fclose(F);
% 
% outputRegList = RegList;
%     
