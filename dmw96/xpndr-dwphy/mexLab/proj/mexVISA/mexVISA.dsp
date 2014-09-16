# Microsoft Developer Studio Project File - Name="mexVISA" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MEXVISA - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mexVISA.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mexVISA.mak" CFG="MEXVISA - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mexVISA - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MEXVISA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\etc\VXIPNP\WinNT\include" /I "C:\VXIPNP\WinNT\include" /I "c:\Program Files\VISA\WinNT\include" /I "h:\wise\source" /I "c:\Program Files\National Instruments\Languages\Microsoft C\\" /I "c:\vxipnp\winnt\include" /I "c:\Program Files\MATLAB\R2006b\extern\Include" /I "c:\Program Files\MATLAB\R2007a\extern\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MEXVISA_EXPORTS" /D "_DLL" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:2.0 /dll /pdb:none /machine:I386 /out:"..\..\mexVISA.mexw32"
# Begin Target

# Name "mexVISA - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\source\mexVISA.c
# End Source File
# Begin Source File

SOURCE=..\..\source\mexVISA.def
# End Source File
# Begin Source File

SOURCE=..\..\source\ViMexUtil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\source\mexVISA.h
# End Source File
# Begin Source File

SOURCE=..\..\source\ViMexUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\source\wiType.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\source\mexVISA.rc
# End Source File
# End Group
# Begin Source File

SOURCE=C:\etc\VXIPNP\WinNT\lib\msc\visa32.lib
# End Source File
# Begin Source File

SOURCE="C:\Program Files\MATLAB\R2007a\extern\lib\win32\microsoft\libmex.lib"
# End Source File
# Begin Source File

SOURCE="C:\Program Files\MATLAB\R2007a\extern\lib\win32\microsoft\libmx.lib"
# End Source File
# End Target
# End Project
