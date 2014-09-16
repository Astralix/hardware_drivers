function Data = wise_ReadRegisterCore(ReadFn, varargin)
% Generic wrapper for baseband register reads
%
%    Data = wise_ReadRegisterCore(ReadFn, Address) returns the value of the
%    register at the specified address using the supplied read function which has the form
%
%        Value = ReadFn(Address)
%
%    and a function to return a structure defining a parameter field with the form
%
%        ParamStruct = RegMapFn(FieldName)
%
%    Data = wise_ReadRegisterCore(ReadFn, Address, BitFieldString) returns the 
%    value for the specified bit range at the given address. BitRangeString specifies the 
%    bit field as in '7:0'.
%
%    Data = wise_ReadRegisterCore(ReadFn, Address, BitPosition) returns the 
%    value at the specified bit position (7 down to 0) at the given address.
%
%    Data = wise_ReadRegisterCore(ReadFn, FieldName) returns the value for the
%    parameter described by FieldName whose underlying register location is obtained via
%    RegMapFn.

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

if nargin == 1, %% Read all registers (range specified by part ID)
    
    BasebandID = ReadFn(1);
    if any(BasebandID == (1:6)), A = 0:255; % Dakota and Mojave
    else                         A = [0:255, 512:1023]; end
    
    Data = zeros(numel(A), 2);
    Data(:,1) = A(:);
    for i = 1:numel(A),
        Data(i,2) = ReadFn(A(i));
    end

elseif nargin == 2,
    
    if ischar(varargin{1}) %%% Read by parameter name
        
        RegField = wise_GetRegisterField(ReadFn, varargin{1});
        Data = wise_ReadRegisterCore(ReadFn, RegField);

    elseif isstruct(varargin{1}) %%% Read by parameter structure
        
        Z = varargin{1};
        D = 0;
        for i = 1:size(Z.Field, 1),
            D = D * 2^(Z.Field(i,2) - Z.Field(i,3) + 1);
            Address  = Z.Field(i,1);
            BitField = sprintf('%d:%d',Z.Field(i,2),Z.Field(i,3));
            D = D + wise_ReadRegisterCore(ReadFn,Address,BitField);
        end
        if isfield(Z,'Signed'),
            switch Z.Signed
                case 1, % Two's Complement
                    W = sum(Z.Field(:,2)) - sum(Z.Field(:,3)) + size(Z.Field,1);
                    M = 2^(W-1);
                    D = mod(D+M, 2*M) - M;
                otherwise,
                    error('Unsupported Signed Format');
            end
        end
        Data = D;
        
    else %%% Read by register address
        
        Address = varargin{1};
        if numel(Address) == 1,
            Data = ReadFn(Address);
        else
            if min(size(Address)) > 1, error('Badly formed argument'); end
            Data = zeros(size(Address));
            for i = 1:length(Address),
                Data(i) = ReadFn(Address(i));
            end
        end
    end
    
elseif nargin == 3, %%% Read by address and bit position/field string or by FieldName
    
    if isa(varargin{1},'function_handle'),  %%%%% read by field name

        RegMapFn = varargin{1};
        Z = RegMapFn(varargin{2});
        Data = wise_ReadRegisterCore(ReadFn, Z);
        
    else %%%%% read by address and bit position
    
        Address  = varargin{1};
        BitRange = varargin{2};

        if ischar(BitRange),
            k = strfind(BitRange,':');
            BitH = str2double( BitRange(1:(k-1)) );
            BitL = str2double( BitRange(k+1:length(BitRange)) );
        else
            BitH = BitRange;
            BitL = BitRange;
        end

        X = 0;
        NumBytes = ceil((1+BitH)/8);
        for i = 1:NumBytes,
            x = ReadFn(Address+i-1);
            X = bitor( X, bitshift(x, 8*(NumBytes-i)) );
        end
        Mask = bitshift(2^(BitH-BitL+1) - 1, BitL);
        Data = bitshift( bitand(X, Mask), -BitL);

    end
    
else
    error('Incorrect number of arguments');
end
