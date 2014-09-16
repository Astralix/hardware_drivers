function [X0,X1] = CompareRegValues
% Compare working set of registers to baseline to indicate differences

X0 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_01Aug2008.txt');
X1 = LoadRegisters('RF52RegDefaultsDW52RF52B21Sandy_external_18Aug2008.txt');

%%% Find differences
Ad = [];
for i=1:length(X1),
    A = X1(i,1);
    k = find(X0(:,1) == A);
    if(isempty(k))
        Ad = [Ad; A];
    elseif(X1(i,2) ~= X0(k,2))
        Ad = [Ad; A];
    end
end

%%% Display info
for i=1:length(Ad),
    A = Ad(i);
    if A>=256 && A<384, Ax = A-256; else Ax=A; end
    d0 = X0(find(X0(:,1)==A),2);
    d1 = X1(find(X1(:,1)==A),2);
    fprintf('A = %3d (%3d), Original = %s (%3d), New = %s (%3d)\n',A,Ax,dec2bin(d0,8),d0,dec2bin(d1,8),d1);
end


%% FUNCTION: LoadRegisters
%
function outputRegList = LoadRegisters(filename)
% RegList = BERLAB_LoadRegisters(filename)

outputRegList = [];

F = fopen(filename,'r');
if(F<1), error('Unable to load file %s',filename); end

n = 0; k=0;
while(~feof(F))
    s = fgets(F); k=k+1;
    try
        if(sum(s(1:2)=='//') < 2)                % skip comment lines
            k1 = min(strfind(s,'//'));           % find first comment
            if(~isempty(k1)) s=s(1:(k1-1)); end; % remove comments
            n = n + 1;                           % count number of registers
            k1 = min(strfind(s,'Reg['))+4;       % skip over "Reg["
            while(s(k1)==' ') k1=k1+1; end       % skip whitespace
            k2 = min(strfind(s,']')) - 1;        % find the end of Reg[ ]
            Addr = str2double(s(k1:k2));         % ...and convert to address
            if(Addr>0 && Addr<=1023)             % if a valid PHY address...
                k1 = strfind(s,'=8''d') + 4;
                if(isempty(k1)),
                    msg = sprintf('Format error on line %d\n\n%s',k,s);
                    uiwait(errordlg(msg,'Load Registers'));
                    fclose(F); return;
                end
                k2 = length(s);
                if(isempty(k2)), k2 = length(s); end;
                Data = str2double(s(k1:k2));
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
