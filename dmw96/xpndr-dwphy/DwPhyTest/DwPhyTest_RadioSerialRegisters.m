function data = DwPhyTest_RadioSerialRegisters
% data = DwPhyTest_RadioSerialRegisters tests access to RF52 serial registers.
%    Write/read tests are run at all timing/speeds allowed by the DW52 baseband.
%    Pass/fail is based on the default operating speed.

% Written by Barrett Brickner
% Copyright 2008 DSP Group, Inc., All Rights Reserved.
% DwPhy Test 2.2 - Radio Register Write/Read

% Standard results
DwPhyLab_Setup;
data.Test = mfilename('fullpath');
data.Timestamp = datestr(now);
data.Result = 1;

% Put the baseband to sleep, then reset
DwPhyLab_Sleep; pause(0.001);
DwPhyLab_Initialize;

data.WriteList = RadioWriteableRegisters;
data.SREdge = 0:1;
data.SRFreq = 0:3;

hBar = waitbar(0.0,'Testing Nominal Speed...','Name',mfilename,'WindowStyle','modal');

% Test under default settings
data.Result = WriteReadTest(data.WriteList, 1, hBar);

% Test for all speeds
for SREdge = data.SREdge,
    for SRFreq = data.SRFreq,
        
        DwPhyCheck( DwPhyMex('DwPhy_Reset') ); % in case registers are corrupt from last run

        if ishandle(hBar)
            waitbar((1+4*SREdge+SRFreq)/9,hBar,sprintf('SREdge = %d, SRFreq = %d',SREdge,SRFreq));
        else
            fprintf('*** TEST ABORTED ***\n');
            return ;
        end
        
        DwPhyLab_WriteRegister(237, 128 + 4*SREdge + SRFreq);
        data.ResultBySpeed(SREdge+1,SRFreq+1) = WriteReadTest(data.WriteList, 0, hBar);
    end
end
DwPhyLab_WriteRegister(237, 128); % reset serial register clock speed
DwPhyCheck( DwPhyMex('DwPhy_Reset') ); % in case registers are corrupt from last run
data.Runtime = 24*3600*(now - datenum(data.Timestamp));
if ishandle(hBar), close(hBar); end;

%% Write/Read Test
function Result = WriteReadTest(WriteList, Verbose, hBar)
if nargin<3, hBar = []; end;
addr = WriteList(:,1);
mask = WriteList(:,2);
NumReg = length(addr);
y = zeros(NumReg,1);
Result = 1;

for i=1:16 % extend to 256 if testing all patterns

    if ~isempty(hBar) && ~ishandle(hBar) % check for abort
        Result = -1;
        return;
    end

    writedata = mod( (1:NumReg)+i, 256 ); % test data
    for k=1:NumReg,
        DwPhyLab_WriteRegister(addr(k), writedata(k));
    end
    for k=1:NumReg,
        y(k) = DwPhyLab_ReadRegister(addr(k));
        difference = bitand( bitxor( writedata(k), y(k) ), mask(k) );
        if difference ~=0,
            Result = 0; % test failed
            if Verbose,
                fprintf('> %s failed on register %d\n',mfilename,WriteList(k,1));
            end
        end
    end
    
end    

%% RadioWriteableRegisters - array with [addr, mask]
function CheckList = RadioWriteableRegisters
if DwPhyLab_RadioIsRF22
    RegMaskList = WriteableAddressesRF22A;
    CheckList = zeros(length(RegMaskList), 2);
    for i=1:size(CheckList,1)
        if RegMaskList{i}(1) >= 128
            CheckList(i,1) = (RegMaskList{i}(1) - 128) + hex2dec('8000'); % indirect access protocol
        else
            CheckList(i,1) = RegMaskList{i}(1) + 256;
        end
        CheckList(i,2) = RegMaskList{i}(2);    
    end    
else    
    CheckList = WriteableAddressesRF52A120;
    CheckList = reshape(CheckList, length(CheckList), 1);
    CheckList = CheckList + 256; % convert to Phy address space
    CheckList(:,2) = 255*ones(size(CheckList));
end

%% WriteableAddressesRF52A120
function CheckList = WriteableAddressesRF52A120
CheckList = [ ...
      2   3   4   5  10  11  12  13  14  15  16  17  18  19  20  21 ...
     22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37 ...
     38  40  47  48  49  50  51  52  53  54  55  56  57  58  59  60 ...
     61  62  63  64  65  66  67  68  69  70  71  72  73  74  76  78 ...
     79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94 ...
     95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 ...
    111 112 113 114 115 116 117 118 119 120 121 122 123 124 125     ...
];

%% WriteableAddressesRF22A
function CheckList = WriteableAddressesRF22A
CheckList = { ...
    [  2, 63] [  3, 31]           [  5,127] [ 12,255] [ 13,255] [ 14,255] [ 15,255] ...
    [ 16,255] [ 17, 63] [ 18, 55] [ 19,  3] [ 20,255] [ 21,255]           [ 23,255] ...
    [ 24,127] [ 25, 63] [ 26,255] [ 27,207] [ 28,255] [ 29, 15] [ 30,127] [ 31,255] ...
    [ 32, 15] [ 41, 63] [ 42, 63] [ 43, 63] [ 45, 63] [ 46, 63] [ 47, 63]           ...
    [ 49, 63] [ 50, 63] [ 60,  3] [ 61,252] [ 62,255] [ 63,255] [ 64,255] [ 65,255] ...
    [ 66,255] [ 67,255] [ 70,252] [ 71, 63] [ 72,252] [ 73,127] [ 74,127] [ 75,  3] ...
    [ 76,255] [ 80,248] [ 81, 63] [ 82, 63] [ 83,255] [ 84,255] [ 85,255] [ 86,127] ...
    [ 89,127] [ 90,127] [ 91,127] [ 92,119] [ 93,243] [ 94, 15] [ 95,255] [ 96,255] ...
    [ 97, 15] [100,252] [101,255] [102,191] [103,247] [104,255] [105,255] [106,  3] ...
    [107,255] [108,127] [110,255] [111,  7] [112, 63] [113,255] [114,255] [115,255] ...
    [116,255] [117,253] [118,255] [119,255] [120,255] [121,255] [122,255] [123,255] ...
    [124,255] [125,255]           [129,254] [130,127] [132,255] [133,255] [134,159] ...    
    [135,159] [136,159] [137,159] [143,255] [144,255] [145,255] [146, 63] [147,255] ...
    [148,127] [149, 15] [152, 63] [153,255] [154,255] [155,127] [156,255] [157,255] ...
    [158,255] [159,255] [160,255] [161,255] [162,255] [163,255] [164,255] [165,255] ...
    [166,255] [167,255] [168,255] [169,255] [172,255] [173, 15] [174,255] [175,255] ...
    [176,255] [177,255] [178,255] [179,255] [180,255] [181,127] [183, 15] [184,127] ...
    [185,255] [186,255] [187,255] [189, 63] [190, 63] [191,255] [192,255] [193, 15] ...
    [194,127] [195,127] [196,127] [197,127] [198,127] [199,127] [200,127] [201,255] ...
    [202,255] [203,255] [204, 55] [205,255] [206,255] [207,255] [208,255] [209,255] ...
    [210,255] [211,255] [212,255] [213,255] [214,255] [215,255] [216,255] [217, 63] ...
    [218,255] [219,127] [220,255] [221, 15] [223, 55] [224,255] [225,255] [226,127] ...
    [227, 63] [228,252] [229, 63] [230, 63] [231, 63] [232, 63] [233,127] [236,239] ...
    [237, 15] [238, 63] [239, 63] [240,255] [243,255] [244,255] [245,255] [246, 15] ...
    [247,127] [248,127] [249,255] [250,255] [251, 31]
};

%% REVISIONS
% 080502 Change clock speed to 10 MHz at end of test prior to reset
% 080602 Moved addresses from A120WriteableAddresses.txt into a local function
% 100927 [SM]: Add support for RF22 register/mask list
