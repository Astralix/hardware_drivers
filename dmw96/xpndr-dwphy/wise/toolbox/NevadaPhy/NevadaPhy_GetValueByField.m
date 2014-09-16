function [Value, ok] = NevadaPhy_GetValueByField(RegList, RegByField, Label)
%[Value, OK] = NevadaPhy_GetValueByField(RegList, RegByField, Label)

%   Developed by Barrett Brickner
%   Copyright 2009 DSP Group, Inc. All rights reserved.

if nargin ~= 3, error('Wrong number of input arguments'); end;

%--- Return if label is not in list
if(~isfield(RegByField,Label))
    Value = 0;
    ok = 0;
    return;
end

%--- Return if required registers are not in the list
L = 0;
X = RegByField.(Label);
A = X.Field(:,1);
for m=1:length(A),
    if(length(find(RegList(:,1)==A(m)))~=1)
        Value = 0; ok = 0;
        return;
    else
        L = L + X.Field(m,2) - X.Field(m,3) + 1;
    end
end

%--- Extract value from register list
D = 0;
Lacc = 0;
for m=length(A):-1:1,
    a = X.Field(m,2);                % top bit
    b = X.Field(m,3);                % bottom bit
    k = find(RegList(:,1)==A(m));    % register index
    mask = sum(bitset(0,1+(b:a)));   % mask in register
    d = bitshift(bitand(RegList(k,2), mask),-b); % extract data
    D = bitor(D, bitshift(d,Lacc));  % OR data into total
%    fprintf('   m=%d, k=%d, [%d:%d], Lacc=%d, mask=%s, d=%s->%d,D=%d\n',m,k,a,b,Lacc,dec2bin(mask,8),dec2bin(RegList(k,2),8),d,D);
    Lacc = Lacc + a - b + 1;
end

%--- Format conversion
if(isfield(X,'Signed'))
    signed = X.Signed;
else
    signed = 0;
end
if(signed)
    if(bitget(D,L))
        Value = D - 2^L;
    else
        Value = D;
    end
else
    Value = D;
end
ok = 1;
