function varargout = RegGUI_MENU(varargin)
% RegGUI_MENU
%
%      RegGUI_MENU('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in RegGUI_MENU.M with the given input arguments.
%

if nargin && isstr(varargin{1})
    fn = str2func(varargin{1});
    feval(fn, varargin{2}, varargin{3}, varargin{4});
elseif nargin && ishandle(varargin{1})
    varargout{1} = CreateMenu(varargin{1});        
end

%--- Define the Menu --------------------------
function h = CreateMenu(fig)

h.File = uimenu(fig,'label','File');
uimenu(h.File,'label','Load Registers',  'callback','RegGUI_MENU(''LoadRegisters_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Save Registers',  'callback','RegGUI_MENU(''SaveRegisters_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Page Setup...',   'callback','RegGUI_MENU(''PageSetup_Callback'',gcbo,[],guidata(gcbo))','Separator','on');
uimenu(h.File,'label','Print Setup...',  'callback','RegGUI_MENU(''PrintSetup_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Print Preview...','callback','RegGUI_MENU(''PrintPreview_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Print...',        'callback','RegGUI_MENU(''Print_Callback'',gcbo,[],guidata(gcbo))');
uimenu(h.File,'label','Exit...',         'callback','RegGUI_MENU(''Exit_Callback'',gcbo,[],guidata(gcbo))','Separator','on');

h.Registers = uimenu(fig,'label','Register');

%uimenu(fig, 'Label', 'Window', 'Callback', winmenu('callback'), 'Tag', 'winmenu');
%winmenu(fig);

h.Help = uimenu(fig,'label','Help');
g = uimenu(h.Help,'label','About','callback','RegGUI_MENU(''About_Callback'',gcbo,[],guidata(gcbo))','Separator','on');
%disp(get(g));

% --- Get Parent Handle ----------------------------------------------
function h = getParent(h)
while(get(h,'Parent'))
    h = get(h,'Parent');
end

% --------------------------------------------------------------------
% --- MENU CALLBACKS
% --------------------------------------------------------------------
function SaveRegisters_Callback(hObject, eventdata, handles)
if(isfield(handles,'RegList'))
	RegList = handles.RegList;
	if(isempty(RegList))
        uiwait(warndlg('Empty RegList','Save Registers','modal'));
	else
        filename = sprintf('RegList-%s%s%s.mat',datestr(now,'yy'),datestr(now,'mm'),datestr(now,'dd'));
		[filename, pathname] = uiputfile(filename,'Save Registers as');
		if(filename)
            filename = strcat(pathname,filename);
            eval(sprintf('save %s RegList',filename));
		end
	end
else
    uiwait(warndlg('No RegList parameter associate with this window.','Save Registers','modal'));
end

% --------------------------------------------------------------------
function LoadRegisters_Callback(hObject, eventdata, handles)
filename = sprintf('RegList-%s%s%s.mat',datestr(now,'yy'),datestr(now,'mm'),datestr(now,'dd'));
[filename, pathname] = uigetfile('*.mat','Load Registers from');
if(filename)
	filename = strcat(pathname,filename);
	S = load(filename);
    if(isfield(S,'RegList'))
        handles.RegList = S.RegList;
		guidata(hObject, handles);
		feval(handles.WriteRegistersFn, handles.RegList);
		handles.ChangesPending = 0;
		set(handles.ChangesPendingString,'String','');
		set(handles.WriteRegisters,'FontWeight','normal');
		guidata(hObject, handles);
        if(isfield(handles,'UpdateRegisterListFn'))
    		feval(handles.UpdateRegisterListFn,handles);
    		feval(handles.SetParameterValuesFn,handles);
    		guidata(hObject, handles);
        end
    end
end

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
function About_Callback(hObject, eventdata, handles)
msg{1} = 'Register GUI';
msg{2} = 'Written by Barrett Brickner.';
msg{3} = 'Copyright (c) 2003 Bermai, Inc., 2007 DSP Group, Inc.';
msg{4} = '';
msg{5} = sprintf('Parent Handle = %8.8f\n',getParent(hObject));
msgbox(msg,sprintf('About %s',get(getParent(hObject),'name')),'modal');
