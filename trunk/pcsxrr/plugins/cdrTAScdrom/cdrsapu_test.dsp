# Microsoft Developer Studio Project File - Name="cdrsapu_test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=cdrsapu_test - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cdrsapu_test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cdrsapu_test.mak" CFG="cdrsapu_test - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cdrsapu_test - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "cdrsapu_test - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cdrsapu_test - Win32 Release"

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
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX- /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FD /c
# SUBTRACT CPP /Z<none> /YX
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "cdrsapu_test - Win32 Debug"

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
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX- /Z7 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /map

!ENDIF 

# Begin Target

# Name "cdrsapu_test - Win32 Release"
# Name "cdrsapu_test - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\cache.cpp
# End Source File
# Begin Source File

SOURCE=.\cdrsapu.rc
# End Source File
# Begin Source File

SOURCE=.\if_aspi.cpp
# End Source File
# Begin Source File

SOURCE=.\if_defs.cpp
# End Source File
# Begin Source File

SOURCE=.\if_mscd.cpp
# End Source File
# Begin Source File

SOURCE=.\if_spti.cpp
# End Source File
# Begin Source File

SOURCE=.\iso_fs.cpp
# End Source File
# Begin Source File

SOURCE=.\plg_defs.cpp
# End Source File
# Begin Source File

SOURCE=.\plg_fpse.cpp
# End Source File
# Begin Source File

SOURCE=.\plg_main.cpp
# End Source File
# Begin Source File

SOURCE=.\plg_psemu.cpp
# End Source File
# Begin Source File

SOURCE=.\scsi_cdb.cpp
# End Source File
# Begin Source File

SOURCE=.\scsi_cmds.cpp
# End Source File
# Begin Source File

SOURCE=.\scsi_utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\iso_fs.h
# End Source File
# Begin Source File

SOURCE=.\Locals.h
# End Source File
# Begin Source File

SOURCE=.\mscdex.h
# End Source File
# Begin Source File

SOURCE=.\Ntddcdrm.h
# End Source File
# Begin Source File

SOURCE=.\ntddmmc.h
# End Source File
# Begin Source File

SOURCE=.\Ntddscsi.h
# End Source File
# Begin Source File

SOURCE=.\Ntddstor.h
# End Source File
# Begin Source File

SOURCE=".\PSEmu Plugin Defs.h"
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Scsi.h
# End Source File
# Begin Source File

SOURCE=.\Scsidefs.h
# End Source File
# Begin Source File

SOURCE=.\Sdk.h
# End Source File
# Begin Source File

SOURCE=.\Type.h
# End Source File
# Begin Source File

SOURCE=.\Win32def.h
# End Source File
# Begin Source File

SOURCE=.\Winioctl.h
# End Source File
# Begin Source File

SOURCE=.\Wnaspi32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\asynctrig.bmp
# End Source File
# End Group
# End Target
# End Project
