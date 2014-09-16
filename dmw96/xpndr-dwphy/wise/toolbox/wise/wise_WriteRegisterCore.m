function wise_WriteRegisterCore(ReadWriteFn, varargin)
% Generic wrapper for baseband register writes
%
%    wise_WriteRegisterCore(ReadWriteFn, Address, Value) writes Values to the 
%    specified register address using the supplied write function, which has the form
%
%       ReadValue = WriteFn(Address, WriteValue)
%
%    wise_WriteRegisterCore(ReadWriteFn, Address, BitFieldString, Value) writes 
%    the specified value to the bit range at the given address. BitRangeString specifies 
%    the bit field as in '7:0'.
%
%    wise_WriteRegisterCore(ReadWriteFn, Address, BitPosition, Value) writes 
%    value to the specified bit position (7 down to 0) at the given address.
%
%    wise_WriteRegisterCore(ReadWriteFn, FieldName, Value) writes Value to 
%    registers for the parameter described by FieldName whose underlying register location
%    is obtained via RegMapFn.

% Written by Barrett Brickner
% Copyright 2010 DSP Group, Inc., All Rights Reserved.

switch nargin,
    
    case 2, %%% matrix with address, data pairs %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   
        X = varargin{1};
        if size(X,2) ~= 2, error('Badly formed argument list'); end
        for i = 1:size(X,1),
            ReadWriteFn(X(i,1), X(i,2));
        end

    case 3, %%% address or parameter structure and value %%%%%%%%%%%%%%%%%%%%%%
        
        if ischar(varargin{1}) %%% Write by parameter name

            RegField = wise_GetRegisterField(ReadWriteFn, varargin{1});
            wise_WriteRegisterCore(ReadWriteFn, RegField, varargin{2});

        elseif isstruct(varargin{1}),
            
            Z = varargin{1};
            D = varargin{2};
            
            if isfield(Z,'Signed'),
                switch Z.Signed
                    case 1, % Two's Complement
                        W = sum(Z.Field(:,2)) - sum(Z.Field(:,3)) + size(Z.Field,1);
                        M = 2^(W-1);
                        D = mod(D+2*M, 2*M);
                    otherwise,
                        error('Unsupported Signed Format');
                end
            end
            
            if isfield(Z,'min') && isfield(Z,'max')
                if D < Z.min || D > Z.max,
                    error('Value %d out of range [%d,%d]',D,Z.min,Z.max);
                end
            end
            
            for i = size(Z.Field, 1) : -1 : 1,
                Address  = Z.Field(i,1);
                BitField = sprintf('%d:%d',Z.Field(i,2),Z.Field(i,3));
                W = Z.Field(i,2) - Z.Field(i,3) + 1;
                x = bitand(D, 2^W - 1); % value to write to this bitfield
                wise_WriteRegisterCore(ReadWriteFn, Address, BitField, x);
                D = bitshift(D, -W); % shift out bits that have been written
            end
            
        else %%% Write by register address

            ReadWriteFn(varargin{1}, varargin{2});

        end
        
    case 4, %%% Write by address and bit position/field string %%%%%%%%%%%%%%%%
    
        if isa(varargin{1},'function_handle'),  %%%%% read by field name

            RegMapFn = varargin{1};
            Z = RegMapFn(varargin{2});
            wise_WriteRegisterCore(ReadWriteFn, Z, varargin{3});

        else %%%%% read by address and bit position

            Address  = varargin{1};
            BitRange = varargin{2}; 
            Value    = varargin{3};

            if ischar(Value), Value=bin2dec(Value); end

            if ischar(BitRange),
                k = strfind(BitRange,':');
                if isempty(k),
                    BitH = str2double(BitRange);
                    BitL = str2double(BitRange);
                else
                    BitH = str2double( BitRange(1:(k-1)) );
                    BitL = str2double( BitRange(k+1:length(BitRange)) );
                end
            else
                BitH = BitRange;
                BitL = BitRange;
            end

            Mask = 2^(BitH - BitL + 1) - 1;
            if Value > Mask,
                error('Value too large for field');
            end

            NumBytes = ceil((1+BitH)/8);
            ReadMask = bitxor(2^(8*NumBytes)-1, bitshift(Mask, BitL));

            X = bitand(ReadWriteFn(Address), ReadMask);
            X = bitor (X, bitshift(Value, BitL));

            for i=1:NumBytes,
                x = bitand(255, bitshift(X, -8*(NumBytes-i)) );
                ReadWriteFn(Address+i-1, x);
            end
            
        end
        
    otherwise, %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        
        error('Incorrect number of arguments');
        
end
