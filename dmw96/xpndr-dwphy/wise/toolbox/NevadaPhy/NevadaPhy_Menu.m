function varargout = NevadaPhy_Menu(varargin)
% NevadaPhy_Menu
%
%      NevadaPhy_Menu('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in NevadaPhy_Menu.m with the given input arguments.

if nargin && ischar(varargin{1})
    fn = str2func(varargin{1});
    feval(fn, varargin{2}, varargin{3}, varargin{4});
elseif (nargin && ishandle(varargin{1}))
    varargout{1} = CreateMenu(varargin{1});        
end

%% FUNCTION: CreateMenu
function h = CreateMenu(fig)

%%% File Menu
h.File = uimenu(fig,'label','File');
%uimenu(h.File,'label','Save Registers',  'callback','NevadaPhy_Menu(''SaveRegisters_Callback'',gcbo,[],guidata(gcbo))');

uimenu(h.File,'label','Page Setup...',   'callback','NevadaPhy_Menu(''PageSetup_Callback'',gcbo,[],guidata(gcbo))','Separator','on');
uimenu(h.File,'label','Print Setup...',  'callback','NevadaPhy_Menu(''PrintSetup_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Print Preview...','callback','NevadaPhy_Menu(''PrintPreview_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Print...',        'callback','NevadaPhy_Menu(''Print_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Exit...',         'callback','NevadaPhy_Menu(''Exit_Callback'',gcbo,[],guidata(gcbo))','Separator','on');

%%% WiSE
h.WiSE = uimenu(fig, 'label','WiSE');

%%% Tools
h.Tools = uimenu(fig, 'label','Tools');
uimenu(h.Tools,'label','Register GUI','callback','NevadaPhy_Menu(''Tool_Callback'',gcbo,''NevadaPhyRegGUI'',guidata(gcbo))');
uimenu(h.Tools,'label','Trace Waveform Display','callback','NevadaPhy_Menu(''Tool_Callback'',gcbo,''NevadaPhyWaveform'',guidata(gcbo))');

%%% Window Menu
uimenu(fig, 'Label', 'Window', 'Callback', winmenu('callback'), 'Tag', 'winmenu');
winmenu(fig);

%%% Help Menu
h.Help = uimenu(fig,'label','Help');
%uimenu(h.Help,'label','Dakota2 Modem Registers','callback','NevadaPhy_Menu(''OpenPDF_Callback'',gcbo,''Dakota2Registers.pdf'',guidata(gcbo))');
uimenu(h.Help,'label','About','callback','NevadaPhy_Menu(''About_Callback'',gcbo,[],guidata(gcbo))','Separator','on');


%% Get Parent Handle ----------------------------------------------
function h = getParent(h)
while(get(h,'Parent'))
    h = get(h,'Parent');
end

%% MENU CALLBACK FUNCTIONS
function SaveRegisters_Callback(hObject, eventdata, handles)
% RegList = BERLAB_PHY_Registers;
% param = BERLAB_Parameters;
% if(isempty(RegList))
%     uiwait(warndlg('No registers written since MATLAB was loaded or last PowerDown','Save Registers','modal'));
% else
%     filename = sprintf('RegList-%s%s%s.txt',datestr(now,'yy'),datestr(now,'mm'),datestr(now,'dd'));
% 	[filename, pathname] = uiputfile(filename,'Save PHY Registers as');
% 	if(filename)
%         filename = strcat(pathname,filename);
%         BERLAB_SaveRegisters(filename, RegList);
% 	end
% end

% --------------------------------------------------------------------
function PageSetup_Callback(hObject, eventdata, handles)
pagesetupdlg(getParent(hObject));

% --------------------------------------------------------------------
function PrintPreview_Callback(hObject, eventdata, handles)
printpreview(getParent(hObject));

% --------------------------------------------------------------------
function PrintSetup_Callback(hObject, eventdata, handles)
printdlg('-setup',getParent(hObject));

% --------------------------------------------------------------------
function Print_Callback(hObject, eventdata, handles)
printdlg(getParent(hObject));

% --------------------------------------------------------------------
function Exit_Callback(hObject, eventdata, handles)
close(getParent(hObject));

% --------------------------------------------------------------------
function Tool_Callback(hObject, eventdata, handles)
feval(eventdata);

% --------------------------------------------------------------------
function About_Callback(hObject, eventdata, handles)
i=0+1; msg{i} = 'NevadaPhy WiSE Graphic User Interface';
i=i+1; msg{i} = 'Copyright (c) 2009 DSP Group, Inc. All Rights Reserved.';
msgbox(msg,sprintf('About %s',get(getParent(hObject),'name')),'modal');

% --------------------------------------------------------------------
function OpenPDF_Callback(hObject, eventdata, handles)
open(eventdata);

