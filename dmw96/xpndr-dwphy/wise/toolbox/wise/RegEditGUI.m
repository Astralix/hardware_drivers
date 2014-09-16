function varargout = RegEditGUI(varargin)
% REGEDITGUI M-file for RegEditGUI.fig
%      REGEDITGUI, by itself, creates a new REGEDITGUI or raises the existing
%      singleton*.
%
%      H = REGEDITGUI returns the handle to a new REGEDITGUI or the handle to
%      the existing singleton*.
%
%      REGEDITGUI('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in REGEDITGUI.M with the given input arguments.
%
%      REGEDITGUI('Property','Value',...) creates a new REGEDITGUI or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before RegEditGUI_OpeningFunction gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to RegEditGUI_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help RegEditGUI

% Last Modified by GUIDE v2.5 11-Feb-2003 14:59:46

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @RegEditGUI_OpeningFcn, ...
                   'gui_OutputFcn',  @RegEditGUI_OutputFcn, ...
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


% --- Executes just before RegEditGUI is made visible.
function RegEditGUI_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to RegEditGUI (see VARARGIN)

% Choose default command line output for RegEditGUI
handles.output = hObject;
handles.D = {'D0','D1','D2','D3','D4','D5','D6','D7'};

handles.addr = -1;
handles.data = 69;

if nargin < 4 
    disp('RegEditGUI Running in Test Mode (Dummy Data)');
elseif (length(varargin) == 2 && strcmpi(varargin{1},'AddrData') && ~isempty(varargin{2}==2))
    AddrData = varargin{2};
    handles.addr = AddrData(1);
    handles.data = AddrData(2);
else
    errordlg('RegEditGUI Error','Invalid parameter specification')
end	  

% Update handles structure
guidata(hObject, handles);

% Make the GUI modal
set(handles.figure1,'WindowStyle','modal')
set(handles.figure1,'Name',sprintf('Register %d',handles.addr));
Update(handles);

% UIWAIT makes RegEditGUI wait for user response (see UIRESUME)
uiwait(handles.figure1);

% --- Outputs from this function are returned to the command line.
function varargout = RegEditGUI_OutputFcn(hObject, eventdata, handles)
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
if(~isempty(handles))
    varargout{1} = handles.figure1;
    if(nargout==3)
        if(isstruct(handles))
            varargout{1} = handles.output;
            varargout{2} = handles.addr;
            varargout{3} = handles.data;

            % The figure can be deleted now
            delete(handles.figure1);
        else
            varargout{1} = 'CANCEL';
            varargout{2} = -1;
            varargout{3} = -1;
        end
    end
else
    if(nargout==3)
        varargout{1} = 'CANCEL';
        varargout{2} = -1;
        varargout{3} = -1;
    else
        varargout{1} = NaN;
    end
end

% --- Executes on button press in OK.
function OK_Callback(hObject, eventdata, handles)
handles.output = get(hObject,'String');
guidata(hObject, handles);
uiresume(handles.figure1);

% --- Executes on button press in CANCEL.
function CANCEL_Callback(hObject, eventdata, handles)
handles.output = get(hObject,'String');
guidata(hObject, handles);
uiresume(handles.figure1);


% --- Executes on button press in D?.
function D_Callback(hObject, eventdata, handles)
% Hint: get(hObject,'Value') returns toggle state of D?
b = get(hObject,'Value');
tag = get(hObject,'Tag');
k = str2num(tag(2));
handles.data = bitset(handles.data, k+1, b);
guidata(hObject, handles);
Update(handles);

% --- Update data values on GUI
function Update(handles)
for i=1:8,
    h = handles.(handles.D{i}); b = bitget(handles.data,i);
    set(h,'Value',b);   set(h,'String',num2str(b));
end
set(handles.Number,'String',handles.data);

% --- Executes when user attempts to close figure1.
function figure1_CloseRequestFcn(hObject, eventdata, handles)
disp('a');
if isequal(get(handles.figure1, 'waitstatus'), 'waiting')
    % The GUI is still in UIWAIT, us UIRESUME
    handles.output = 'CANCEL';
    uiresume(handles.figure1);
else
    % The GUI is no longer waiting, just close it
    delete(handles.figure1);
end
disp('b');