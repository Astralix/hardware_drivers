function outputRegList = LoadRegisters(filename, writableList)

outputRegList = [];
if nargin < 2, writableList = []; end

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
                    if isempty(find(writableList == Addr, 1)), continue; end
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
    
