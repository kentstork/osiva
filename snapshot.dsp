# Microsoft Developer Studio Project File - Name="snapshot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=snapshot - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "snapshot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snapshot.mak" CFG="snapshot - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snapshot - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "snapshot - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "snapshot - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 jpeg6b_r.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/osiva.exe"

!ELSEIF  "$(CFG)" == "snapshot - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 jpeg6b_d.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "snapshot - Win32 Release"
# Name "snapshot - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\contour.cpp
# End Source File
# Begin Source File

SOURCE=.\cpscreen.cpp
# End Source File
# Begin Source File

SOURCE=.\ddproxy.cpp
# End Source File
# Begin Source File

SOURCE=.\dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\dragsrc.cpp
# End Source File
# Begin Source File

SOURCE=.\hotspot.cpp
# End Source File
# Begin Source File

SOURCE=.\iconbar.cpp
# End Source File
# Begin Source File

SOURCE=.\ooptions.cpp
# End Source File
# Begin Source File

SOURCE=.\readgif.cpp
# End Source File
# Begin Source File

SOURCE=.\readjpeg.c
# End Source File
# Begin Source File

SOURCE=.\reduce.cpp
# End Source File
# Begin Source File

SOURCE=.\resizer.cpp
# End Source File
# Begin Source File

SOURCE=.\rotate.cpp
# End Source File
# Begin Source File

SOURCE=.\snapshot.cpp
# End Source File
# Begin Source File

SOURCE=.\tooltip.cpp
# End Source File
# Begin Source File

SOURCE=.\wndmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\wregion.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cmdcodes.h
# End Source File
# Begin Source File

SOURCE=.\contour.h
# End Source File
# Begin Source File

SOURCE=.\ddproxy.h
# End Source File
# Begin Source File

SOURCE=.\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\hotspot.h
# End Source File
# Begin Source File

SOURCE=.\iconbar.h
# End Source File
# Begin Source File

SOURCE=.\ooptions.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\smlstr.h
# End Source File
# Begin Source File

SOURCE=.\snapshotw.h
# End Source File
# Begin Source File

SOURCE=.\tooltip.h
# End Source File
# Begin Source File

SOURCE=.\wndmgr.h
# End Source File
# Begin Source File

SOURCE=.\wregion.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\app.ico
# End Source File
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\error.gif
# End Source File
# Begin Source File

SOURCE=.\gif.ico
# End Source File
# Begin Source File

SOURCE=.\iconbar.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar2.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar2_md.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar2_mo.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar2_na.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar528.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar_md.gif
# End Source File
# Begin Source File

SOURCE=.\iconbar_mo.gif
# End Source File
# Begin Source File

SOURCE=.\index.cur
# End Source File
# Begin Source File

SOURCE=.\jpg.ico
# End Source File
# Begin Source File

SOURCE=.\osiva.gif
# End Source File
# Begin Source File

SOURCE=.\Pan.cur
# End Source File
# Begin Source File

SOURCE=.\PanMove.cur
# End Source File
# Begin Source File

SOURCE=.\snapshot.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\changes.txt
# End Source File
# Begin Source File

SOURCE=.\instructions.txt
# End Source File
# Begin Source File

SOURCE=.\notes.txt
# End Source File
# Begin Source File

SOURCE=.\scrap.txt
# End Source File
# End Target
# End Project
