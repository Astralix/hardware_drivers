function [S,Y] = TamalePhy_GetTraceState
% [S,Y] = TamalePhy_GetTraceState;
%         Retrieve trace and state data from TamalePHY, RF55, and Tamale models

S  = wiseMex('Tamale_GetRxState()');
S2 = wiseMex('wiChanl_GetState()');
S.RX.Fs   = S2.Fs;
S.RX.N_Rx = S2.N_Rx;
S.RX.Nr   = S2.Nr;
S.RX.r    = S2.r;
S.RX.Ar   = S2.A;
S.RX.PositionOffset = wiseMex('wiChanl_GetParameter','PositionOffset.t0');
S2 = wiseMex('TamalePhy_GetState()');
S.RX.sRX0 = S2.sRX0;
S.TX = wiseMex('Tamale_GetTxState()');
clear S2;
S2 = wiseMex('bTamale_GetRxState()');
if(S.EnableTrace)
    k = 2:length(S2.EDOut)-4; % shorten vector to avoid end-point problems
    S.bRX.EDOut = S2.EDOut(k);
    S.bRX.CQOut = S2.CQOut(k);
    S.bRX.traceCCK = S2.traceCCK(k);
    S.bRX.traceControl = S2.traceControl(k);
    S.bRX.traceState = S2.traceState(k);
    S.bRX.k80 = S2.k80(k);
    S.bRX.x = S2.x;
    S.bRX.y = S2.y;
    S.bRX.z = S2.z;
    S.bRX.r = S2.r;
    S.bRX.u = S2.u;
    S.bRX.w = S2.w;
    S.bRX.h1 = S2.h1;
    S.bRX.h2 = S2.h2;
    S.bRX.Np = S2.Np;
    S.bRX.Np2 = S2.Np2;
end
clear S2;
S.RF55 = wiseMex('RF55_GetRxState()');

X = S.traceRxState;
Y.traceValid = bitget(X,1);
Y.State      = GetBitField(X, 3, 8);
Y.PktTime    = GetBitField(X, 9,24);
Y.phase80    = GetBitField(X,25,31);

X = S.traceRxControl;
Y.DW_PHYEnB     = bitget(X, 1);
Y.PHY_Sleep     = bitget(X, 2);
Y.MAC_TX_EN     = bitget(X, 3);
%PHY_TX_DONE
Y.PHY_CCA       = bitget(X, 5);
Y.PHY_RX_EN     = bitget(X, 6);
%PHY_RX_ABORT
Y.TxEnB         = bitget(X, 8);
Y.TxDone        = bitget(X, 9);
Y.RadioSleep    = bitget(X,10);
Y.RadioStandby  = bitget(X,11);
Y.RadioTX       = bitget(X,12);
Y.PAEnB         = bitget(X,13);
Y.AntTxEnB      = bitget(X,14);
Y.BGEnB         = bitget(X,15);
Y.DACEnB        = bitget(X,16);
Y.ADCEnB        = bitget(X,17);
Y.ModemSleep    = bitget(X,18);
Y.PacketAbort   = bitget(X,19);
Y.NuisanceCCA   = bitget(X,20);
Y.RestartRX     = bitget(X,21);
Y.DFEEnB        = bitget(X,22);
Y.DFERestart    = bitget(X,23);
Y.TurnOnOFDM    = bitget(X,24);
Y.TurnOnCCK     = bitget(X,25);
Y.NuisancePacket= bitget(X,26);
Y.ClearDCX      = bitget(X,27);
Y.bPathMux      = bitget(X,28);
Y.PHY_RX_ERROR  = bitget(X,29);
Y.PHY_DFS_INT   = bitget(X,30);

Y.AdvDly = S.AdvDly;

X = S.traceDataConv;
Y.ADCAOPM = GetBitField(X,0,1);
Y.ADCBOPM = GetBitField(X,2,3);
Y.DACOPM  = GetBitField(X,4,6);
Y.ADCFCAL = GetBitField(X,7,7);

%pause(0.1); % sometimes traceDFS has problems...don't know why, so I tried this
X = S.traceDFS;
X = X( 1:(length(S.traceDFS) - 1));
Y.DFS.CheckFCS = bitget(X, 6);
Y.DFS.FCSPass  = bitget(X, 7);
Y.DFS.InPacket = bitget(X, 8);
Y.DFS.RadarDet = bitget(X,10);
Y.DFS.Counter  = GetBitField(X,10,21);
Y.DFS.PulseSR  = GetBitField(X,22,31);

X = S.traceRadioIO;
Y.MC            = GetBitField(X,2,3);
Y.AGAIN         = GetBitField(X,4,9);
Y.LNAGAIN       = bitget(X,11);
Y.LNAGAIN2      = bitget(X,12);
Y.LGSIGAFE      = bitget(X,13);
Y.SW1           = bitget(X,18);
Y.SW1B          = bitget(X,19);
Y.SW2           = bitget(X,20);
Y.SW2B          = bitget(X,21);
Y.PA24          = bitget(X,22);
Y.PA50          = bitget(X,23);
Y.LNA24A        = bitget(X,26);
Y.LNA24B        = bitget(X,27);
Y.LNA50A        = bitget(X,28);
Y.LNA50B        = bitget(X,29);

if isfield(S.bRX,'traceControl')
    X = S.bRX.traceControl;
    Y.CEDone = bitget(X,10);
end

X = S.aSigOut;
aSigOut.SigDet = bitget(X,1);
aSigOut.AGCdone = bitget(X,2);
aSigOut.SyncFound = bitget(X,3);
aSigOut.HeaderValid = bitget(X,4);
aSigOut.RxFault = bitget(X,5);
aSigOut.StepUp = bitget(X,6);
aSigOut.StepDown = bitget(X,7);
aSigOut.EnergyDetect = bitget(X,8);
aSigOut.PulseDet = bitget(X,9);
aSigOut.ValidRSSI = bitget(X,10);
aSigOut.dValidRSSI = bitget(X,11);
aSigOut.RxDone = bitget(X,12);
aSigOut.PeakFound = bitget(X,13);
aSigOut.Run11b = bitget(X,14);
aSigOut.AntSel = bitget(X,15);
aSigOut.ChEstDone = bitget(X,17);
aSigOut.RX_EN = bitget(X,18);
aSigOut.TimeoutRIFS = bitget(X,19);

X = S.bSigOut;
bSigOut.SigDet = bitget(X,1);
bSigOut.AGCdone = bitget(X,2);
bSigOut.SyncFound = bitget(X,3);
bSigOut.HeaderValid = bitget(X,4);
bSigOut.StepUp = bitget(X,5);
bSigOut.StepDown = bitget(X,6);
bSigOut.EnergyDetect = bitget(X,7);
bSigOut.RxDone = bitget(X,8);
bSigOut.CSCCK = bitget(X,9);
bSigOut.AntSel = bitget(X,10);
bSigOut.PwrStepValid = bitget(X,11);
bSigOut.ValidRSSI = bitget(X,12);
bSigOut.RX_EN = bitget(X,13);

Y.aSigOut = aSigOut;
Y.bSigOut = bSigOut;

X = S.traceRSSI;
Y.RSSI0   = GetBitField(X, 0, 7);
Y.RSSI1   = GetBitField(X, 8,15);
Y.msrpwr0 = GetBitField(X,16,22);
Y.msrpwr1 = GetBitField(X,23,29);

k20 = 1 : (length(S.traceFrameSync)-1);
X = S.traceFrameSync(k20);
Y.FrameSync.FSEnB      = GetBitField(X, 0, 0);
Y.FrameSync.State      = GetBitField(X, 1, 8);
Y.FrameSync.RatioSig   = GetBitField(X, 9,19);
Y.FrameSync.RatioExp   = GetBitField(X,20,27,1);
Y.FrameSync.Ratio      = Y.FrameSync.RatioSig .* 2.^Y.FrameSync.RatioExp;
Y.FrameSync.SigDetValid= GetBitField(X,28,28);
Y.FrameSync.PeakFound  = GetBitField(X,29,29);
Y.FrameSync.AGCDone    = GetBitField(X,30,30);
Y.FrameSync.PeakSearch = GetBitField(X,31,31);

X = S.traceSigDet(k20);
Y.SigDet.SDEnB  = GetBitField(X, 0, 0);
Y.SigDet.State  = GetBitField(X, 1, 8);
Y.SigDet.SigDet = GetBitField(X, 9,10);
Y.SigDet.x      = GetBitField(X,11,30);
Y.SigDet.CheckSigDet = GetBitField(X,31,31);

X = S.traceDFE(k20);
Y.DFE.State = GetBitField(X, 2, 7);
Y.DFE.CCA1 = GetBitField(X, 13,13);
Y.DFE.CCA2 = GetBitField(X, 14,14);

X = S.DFEFlags(k20);
Y.DFE.InitGains   = bitget(X, 7);
Y.DFE.UpdateGains = bitget(X,10);

Y.N =length(aSigOut.ValidRSSI);
Y.Nb=length(bSigOut.ValidRSSI);

X = S.traceRX2(k20);
Y.RX2.CheckRotate       = bitget(X, 2);
Y.RX2.Rotated6Mbps      = bitget(X, 3);
Y.RX2.Rotated6MbpsValid = bitget(X, 4);
Y.RX2.Greenfield        = bitget(X, 5);
Y.RX2.Unsupported       = bitget(X, 6);

X = S.traceRx;
Y.traceRx.Markers = GetBitField(X, 1, 4);
Y.traceRx.Window  = GetBitField(X, 5, 5);
Y.traceRx.Windows = GetBitField(X, 6,13);
Y.traceRx.FCS     = -2*bitget(X,16) + bitget(X,15);

if(S.EnableTrace)
    X = S.bRX.traceState;
    bRX.State   = GetBitField(X, 0, 5);
    bRX.Counter = GetBitField(X, 6,16);
    bRX.CQPeak  = GetBitField(X,17,21);
    bRX.CQOut   = S.bRX.CQOut;
    bRX.EDOut   = S.bRX.EDOut;
    bRX.k80     = S.bRX.k80;
    
    X = S.bRX.traceCCK;
    bRX.DGain = GetBitField(X,23,27); %k=find(Z>=16); Z(k)=Z(k)-32; bRX.DGain=Z;
    Y.bRX = bRX;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function x = GetBitField(S, k0, k1, Signed)
if(nargin<4), Signed = 0; end
n = k1-k0+1;
d1 = 2^k0;
d2 = 2^n;
y = floor(S/d1);
x = rem(y,d2);
if(Signed)
    b = bitget(x,n);
    y = bitset(x,n,0);
    s = -2^(n-1) * b;
    x = s+y;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
