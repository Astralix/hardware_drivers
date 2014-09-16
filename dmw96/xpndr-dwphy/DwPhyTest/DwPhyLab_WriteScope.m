function DwPhyLab_WriteScope(h,msg)
%DwPhyLab_WriteScope(Handle, Message)
%
%   THIS FUNCTION IS DEPRECATED: USE DwPhyLab_SendCommand INSTEAD.

DwPhyLab_SendCommand(h, msg);

