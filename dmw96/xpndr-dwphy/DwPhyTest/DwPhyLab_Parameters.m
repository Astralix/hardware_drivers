function ParamOut = DwPhyLab_Parameters(ParamName, ParamValue)
% ParamOut = DwPhyLab_Parameters
%
% ParamOut = DwPhyLab_Parameters(ParamName)
%            DwPhyLab_Parameters(ParamName,ParamValue)

% Written by Barrett Brickner
% Copyright 2008-2010 DSP Group, Inc., All Rights Reserved.

persistent param; % retain param structure between function calls
mlock;            % prevent this file and the param array from being cleared

if isempty(param),
    try
        param = DwPhyLab_LocalDefaults;
    catch
        param = [];
        
        param.STA.IP = '10.82.0.XXX';
        param.STA.Port = 30000;
        
        param.STA.WiFiMAC = '452301900300';
        param.CalDataFile = 'MNPhyLab_CableLoss_ShieldBox.mat';
    end
end
if nargin==1,
    if isfield(param,ParamName),
        ParamOut = param.(ParamName);
    else
        ParamOut = [];
    end
    return;
end
if nargin==2,
    param.(ParamName) = ParamValue;
end

% Force defaults
if ~isfield(param,'Part'), param.Part=''; end
if ~isfield(param,'PCB'),  param.PCB=''; end;
if ~isfield(param,'DwPhyTestExceptions'), param.DwPhyTestExceptions = ''; end
if ~isfield(param,'BaseAddress'), param.BaseAddress = hex2dec('06500000'); end
if ~isfield(param,'PlatformID'),  param.PlatformID = 0; end
if ~isfield(param,'UseAgilentWaveformDownloadAssistent'), param.UseAgilentWaveformDownloadAssistent = 0; end
param.DwPhyMex        = DwPhyMex('About');
param.wiseMex         = wiseMex('wiMain_VersionString()');
param.ComputerName    = getenv('COMPUTERNAME');
param.OperatingSystem = getenv('OS');
param.CPU             = getenv('PROCESSOR_IDENTIFIER');
param.MatlabVersion   = version;
param.matlabroot      = matlabroot;

% output the parameters
if nargout,
    ParamOut = param;
end

%% REVISIONS
% 2008-06-27 Block output if not requested
%            Added field DwPhyTest
% 2010-04-02 Add default value for parameter BaseAddress
