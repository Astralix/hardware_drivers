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
