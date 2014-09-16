function varargout = NevadaPhyRegGUI(varargin)
% NEVADAPHYREGGUI M-file for NevadaPhyRegGUI.fig
%      NEVADAPHYREGGUI, by itself, creates aC new NEVADAPHYREGGUI or raises the existing
%      singleton*.
%
%      H = NEVADAPHYREGGUI returns the handle to aC new NEVADAPHYREGGUI or the handle to
%      the existing singleton*.
%
%      NEVADAPHYREGGUI('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in NEVADAPHYREGGUI.M with the given input arguments.
%
%      NEVADAPHYREGGUI('Property','Value',...) creates aC new NEVADAPHYREGGUI or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before NevadaPhyRegGUI_OpeningFunction gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to NevadaPhyRegGUI_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help NevadaPhyRegGUI

% Last Modified by GUIDE v2.5 06-Aug-2010 15:03:19

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @NevadaPhyRegGUI_OpeningFcn, ...
                   'gui_OutputFcn',  @NevadaPhyRegGUI_OutputFcn, ...
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

%------------------------------------------------
% --- Add text to Regsister Fields in UI Children
%------------------------------------------------
function AddTextToRegisterFields(handles, hObject)
C = get(hObject,'Children');
for i=1:length(C),
    h = C(i);
    try
        style = get(h,'Style');
        if(strcmp(style,'edit'))
            s = get(h,'String');
            if(isfield(handles.RegByField,s))
                set(h,'Tag',s);
                fprintf('%d, %s\n',isnumeric(s),s);
            end
        end
        tag   = get(h,'Tag');
        if(isfield(handles.RegByField,tag))
            P = get(h,'Position');
            if(strcmp(style,'edit'))
                P(1) = P(1) + P(3) + 0.5; P(2)=P(2)-0.3; P(3)=12+1.3*(length(tag)-8);
                h = uicontrol(hObject,'Style','text','String',tag,'HorizontalAlignment','Left','Units','Characters','Position',P);
            elseif(strcmp(style,'popupmenu'))
                P(2)=P(2)+P(4); P(3)=15; P(4)=1.15;
                h = uicontrol(hObject,'Style','text','String',tag,'HorizontalAlignment','Left','Units','Characters','Position',P);
            end
        end
    catch
        if(~isempty(get(h,'Children'))), AddTextToRegisterFields(handles,h); end
    end
end
guidata(hObject, handles)

%----------------------------------------------------------
% --- Executes just before NevadaPhyRegGUI is made visible.
%----------------------------------------------------------
function NevadaPhyRegGUI_OpeningFcn(hObject, eventdata, handles, varargin)

% Choose default command line output for NevadaPhyRegGUI
handles.output = hObject;
handles.ReadRegistersFn = @NevadaPhy_GetRegisters;
handles.WriteRegistersFn = @NevadaPhy_SetRegisters;
set(handles.ChangesPendingString,'String','');
set(handles.WriteOnChange,'Value',1);

% Setup callbacks
if nargin < 5 
    
    UseDwPhyLab = 0;
    
    if exist('DwPhyMex','file'),
        if exist('DwPhyLab_LocalDefaults','file'),
            param = DwPhyLab_Parameters;
            if isfield(param,'STA')
                if isfield(param.STA,'IP')
                    if ~isempty(param.STA.IP),
                        UseDwPhyLab = 1;
                    end
                end
            end
        end
    end
    if UseDwPhyLab,
        handles.ReadRegistersFn  = @DwPhyLab_ReadRegister;
        handles.WriteRegistersFn = @DwPhyLab_WriteRegister;
    else
        disp('NevadaRegGUI Running in Test Mode (wiseMex I/O)');
    end
    
elseif (length(varargin) == 3 && strcmpi(varargin{1},'RegisterReadWriteFns') && length(varargin{2})==2)
    %elseif (length(varargin) == 2 & strcmpi(varargin{1},'RegisterReadWriteFns'))
    fn = varargin{2};
    if(iscell(fn))
        handles.ReadRegistersFn = fn{1};
        handles.WriteRegistersFn = fn{2};
    else
        handles.ReadRegistersFn = fn(1);
        handles.WriteRegistersFn = fn(2);
    end
    set(handles.WriteOnChange,'Value',varargin{3});
else
    uiwait(errordlg('Invalid input parameter specification','NevadaRegGUI Error','modal'));
end	  
guidata(hObject, handles);
setptr(handles.NevadaPhyRegGUI,'watch');
[handles.RegByField, handles.ValidAddr] = NevadaPhy_RegisterMap;
handles.RegList = feval(handles.ReadRegistersFn);
guidata(hObject, handles)
UpdateRegisterList(handles);
SetParameterValues(handles);
setptr(handles.NevadaPhyRegGUI,'arrow');

% Add text to edit boxes
AddTextToRegisterFields(handles, handles.NevadaPhyRegGUI);
UpdateInfoTextBoxes(handles);
if(~isfield(handles,'MenuBar'))
    handles.MenuBar = RegGUI_MENU(handles.NevadaPhyRegGUI);
end
guidata(hObject, handles)

% --- Outputs from this function are returned to the command line.
function varargout = NevadaPhyRegGUI_OutputFcn(hObject, eventdata, handles)
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in aC future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;
varargout{2} = handles.RegList;

% --- Executes during object creation, after setting all properties.
function RegisterList_CreateFcn(hObject, eventdata, handles)
% hObject    handle to RegisterList (see GCBO)
% eventdata  reserved - to be defined in aC future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: listbox controls usually have aC white background on Windows.
%       See ISPC and COMPUTER.
if ispc
    set(hObject,'BackgroundColor','white');
else
    set(hObject,'BackgroundColor',get(0,'defaultUicontrolBackgroundColor'));
end

[handles.RegByField, handles.ValidAddr] = NevadaPhy_RegisterMap;
UpdateRegisterList(handles);
guidata(hObject, handles);

% --- Handle Parameter Values Changes
function ParameterChanged(hObject, handles)
if(get(handles.WriteOnChange,'Value'))
    feval(handles.WriteRegistersFn, handles.RegList);
    set(handles.ChangesPendingString,'String','');
else
    set(handles.ChangesPendingString,'String','Write Pending');
end
UpdateInfoTextBoxes(handles);
guidata(hObject, handles);

% --- Executes on selection change in RegisterList.
function RegisterList_Callback(hObject, eventdata, handles)
addr = get(hObject,'Value') - 1;
if(find(handles.ValidAddr==addr))
    k = find(handles.RegList(:,1)==addr);
    [button, a, d] = RegEditGUI('AddrData',[addr,handles.RegList(k,2)]);
    if(strcmp(button,'OK'))
        handles.RegList(k,2) = d;
        guidata(hObject, handles);
        UpdateRegisterList(handles);
        SetParameterValues(handles);
        ParameterChanged(hObject, handles);
    end
end

% --- Executes on selection of WriteRegisters
function WriteRegisters_Callback(hObject, eventdata, handles)
feval(handles.WriteRegistersFn, handles.RegList);
set(handles.ChangesPendingString,'String','');
guidata(hObject, handles);

% --- Executes on selection of ReadRegisters
function ReadRegisters_Callback(hObject, eventdata, handles)
setptr(handles.NevadaPhyRegGUI,'watch');
h = waitbar(0,'Reading Registers...','Name','Nevada','WindowStyle','modal');
handles.RegList = feval(handles.ReadRegistersFn);
if(ishandle(h)), waitbar(0.5,h); end
set(handles.ChangesPendingString,'String','');
guidata(hObject, handles);
UpdateRegisterList(handles);
SetParameterValues(handles);
if(ishandle(h)), close(h); end;
setptr(handles.NevadaPhyRegGUI,'arrow');

% --- Executes on selection of WriteOnChanges
function WriteOnChange_Callback(hObject, eventdata, handles)
if(get(handles.WriteOnChange,'Value'))
    feval(handles.WriteRegistersFn, handles.RegList);
    set(handles.ChangesPendingString,'String','');
end
guidata(hObject, handles);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% --- Write Contents of RegisterList
function UpdateRegisterList(handles)
if(~isfield(handles,'RegList')) return; end;
RegList = handles.RegList;
ListBoxTop = get(handles.RegisterList,'ListBoxTop');
N = length(handles.ValidAddr);
List = cell(N,1);
for addr=0:max(handles.ValidAddr),
    i = addr + 1;
    if(find(handles.ValidAddr==addr))
        k=find(RegList(:,1)==addr);
        if(k)
            List{i} = sprintf('%4d %4d (%s)',addr,RegList(k,2),dec2bin(RegList(k,2),8));
        else
            List{i} = sprintf('%4d UNDEFINED',addr);
        end
    else
        List{i} = sprintf('%4d    -  --------',addr);
    end
end
set(handles.RegisterList,'String',List);
set(handles.RegisterList,'ListBoxTop',ListBoxTop);
UpdateInfoTextBoxes(handles);


% --- Executes on selection change in Parameter Controls
function Parameter_Callback(hObject, eventdata, handles)
%fprintf('Parameter_Callback: %s\n',get(hObject,'Tag'));
handles = UpdateValue(hObject, eventdata, handles);
guidata(hObject, handles);
ParameterChanged(hObject, handles);

% --- If Enable == 'on', executes on mouse press in 5 pixel border.
% --- Otherwise, executes on mouse press in 5 pixel border or over RegisterList.
function RegisterList_ButtonDownFcn(hObject, eventdata, handles)


% --- Executes on mouse press over figure background, over aC disabled or
% --- inactive control, or over an axes background.
function NevadaPhyRegGUI_WindowButtonDownFcn(hObject, eventdata, handles)


% -------------------------------------------------------------------------
% FUNCTION: UpdateValue
% Retrieve the value/string from aC control and update the register list
% -------------------------------------------------------------------------
function handles = UpdateValue(hObject, eventdata, handles)
style = get(hObject,'Style');
if(strcmp(style,'edit'))
    d = str2double(get(hObject,'String'));
else
    d = get(hObject,'Value');
    if(strcmp(style,'popupmenu')), d=d-1; end;
end
if(isnumeric(d))
    tag = get(hObject,'Tag');
    if(isfield(handles.RegByField,tag))
        X = handles.RegByField.(tag);        % get parameter register field def.
        if(isfield(X,'Signed')), signed = X.Signed; else signed = 0; end
        d = min(max(d,X.min),X.max);         % limit input data to valid range
        A = X.Field(:,1);                    % address list
        if(signed)
            if(length(A)~=1) error('Cannot handled multi-byte signed fields'); end;
            L = X.Field(1,2) - X.Field(1,3) + 1;                  % length (bits)
            if(d<0), u  = d + 2^L; else u = d; end
            mask1 = sum(bitset(0,1+(X.Field(1,3):X.Field(1,2)))); % bits in Field
            mask0 = bitxor(255,mask1);                            % other bits in register
            k = find(handles.RegList(:,1)==A);                    % position of register
            D = bitand(handles.RegList(k,2), mask0);              % clear bits involved in parameter
            dm = bitand(bitshift(u,X.Field(1,3)),mask1);
            handles.RegList(k,2) = bitor(D, dm);                  % put new bits into register
        else
            for m=1:length(A),
                i = (m+1) : length(A);                            % fields after the mth
                L = (sum(X.Field(i,2)) - sum(X.Field(i,3))) + length(i); % number of LSBs before A(m)
                mask1 = sum(bitset(0,1+(X.Field(m,3):X.Field(m,2)))); % bits in Field
                mask0 = bitxor(255,mask1);                        % other bits in register
                k = find(handles.RegList(:,1)==A(m));             % position of register
                D = bitand(handles.RegList(k,2), mask0);          % clear bits involved in parameter
                dm = bitand(bitshift(bitshift(d,-L),X.Field(m,3)),mask1);
                handles.RegList(k,2) = bitor(D, dm);              % put new bits into register
                guidata(hObject, handles);
            end
        end
    end
end
guidata(handles.NevadaPhyRegGUI, handles);
UpdateRegisterList(handles);
switch(style)
    case 'edit',      set(hObject,'String',num2str(d));
    case 'popupmenu', set(hObject,'Value', d+1);
    otherwise,        set(hObject,'Value', d);
end

% --- Executes during object creation, after setting all properties.
function Popup_CreateFcn(hObject, eventdata, handles)
if ispc
    set(hObject,'BackgroundColor','white');
else
    set(hObject,'BackgroundColor',get(0,'defaultUicontrolBackgroundColor'));
end

% --- Set Parameter Controls to Defaults
function SetParameterValues(handles)
name = fieldnames(handles);
for i=1:length(name),
    if(isfield(handles.RegByField,name{i}))
        h = handles.(name{i});
        d = NevadaPhy_GetValueByField(handles.RegList, handles.RegByField, name{i});        
        style = get(h,'Style');
        switch(style)
            case 'edit',      set(h,'String',num2str(d));
            case 'popupmenu', set(h,'Value', d+1);
            case 'checkbox',  set(h,'Value', d);
            otherwise,        set(h,'Value', d);
        end
    end
end

% --- UpdateInfoTextBoxes
function UpdateInfoTextBoxes(handles)
k=find(handles.RegList(:,1)==1);
n = handles.RegList(k,2);
if(~isempty(n))
    switch(n)
        case  1, msg = 'Dakota 1/1b';
        case  2, msg = 'Dakota 2';
        case  3, msg = 'Dakota 2g';
        case  4, msg = 'Dakota 4';
        case  5, msg = 'Mojave';
        case  6, msg = 'Mojave b';
        case  7, msg = 'Nevada FPGA';
        case  8, msg = 'Nevada';
        otherwise, msg = 'Unknown Part';
    end
	set(handles.PartIDString,'String',msg);
end
k0=find(handles.RegList(:,1)==22); 
if(~isempty(k0))
    RSSI0 = round( 3/4*handles.RegList(k0,2) );
    RSSI1 = round( 3/4*handles.RegList(k0+1,2) );
    k=find(handles.RegList(:,1)==17);
    msg = [];
    i=0+1; msg{i} = sprintf('RSSI0 =% 4d dBm, RSSI1 =% 4d dBm',RSSI0-100,RSSI1-100);
    i=i+1; msg{i} = '';
    RxFault = bitget(handles.RegList(k,2),1);
    if(RxFault)
        i=i+1; msg{i} = sprintf('*** RX FAULT ***\n');
    else
       bRxMode = bitget(handles.RegList(k,2),8); 
       k=find(handles.RegList(:,1)==18);
       RATE = bitshift(handles.RegList(k,2),-4);
       LENGTH = 256*bitand(handles.RegList(k,2),15) + handles.RegList(k+1,2);
       SERVICE = 256*handles.RegList(k+2,2) + handles.RegList(k+3,2);
       if(bRxMode)
           SERVICE = NaN;
           Mbps = {'1 Mbps, Long','2 Mbps, Long','5.5 Mbps, Long','11 Mbps, Long','?','2 Mbps, Short','5.5 Mbps, Short','11 Mbps, Short','?','?','?','?','?','?','?','?'};
       else
           Mbps = {'?','48 Mbps','?','54 Mbps','?','12 Mbps','?','18 Mbps','?','24 Mbps','72 Mbps','36 Mbps','?','6 Mbps','?','9 Mbps'};
       end
       i=i+1; msg{i} = sprintf('Last RX: RATE    = %s',Mbps{RATE+1});
       i=i+1; msg{i} = sprintf('         LENGTH  = %d bytes',LENGTH);
       i=i+1; msg{i} = sprintf('         SERVICE = 0x%04X',SERVICE);
    end
    set(handles.RxInfo,'String',msg);
end
