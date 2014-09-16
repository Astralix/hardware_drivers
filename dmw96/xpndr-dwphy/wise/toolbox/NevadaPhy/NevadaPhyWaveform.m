function varargout = NevadaPhyWaveform(varargin)
% NevadaPhyWaveform -- Waveform display tool for the WiSE NevadaPHY model
%
%      To use the waveform display, process a transmit/receive operation with the
%      NevadaPHY configured with Nevada.EnableTrace = 1 and Nevada.EnableModelIO = 1.

% Written by Barrett Brickner
% Copyright 2009 DSP Group, Inc., All Rights Reserved.

%% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @NevadaPhyWaveform_OpeningFcn, ...
                   'gui_OutputFcn',  @NevadaPhyWaveform_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT

%% --- Executes just before NevadaPhyWaveform is made visible.
function NevadaPhyWaveform_OpeningFcn(hObject, eventdata, handles, varargin) %#ok<INUSL>
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to NevadaPhyWaveform (see VARARGIN)

% Choose default command line output for NevadaPhyWaveform
handles.output = hObject;

if(~isfield(handles,'MenuBar')) 
    handles.MenuBar = NevadaPhy_Menu(handles.NevadaPhyWaveform);
end

% Update handles structure
guidata(hObject, handles);

UpdateAll(handles);
DrawAll(handles);

Z = get(0,'ScreenSize');
P = get(handles.NevadaPhyWaveform,'Position');
P(2) = Z(4) - P(4);
set(handles.NevadaPhyWaveform,'Position',P);

% UIWAIT makes NevadaPhyWaveform wait for user response (see UIRESUME)
% uiwait(handles.NevadaPhyWaveform);


%% --- Outputs from this function are returned to the command line.
function varargout = NevadaPhyWaveform_OutputFcn(hObject, eventdata, handles)  %#ok<INUSL,INUSL>
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


%% --- Executes on selection change in Axes1Menu.
function AxesMenu_Callback(hObject, eventdata, handles) %#ok<INUSL,DEFNU>
switch get(hObject,'Tag')
    case 'Axes1Menu',                 DrawAxes1(handles);
    case {'Axes2Menu','Axes2YScale'}, DrawAxes2(handles);
    case 'Axes3Menu',                 DrawAxes3(handles);
end

%% --- Executes during object creation, after setting all properties.
function AxesMenu_CreateFcn(hObject, eventdata, handles) %#ok<INUSD,DEFNU>
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


%% UpdateAll
function UpdateAll(handles)

try
    oldY = getappdata(handles.PlotData,'Y');
catch
    oldY = [];
end

[S,Y] = NevadaPhy_GetTraceState;
if ~any(Y.traceValid)
    error('Unable to update: trace data is invalid; Nevada TX/RX may not have run');
end

k80 = 1:length(S.traceRxState);
k40 = 1:length(S.c);
k20 = 1:length(S.y);

if isfield(S,'TX') && isfield(S.TX,'TimingShift'),
    Y.dt = (S.TX.TimingShift + S.RX.PositionOffset/S.RX.Fs) * 1e6;
else
    Y.dt = 50.3165;
end

t20 = (k20-1)/20 - Y.dt;  Y.t20 = t20;
t40 = (k40-1)/40 - Y.dt;  Y.t40 = t40;
t80 = (k80-1)/80 - Y.dt;  Y.t80 = t80;
Y.t22 = Y.t80(Y.bRX.k80 + 1);

if isempty(oldY)
    Y.tMin = floor(min(t80));
    Y.tMax = ceil(max(t80));
else
    Y.tMin = oldY.tMin;
    Y.tMax = oldY.tMax;
end    

setappdata(handles.PlotData,'S',S);
setappdata(handles.PlotData,'Y',Y);

%% DrawAxes1
function DrawAxes1(handles)
axes(handles.axes1); cla; 
S = getappdata(handles.PlotData,'S');
Y = getappdata(handles.PlotData,'Y');

if (numel(S.RX.r) == 0) && get(handles.Axes1Menu,'Value') < 3,
    set(handles.Axes1Menu,'Value',3);
end    

switch get(handles.Axes1Menu,'Value'),

    case 1, % TX DAC Output
        k = 1:length(S.TX.d);
        plot(Y.t80(k), 1e3*imag(S.TX.d),'g', Y.t80(k), 1e3*real(S.TX.d));
        ylabel('DAC Output (mV)');
        YMax = 512; set(gca,'YTick',-500:250:500);
    
    case 2, % RX Input
        plot(Y.t80, imag(S.RX.r)/S.RX.Ar,'g', Y.t80, real(S.RX.r)/S.RX.Ar );
        ylabel('Radio Input (Normalized)');
        YMax = 2.1;
    
    case 3, % ADC Input
        plot(Y.t80,1000*imag(S.RX.sRX0),'g', Y.t80,1000*real(S.RX.sRX0));
        ylabel('Radio RXA Output (mV)');
        YMax = 1000;

    case 4, % ADC Output (A)
        plot(Y.t40,imag(S.r(:,1)),'g', Y.t40,real(S.r(:,1))); 
        YMax = 512; set(gca,'YTick',-500:250:500);

    case 5, % ADC Output (B)
        plot(Y.t40,imag(S.r(:,2)),'g', Y.t40,real(S.r(:,2))); 
        YMax = 512; set(gca,'YTick',-500:250:500);

    case 6, % ADC Output (A+B)
        plot(Y.t40,real(S.r(:,1)),Y.t40,real(S.r(:,2)),'r'); 
        YMax = 512; set(gca,'YTick',-500:250:500);

    case 7, % ADC + DCX Output
        plot(Y.t40,real(S.r(:,1)),Y.t40,real(S.c(:,1)),'r'); grid on; 
        YMax = 512; set(gca,'YTick',-500:250:500);

    case 8, % ADC + LPF(OFDM) Output
        plot(Y.t40,real(S.r(:,1)),Y.t20,real(S.y(:,1))/2,'r'); grid on; 
        YMax = 650; set(gca,'YTick',-500:250:500);

    case 9, % ADC + DigitalAmp Output
        plot(Y.t40,real(S.r(:,1)),Y.t20,real(S.s(:,1)),'r'); grid on; 
        YMax = 650; set(gca,'YTick',-500:250:500);
end

axis([Y.tMin Y.tMax -YMax YMax]);
set(gca,'XTickLabel',{''});
grid on;

%% DrawAxes2
function DrawAxes2(handles)
axes(handles.axes2); cla; 

Y = getappdata(handles.PlotData,'Y');
t80 = Y.t80;
RSSI0 = 0.75 * Y.RSSI0(2:length(Y.RSSI0));

switch get(handles.Axes2Menu,'Value'),
    
    case 1, % TopState
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime); 
        L = legend('MC','State','PktTime');
        
    case 2, % GAIN
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN); 
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN');
        
    case 3,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.DFE.State); 
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','DFEState');
        
    case 4,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.DFE.State,Y.t20,RSSI0); 
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','DFEState','RSSI0(dB)');
        
    case 5,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State);
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','FrontCtrl');
        
    case 6,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State,Y.t20,RSSI0);
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','FrontCtrl','RSSI0(dB)');

    case 7,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t22,Y.bRX.State,Y.t22,Y.bRX.DGain);
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','FrontCtrl','bDGain');
        
    case 8,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,Y.t22,10*log10(Y.bRX.EDOut),Y.t22,10*log10(Y.bRX.CQOut),Y.t22,Y.bRX.State,Y.t22,Y.bRX.DGain);
        L = legend('MC','State','PktTime','EDOut (dB)','CQOut (dB)','FrontCtrl','bDGain');
        
    case 9,
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.DACOPM+0.02,t80,Y.ADCAOPM+0.04,t80,Y.ADCBOPM+0.06);
        L = legend('MC','State','PktTime','AGAIN','DACOPM','ADCAOPM','ADCBOBM');
        
    case 10, % OFDM Signal Detect
        plot(t80,Y.MC,t80,Y.AGAIN,Y.t20,Y.SigDet.State,Y.t20,Y.SigDet.x,Y.t20,Y.SigDet.SDEnB,Y.t20,Y.SigDet.SigDet);
        L = legend('MC','AGAIN','SigDet State','x','SDEnB','SigDet');
        
    case 11, % FrameSync
        plot(t80,Y.MC,t80,Y.AGAIN,Y.t20,Y.FrameSync.State,Y.t20,Y.FrameSync.SigDetValid,Y.t20,Y.FrameSync.PeakFound,Y.t20,100*Y.FrameSync.Ratio);
        L = legend('MC','AGAIN','FrameSync State','SigDetValid','PeakFound','FrameSyncRatio,%');
        
    case 12, % DFS/Radar Detect
        plot(t80,Y.MC,t80,Y.State,t80,Y.PktTime,t80,Y.AGAIN,t80,Y.LNAGAIN,Y.t20,Y.DFS.Counter,Y.t20,Y.DFS.PulseSR); 
        L = legend('MC','State','PktTime','AGAIN','LNAGAIN','DFS Counter','PulseSR');
end

switch get(handles.Axes2YScale,'Value'),
    case  1, Ymin =  0; Ymax =   8; dY =  1;
    case  2, Ymin =  0; Ymax =  16; dY =  2;
    case  3, Ymin =  0; Ymax =  32; dY =  4;
    case  4, Ymin =  0; Ymax =  40; dY =  4;
    case  5, Ymin =  0; Ymax =  64; dY =  8;
    case  6, Ymin =  0; Ymax =  96; dY =  8;
    case  7, Ymin =  0; Ymax = 100; dY = 10;
    case  8, Ymin =  0; Ymax = 200; dY = 20;
    case  9, Ymin =  0; Ymax = 400; dY = 40;
    case 10, Ymin =  0; Ymax =1000; dY =100;
    case 11, Ymin =  0; Ymax =4096; dY =512;
    case 12, Ymin = 16; Ymax =  32; dY =  2;
    case 13, Ymin = 32; Ymax =  48; dY =  2;
    case 14, Ymin = 32; Ymax =  64; dY =  4;
    case 15, Ymin = 48; Ymax =  64; dY =  2;
    otherwise, Ymax = 40; dY = 4;
end

axis([Y.tMin Y.tMax Ymin Ymax]);
set(gca,'YTick',Ymin:dY:Ymax);
set(gca,'XTickLabel',{''});
set(get(gca,'Children'),'LineWidth',2);
grid on;

PA = get(gca,'Position');
PL = get( L, 'Position');
set(L,'Position',[PA(1)+PA(3)+0.005 PA(2) PL(3) PL(4)]);

%% DrawAxes3
function DrawAxes3(handles)
axes(handles.axes3); cla; 
Y = getappdata(handles.PlotData,'Y');

t20 = Y.t20;
t80 = Y.t80;
t22 = Y.t80(Y.bRX.k80+1);

t1 = Y.tMin;
t2 = Y.tMax;
N =length(Y.aSigOut.ValidRSSI); k = 2:N;
Nb=length(Y.bSigOut.ValidRSSI); kb= 2:Nb-3;

switch get(handles.Axes3Menu,'Value'),
    case 1, % Sleep/Wake
        plot(t80,Y.DW_PHYEnB+0, t80,Y.PHY_Sleep+2, t80,Y.RadioSleep+4, t80,Y.RadioStandby+6, t80,Y.AntTxEnB+8, t80,Y.BGEnB+10, t80,Y.ADCEnB+12, t80,Y.ModemSleep+14, t80,Y.DFEEnB+16); grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'DW_PHYEnB','PHY_Sleep','RadioSleep','RadioStandby','AntTxEnB','BGEnB','ADCEnB','ModemSleep','DFEEnB'});
        axis([t1 t2 0 18]);
    case 2, % MAC/PHY
        plot(t80,Y.DW_PHYEnB+0, t80,Y.PHY_Sleep+2, t80,Y.PHY_CCA+4, t80,Y.PHY_RX_EN+6, t80,Y.PHY_RX_ERROR+8, t80,Y.PacketAbort+10, t80,Y.MAC_TX_EN+12, t80,Y.TxDone+14, t80,Y.PHY_DFS_INT+16); grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'DW_PHYEnB','PHY_Sleep','PHY_CCA','PHY_RX_EN','PHY_RX_ERROR','PacketAbort','MAC_TX_EN','TxDone','PHY_DFS_INT'});
        axis([t1 t2 0 18]);
    case 3, % TX
        plot(t80,Y.PHY_CCA+0, t80,Y.MAC_TX_EN+2, t80,Y.TxEnB+4, t80,Y.TxDone+6, t80,Y.DACEnB+8, t80,Y.SW1+10, t80,Y.PA24+12, t80,Y.PA50+14); grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','MAC_TX_EN','TxEnB','TxDone','DACEnB','SW1','PA24','PA50'});
        axis([t1 t2 0 16]);
    case 4, % RX
        plot(t80,Y.PHY_CCA+0, t80,Y.PHY_RX_EN+2, t20,Y.aSigOut.ValidRSSI(k)+4, t80,Y.ClearDCX+6, t20,Y.aSigOut.AGCdone(k)+8, t20,Y.aSigOut.HeaderValid(k)+10, t20,Y.aSigOut.RxDone(k)+12, t80,Y.traceRx.FCS+14, t80,Y.ADCFCAL+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_CCA','PHY_RX_EN','ValidRSSI','ClearDCX','AGCdone','HeaderValid','RxDone','FCS','ADCFCAL'});
        axis([t1 t2 0 18]);
    case 5, % OFDM-RX1
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.SigDet(k)+6, t20,Y.aSigOut.AGCdone(k)+8, t20,Y.aSigOut.PeakFound(k)+10,t20,Y.aSigOut.HeaderValid(k)+12,t20,Y.aSigOut.RxDone(k)+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','aSigDet','AGCdone','PeakFound','HeaderValid','RxDone'});
        axis([t1 t2 0 16]);
    case 6, % OFDM-RX2
        plot(t80,Y.PHY_RX_EN+0, t20,Y.aSigOut.HeaderValid(k)+2, t20,Y.RX2.CheckRotate+4, t20,Y.RX2.Rotated6Mbps+6, t20,Y.RX2.Rotated6MbpsValid+8, t20,Y.RX2.Greenfield+10, t20,Y.RX2.Unsupported+12, t20,Y.aSigOut.RxFault(k)+14, t20,Y.AdvDly+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_RX_EN','HeaderValid','CheckRotate','Rotated6Mbps','Rotated6MbpsValid','Greenfield','Unsupproted','RxFault','AdvDly'});
        axis([t1 t2 0 18]);
    case 7, % DSSS/CCK-RX
        plot(t80,Y.PHY_CCA+0, t22,Y.bSigOut.ValidRSSI(kb)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.PulseDet(k)+6, t22,Y.bSigOut.StepUp(kb)+8, t22,Y.bSigOut.StepDown(kb)+10, t22,Y.CEDone+12, t22,Y.bSigOut.RxDone(kb)+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','PulseDet','bStepUp','bStepDown','CEDone','bRxDone'});
        axis([t1 t2 0 16]);
    case 8, % OFDM/DSSS Arbitration
        plot(t20,Y.aSigOut.SigDet(k)+0, t22,Y.bSigOut.SigDet(kb)+2, t20,Y.aSigOut.AGCdone(k)+4, t20,Y.FrameSync.FSEnB+6, t20,Y.aSigOut.PeakFound(k)+8, t20,Y.aSigOut.Run11b(k)+10, t80,Y.TurnOnOFDM+12, t80,Y.TurnOnCCK+14, t80,Y.bPathMux+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'aSigDet','bSigDet','AGCdone','FSEnB','PeakFound','Run11b','TurnOnOFDM','TurnOnCCK','bPathMux'});
        axis([t1 t2 0 18]);
    case 9, % AGC
        S = getappdata(handles.PlotData,'S');
        wPeakDet = bitget(S.traceAGC,23);
        wPeakDet = wPeakDet(1:(length(wPeakDet)-1));
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.AGCdone(k)+6, t20,Y.DFE.InitGains+8, t20,Y.DFE.UpdateGains+10, t80,Y.LGSIGAFE+12, t80,Y.LNAGAIN+14, t80,Y.LNAGAIN2+16, t20,wPeakDet+18);
        grid on;
        set(gca,'YTick',0:2:18); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','AGCdone','InitGains','UpdateGains','LGSIGAFE','LNAGAIN','LNAGAIN2','wPeakDet'});
        axis([t1 t2 0 20]);
    case 10, % CCA
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.SigDet(k)+2, t20,Y.DFE.CCA1+4, t20,Y.DFE.CCA2+6, t20,Y.aSigOut.ValidRSSI(k)+8, t80,Y.ClearDCX+10, t20,Y.DFE.InitGains+12, t20,Y.DFE.UpdateGains+14, t80,(Y.PktTime>0)+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_CCA','aSigDet','CCA1','CCA2','ValidRSSI','ClearDCX','InitGains','UpdateGains','PktTime>0'});
        axis([t1 t2 0 18]);
    case 11, % External LNA
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.AGCdone(k)+6, t80,Y.LNAGAIN2+8, t80,Y.LNA24A+10, t80,Y.LNA24B+12,t80,Y.LNA50A+14,t80,Y.LNA50B+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','AGCdone','LNAGAIN2','LNA24A','LNA24B','LNA50A','LNA50B'});
        axis([t1 t2 0 18]);
    case 12, % RX Restart
        plot(t80,Y.PHY_CCA+0, t80,Y.PHY_RX_EN+2, t80,Y.DFEEnB+4, t80,Y.DFERestart+6, t80,Y.RestartRX+8, t20,Y.DFE.InitGains+10, t80,Y.ClearDCX+12, t20,Y.SigDet.CheckSigDet+14, t20,Y.aSigOut.SigDet(k)+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_CCA','PHY_RX_EN','DFEEnB','DFERestart','RestartRX','InitGains','ClearDCX','CheckSigDet','aSigDet'});
        axis([t1 t2 0 18]);
    case 13, % Step-Up/Down Restart
        plot(t80,Y.PHY_CCA+0, t80,Y.RestartRX+2, t80,Y.DFERestart+4, t20,Y.aSigOut.StepUp(k)+6, t20,Y.aSigOut.StepDown(k)+8, t20,Y.aSigOut.SigDet(k)+10, t22,Y.bSigOut.SigDet(kb)+12, t80,Y.DW_PHYEnB+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'PHY_CCA','RestartRX','DFERestart','aStepUp','aStepDown','aSigDet','bSigDet','DW_PHYEnB'});
        axis([t1 t2 0 16]);
    case 14, % Antenna Selection
        plot(t80,Y.MAC_TX_EN+0, t80,Y.SW1+2, t80,Y.SW1B+4, t80,Y.SW2+6, t80,Y.SW2B+8, t20,Y.aSigOut.AntSel(k)+10, t80,Y.ClearDCX+12, t80,Y.DW_PHYEnB+14); grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'MAC_TX_EN','SW1','SW1B','SW2','SW2B','AntSel','ClearDCX','DW_PHYEnB'});
        axis([t1 t2 0 16]);
    case 15, % DFS
        plot(t80,Y.PHY_CCA+0, t20,Y.aSigOut.ValidRSSI(k)+2, t80,Y.ClearDCX+4, t20,Y.aSigOut.PulseDet(k)+6, t20,Y.DFS.RadarDet+8, t20,Y.aSigOut.HeaderValid(k)+10, t20,Y.DFS.FCSPass+12, t20,Y.DFS.CheckFCS+14, t20,Y.DFS.InPacket+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'PHY_CCA','ValidRSSI','ClearDCX','PulseDet','RadarDet','HeaderValid','FCSPass','CheckFCS','InPacket'});
        axis([t1 t2 0 18]);
    case 16, % TxTrace
        S = getappdata(handles.PlotData,'S');
        X = S.TX.traceTx;
        t = t80(1:length(X));
        plot(t80,Y.MAC_TX_EN+0, t80,Y.TxDone+2, t,bitget(X,1)+4, t,bitget(X,2)+6, t,bitget(X,3)+8, t,bitget(X,4)+10, t,bitget(X,9)+12, t,bitget(X,10)+14);
        grid on;
        set(gca,'YTick',0:2:14); set(gca,'YTickLabel',{'MAC_TX_EN','TxDone','TxTrace.Marker1','TxTrace.Marker2','TxTrace.Marker3','TxTrace.Marker4','TxTrace.PktStart','TxTrace.PktEnd'});
        axis([t1 t2 0 16]);
    case 17, % RxTrace
        for i=1:8, W{i} = bitget(Y.traceRx.Windows,i); end %#ok<AGROW>
        plot(t80,Y.traceRx.Window, t80,W{1}+2, t80,W{2}+4, t80,W{3}+6, t80,W{4}+8, t80,W{5}+10, t80,W{6}+12, t80,W{7}+14, t80,W{8}+16);
        grid on;
        set(gca,'YTick',0:2:16); set(gca,'YTickLabel',{'Window','Window1','Window2','Window3','Window4','Window5','Window6','Window7','Window8'});
        axis([t1 t2 0 18]);
end

xlabel('Time (\mus)'); 
c = get(gca,'Children');
set(c,'LineWidth',2);
%p = get(gca,'Position'); set(gca,'Position',[p1 p(2) p3 p4]);
set(get(gca,'Children'),'LineWidth',2);


%% DrawAll
function DrawAll(handles)
DrawAxes1(handles);
DrawAxes2(handles);
DrawAxes3(handles);


%% PlotData Callback
function PlotData_Callback(hObject, eventdata, handles) %#ok<INUSL,DEFNU>
UpdateAll(handles);
DrawAll(handles);

%% TimeShiftZoom_Callback
function TimeShiftZoom_Callback(hObject, eventdata, handles) %#ok<INUSL>
Y = getappdata(handles.PlotData,'Y');
T = Y.tMax - Y.tMin;

if ishandle(hObject),
    Key = get(hObject,'Tag');
else
    Key = hObject;
end

switch Key,
    
    case {'ZoomIn','+','equal','uparrow',30},
        Y.tMin = round(10*Y.tMin + T)/10;
        Y.tMax = round(10*Y.tMax - T)/10;
        
    case {'ZoomIn2','pageup'},
        Y.tMin = round(10*Y.tMin + 2.5*T)/10;
        Y.tMax = round(10*Y.tMax - 2.5*T)/10;
        
    case {'ZoomOut','-','hyphen','downarrow',31},
        Y.tMin = round(10*Y.tMin - T)/10;
        Y.tMax = round(10*Y.tMax + T)/10;
    
    case {'ZoomOut2','pagedown'},
        Y.tMin = round(10*Y.tMin - 2.5*T)/10;
        Y.tMax = round(10*Y.tMax + 2.5*T)/10;
    
    case {'FullLeft','home'},
        Y.tMin = round(10*min(Y.t80))/10;
        Y.tMax = Y.tMin + T;
        
    case {'StepLeft','leftarrow','comma','<',28}
        Y.tMin = round(10*Y.tMin - T/2)/10;
        Y.tMax = round(10*Y.tMax - T/2)/10;

    case {'Center','0'},
        Y.tMin = round(-10*T/2)/10;
        Y.tMax = round(+10*T/2)/10;
        
    case {'StepRight','rightarrow','period','>',29}
        Y.tMin = round(10*Y.tMin + T/2)/10;
        Y.tMax = round(10*Y.tMax + T/2)/10;

    case {'FullRight','end'}
        Y.tMax = round(10*max(Y.t80))/10;
        Y.tMin = Y.tMax - T;
        
    case {'ResetTime'},
        Y.tMin = floor(min(Y.t80));
        Y.tMax = ceil(max(Y.t80));
        
        
    otherwise, fprintf('Unhandled Callback with Key = "%g"\n',Key);
end
setappdata(handles.PlotData,'Y',Y);
DrawAll(handles);

%% Keypress_Callback
function Keypress_Callback(hObject, eventdata, handles) %#ok<INUSL,DEFNU>
Key = get(handles.NevadaPhyWaveform,'CurrentCharacter');

switch Key,
    
    case { '+','-','rightarrow','leftarrow','uparrow','downarrow','pageup','pagedown',...
           'home','end','0','equal','hyphen','period','comma','<','>', ...
           28, 29, 30, 31},
        TimeShiftZoom_Callback(Key, [], handles);
        
end

%% REVISIONS
% 091221-BB: Limit Axes1 selection to 3 if radio input is not valid (for waveform capture)
