function Xout = FormatRegByBand(X)

if(nargin<1),
    disp('...using defaults');
    X = [58   255   177   211   245   247   249   251   253   255    85
         65   255   176   208   240   240   240   240   240   240    80
         92   255    24    26    27    28    29    30    31    31    18
         93   255    11    27    27    27    27    27    27    27   177
         94   255   136   136   136   136   136   136   136   136   132
         96   255    25    26    27    28    29    29    30    31    18
         97   255    48    64    64    64    64    80    80    80    64 ];
end

%% Convert Radio Addr to PHY Addr
if(max(X(:,1)) < 256),
    X(:,1) = X(:,1) + 256;
end
N = length(X(:,1));

%% Synthesize 'C' Code
fprintf('    const uint32_t NumRegByBand = %d;\n',N);
fprintf('    dwPhyRegByBand_t RegByBand[] =\n');
fprintf('    {\n');
for i=1:N,
    fprintf('        {0x%s, 0x%s, {',dec2hex(X(i,1),3), dec2hex(X(i,2),2));
    for j=0:8,
        fprintf('0x%s',dec2hex(X(i,3+j),2));
        if(j<8), fprintf(', '); end
    end
    if i==N, fprintf('} }\n');
    else     fprintf('} },\n');
    end
end
fprintf('    }; // synthesized %s\n',datestr(now));

%% Output
if nargout>0,
    Xout = X;
end