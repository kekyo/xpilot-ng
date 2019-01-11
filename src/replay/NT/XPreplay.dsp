# Microsoft Developer Studio Project File - Name="XPreplay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=XPreplay - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XPreplay.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XPreplay.mak" CFG="XPreplay - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XPreplay - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "XPreplay - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "XPreplay - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x413 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x413 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "XPreplay - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x413 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x413 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "XPreplay - Win32 Release"
# Name "XPreplay - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Properties.cpp
# End Source File
# Begin Source File

SOURCE=.\SliderBar.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\XPreplay.cpp
# End Source File
# Begin Source File

SOURCE=.\XPreplay.rc
# End Source File
# Begin Source File

SOURCE=.\XPreplayDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\XPreplayView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Properties.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SliderBar.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\XPreplay.h
# End Source File
# Begin Source File

SOURCE=.\XPreplayDoc.h
# End Source File
# Begin Source File

SOURCE=.\XPreplayView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Group "items"

# PROP Default_Filter "xpm"
# Begin Source File

SOURCE=..\..\client\items\itemAfterburner.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemArmor.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemAutopilot.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemCloakingDevice.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemDeflector.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemEcm.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemEmergencyShield.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemEmergencyThrust.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemEnergyPack.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemHyperJump.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemLaser.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemMinePack.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemMirror.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemPhasingDevice.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemRearShot.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemRocketPack.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemSensorPack.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemTank.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemTractorBeam.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemTransporter.xbm
# End Source File
# Begin Source File

SOURCE=..\..\client\items\itemWideangleShot.xbm
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\XPreplay.ico
# End Source File
# Begin Source File

SOURCE=.\res\XPreplay.rc2
# End Source File
# Begin Source File

SOURCE=.\res\XPreplayDoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\XPreplay.reg
# End Source File
# End Target
# End Project
