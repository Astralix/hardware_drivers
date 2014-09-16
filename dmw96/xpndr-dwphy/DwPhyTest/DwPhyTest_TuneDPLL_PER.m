function data = DwPhyTest_TuneDPLL_PER(varargin)
% data = DwPhyTest_TuneDPLL(...)
%
%    Input arguments are strings of the form '<param name> = <param value>'
%    Valid parameters include Mbps, fcMHz...

data.Description = mfilename;
data.Timestamp = datestr(now);

%% Default and User Parameters
Mbps  = 54;   %#ok<NASGU>
fcMHz = 2412;
PrdBm = -70;
MinFail = 100;
MaxPkts = 10000;

DwPhyLab_EvaluateVarargin(varargin);

%% Run Test
DwPhyTest_RunTestCommon;
hBar = waitbar(0.0,'Initializing','Name',mfilename, 'WindowStyle','modal');
data.Ploss   = DwPhyLab_RxCableLoss(fcMHz);

%%% Load the PER waveform
waitbar(0.0,hBar,''); set(hBar,'UserData',[0.0 0.2]);
DwPhyLab_LoadPER( data, hBar );
DwPhyLab_SetRxPower(PrdBm + data.Ploss);
if ~ishandle(hBar), fprintf('*** TEST ABORTED ***\n'); return; end

data.Ca = 0:4;
data.Cb = 0:7;

for Ca = data.Ca,
    for Cb = data.Cb,
        x = 8*Ca + Cb;
        data.CaCb(Ca+1,Cb+1) = x;
        for i=0:15,
            DwPhyLab_WriteRegister(48+i, x);
        end
        if ~ishandle(hBar), 
            fprintf('*** TEST ABORTED ***\n'); 
            return;
        else
            waitbar(0.1+0.9*x/40,hBar,{'',sprintf('Measuring with Ca = %d, Cb = %d',Ca,Cb)}); 
            set(hBar,'UserData',[0.1+0.9*x/64 1/64]);
        end
        i = Ca + 1;
        j = Cb + 1;
        [data.PER(i,j),data.RSSI(i,j),data.Npkts(i,j),data.ErrCount(i,j), data.result{i,j}] = GetPER(hBar, MaxPkts, MinFail);
    end
end

if ishandle(hBar), close(hBar); end;

%% Summary Results
[I,J] = find(data.PER == min(min(data.PER)));
data.CaCb_Opt = [data.Ca(I(1)) data.Cb(J(1))];
data.Runtime = 24*3600*(now - datenum(data.Timestamp));


%% FUNCTION GetPER
function [PER, RSSI, Npkts, ErrCount, result] = GetPER(hBar, MaxPkts, MinFail)

PktsPerSequence = DwPhyLab_Parameters('PktsPerSequencePER');
Npkts = 0; ErrCount = 0; 
PER = 1;

while (Npkts < MaxPkts) && (ErrCount < MinFail),
    
    result = DwPhyLab_GetPER;
    ErrCount = ErrCount + (PktsPerSequence - result.ReceivedFragmentCount);
    Npkts = Npkts + PktsPerSequence;
    PER = ErrCount / Npkts;
    RSSI = result.RSSI;

    if ~isempty(hBar) && ~ishandle(hBar), return; end % user abort
end
