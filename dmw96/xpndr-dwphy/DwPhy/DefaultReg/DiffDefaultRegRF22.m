function Xout = DiffDefaultRegRF22

RF22WritableList = [
      2   3   4   5  12  13  14  15  16  17  18  19  20  21  22  23 ...
     24  25  26  27  28  29  30  31  32  41  42  43  45  46  47  48 ...
     49  50  60  61  62  63  64  65  66  67  70  71  72  73  74  75 ...
     76  80  81  82  83  84  85  86  89  90  91  92  93  94  95  96 ...
     97 100 101 102 103 104 105 106 107 108 110 111 112 113 114 115 ...
    116 117 118 119 120 121 122 123 124 125 128 129 130 132 133 134 ...
    135 136 137 143 144 145 146 147 148 149 152 153 154 155 156 157 ...
    158 159 160 161 162 163 164 165 166 167 168 169 172 173 174 175 ...
    176 177 178 179 180 181 183 184 185 186 187 189 190 191 192 193 ...
    194 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 ...
    210 211 212 213 214 215 216 217 218 219 220 221 223 224 225 226 ...
    227 228 229 230 231 232 233 236 237 238 239 240 243 244 245 246 ...
    247 248 249 250 251
];

filenameOld = 'RF22RegDefaultsDW52RF22A02Yarden_02Dec2010.txt';
filenameNew = 'RF22RegDefaultsDW52RF22A12Yarden_06Feb2011.txt';

F1 = fopen(filenameOld,'r');
F2 = fopen(filenameNew,'r');
if F1<1, error('Unable to load file %s',filename1); end
if F2<1, error('Unable to load file %s',filename2); end
fprintf('\n%s  ==>  %s\n', filenameOld, filenameNew);

n = 0; k = 0;
while(~feof(F1))
    s = fgets(F1);
    t1 = regexp(s, '(?<!.*//.*)Reg\[ *(\d+)\] *= *8''d(\d+)', 'tokens');

    s = fgets(F2);
    t2 = regexp(s, '(?<!.*//.*)Reg\[ *(\d+)\] *= *8''d(\d+)', 'tokens');
    
    if ~isempty(t1)                
        Addr1 = str2num(t1{1}{1}) - 256;   
        Data1 = str2num(t1{1}{2});
    
        Addr2 = str2num(t2{1}{1}) - 256;   
        Data2 = str2num(t2{1}{2});
    
        if Addr1 ~= Addr2 
            fprintf('Mismatch in no. of registers.\n'); break;
        end

        if ~isempty(find(RF22WritableList == Addr1)) && Data1 ~= Data2
            if Addr1 < 128 mAddr1 = 256 + Addr1;
            else           mAddr1 = 32768 + Addr1 - 128;
            end
            fprintf('Reg[%3d](0x%04X) = %3d(%s) ==> %3d(%s)\n', Addr1, mAddr1, Data1, dec2bin(Data1, 8), Data2, dec2bin(Data2, 8));
        end
    end
end
fclose(F1);
fclose(F2);

