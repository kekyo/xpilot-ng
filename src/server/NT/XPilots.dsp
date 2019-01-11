# Microsoft Developer Studio Project File - Name="XPilotServer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=XPilotServer - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xpilots.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xpilots.mak" CFG="XPilotServer - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XPilotServer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "XPilotServer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "XPilotServer - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /I "..\..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_XPILOTNTSERVER_" /D "_AFXDLL" /D "_MBCS" /Fr /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /out:".\Release\XPilotServer.exe"
# SUBTRACT LINK32 /debug /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\XPilotServer.exe C:\XPilot
# End Special Build Tool

!ELSEIF  "$(CFG)" == "XPilotServer - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_XPILOTNTSERVER_" /D "_AFXDLL" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /out:".\Debug\XPilotServer.exe"

!ENDIF 

# Begin Target

# Name "XPilotServer - Win32 Release"
# Name "XPilotServer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "server"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\alliance.c
# End Source File
# Begin Source File

SOURCE=..\asteroid.c
# End Source File
# Begin Source File

SOURCE=..\asteroid.h
# End Source File
# Begin Source File

SOURCE=..\cannon.c
# End Source File
# Begin Source File

SOURCE=..\cannon.h
# End Source File
# Begin Source File

SOURCE=..\cell.c
# End Source File
# Begin Source File

SOURCE=..\click.h
# End Source File
# Begin Source File

SOURCE=..\cmdline.c
# End Source File
# Begin Source File

SOURCE=..\collision.c
# End Source File
# Begin Source File

SOURCE=..\command.c
# End Source File
# Begin Source File

SOURCE=..\contact.c
# End Source File
# Begin Source File

SOURCE=..\defaults.h
# End Source File
# Begin Source File

SOURCE=..\event.c
# End Source File
# Begin Source File

SOURCE=..\fileparser.c
# End Source File
# Begin Source File

SOURCE=..\frame.c
# End Source File
# Begin Source File

SOURCE=..\global.h
# End Source File
# Begin Source File

SOURCE=..\id.c
# End Source File
# Begin Source File

SOURCE=..\item.c
# End Source File
# Begin Source File

SOURCE=..\laser.c
# End Source File
# Begin Source File

SOURCE=..\map.c
# End Source File
# Begin Source File

SOURCE=..\map.h
# End Source File
# Begin Source File

SOURCE=..\metaserver.c
# End Source File
# Begin Source File

SOURCE=..\metaserver.h
# End Source File
# Begin Source File

SOURCE=..\netserver.c
# End Source File
# Begin Source File

SOURCE=..\netserver.h
# End Source File
# Begin Source File

SOURCE=..\object.c
# End Source File
# Begin Source File

SOURCE=..\object.h
# End Source File
# Begin Source File

SOURCE=..\objpos.c
# End Source File
# Begin Source File

SOURCE=..\objpos.h
# End Source File
# Begin Source File

SOURCE=..\option.c
# End Source File
# Begin Source File

SOURCE=..\parser.c
# End Source File
# Begin Source File

SOURCE=..\play.c
# End Source File
# Begin Source File

SOURCE=..\player.c
# End Source File
# Begin Source File

SOURCE=..\proto.h
# End Source File
# Begin Source File

SOURCE=..\robot.c
# End Source File
# Begin Source File

SOURCE=..\robot.h
# End Source File
# Begin Source File

SOURCE=..\robotdef.c
# End Source File
# Begin Source File

SOURCE=..\rules.c
# End Source File
# Begin Source File

SOURCE=..\saudio.c
# End Source File
# Begin Source File

SOURCE=..\saudio.h
# End Source File
# Begin Source File

SOURCE=..\sched.c
# End Source File
# Begin Source File

SOURCE=..\sched.h
# End Source File
# Begin Source File

SOURCE=..\score.c
# End Source File
# Begin Source File

SOURCE=..\score.h
# End Source File
# Begin Source File

SOURCE=..\server.c
# End Source File
# Begin Source File

SOURCE=..\server.h
# End Source File
# Begin Source File

SOURCE=..\ship.c
# End Source File
# Begin Source File

SOURCE=..\shot.c
# End Source File
# Begin Source File

SOURCE=..\showtime.c
# End Source File
# Begin Source File

SOURCE=..\tuner.c
# End Source File
# Begin Source File

SOURCE=..\tuner.h
# End Source File
# Begin Source File

SOURCE=..\update.c
# End Source File
# Begin Source File

SOURCE=..\walls.c
# End Source File
# Begin Source File

SOURCE=..\walls.h
# End Source File
# Begin Source File

SOURCE=..\wildmap.c
# End Source File
# End Group
# Begin Group "serverNT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ConfigDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigDlg.h
# End Source File
# Begin Source File

SOURCE=.\ExitXpilots.cpp
# End Source File
# Begin Source File

SOURCE=.\ExitXpilots.h
# End Source File
# Begin Source File

SOURCE=.\ReallyShutdown.cpp
# End Source File
# Begin Source File

SOURCE=.\ReallyShutdown.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UrlWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\UrlWidget.h
# End Source File
# Begin Source File

SOURCE=.\winServer.h
# End Source File
# Begin Source File

SOURCE=.\winSvrThread.c
# End Source File
# Begin Source File

SOURCE=.\WinSvrThread.h
# End Source File
# Begin Source File

SOURCE=.\xpilots.cpp
# End Source File
# Begin Source File

SOURCE=.\xpilots.h
# End Source File
# Begin Source File

SOURCE=.\xpilots.rc
# End Source File
# Begin Source File

SOURCE=.\xpilotsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\xpilotsDlg.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\bit.h
# End Source File
# Begin Source File

SOURCE=..\..\common\checknames.c
# End Source File
# Begin Source File

SOURCE=..\..\common\checknames.h
# End Source File
# Begin Source File

SOURCE=..\..\common\commonproto.h
# End Source File
# Begin Source File

SOURCE=..\..\common\config.c
# End Source File
# Begin Source File

SOURCE=..\..\common\config.h
# End Source File
# Begin Source File

SOURCE=..\..\common\const.h
# End Source File
# Begin Source File

SOURCE=..\..\common\draw.h
# End Source File
# Begin Source File

SOURCE=..\..\common\error.c
# End Source File
# Begin Source File

SOURCE=..\..\common\error.h
# End Source File
# Begin Source File

SOURCE=..\..\common\item.h
# End Source File
# Begin Source File

SOURCE=..\..\common\keys.h
# End Source File
# Begin Source File

SOURCE=..\..\common\list.c
# End Source File
# Begin Source File

SOURCE=..\..\common\math.c
# End Source File
# Begin Source File

SOURCE=..\..\common\net.c
# End Source File
# Begin Source File

SOURCE=..\..\common\net.h
# End Source File
# Begin Source File

SOURCE=..\..\common\pack.h
# End Source File
# Begin Source File

SOURCE=..\..\common\portability.c
# End Source File
# Begin Source File

SOURCE=..\..\common\portability.h
# End Source File
# Begin Source File

SOURCE=..\..\common\randommt.c
# End Source File
# Begin Source File

SOURCE=..\..\common\rules.h
# End Source File
# Begin Source File

SOURCE=..\..\common\setup.h
# End Source File
# Begin Source File

SOURCE=..\..\common\shipshape.c
# End Source File
# Begin Source File

SOURCE=..\..\common\socklib.c
# End Source File
# Begin Source File

SOURCE=..\..\common\socklib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\strdup.c
# End Source File
# Begin Source File

SOURCE=..\..\common\strlcpy.c
# End Source File
# Begin Source File

SOURCE=..\..\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xpmemory.c
# End Source File
# End Group
# Begin Group "commonNT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\NT\winNet.c
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winNet.h
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\wsockerrs.c
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\xpilots.ico
# End Source File
# Begin Source File

SOURCE=.\res\xpilots.rc2
# End Source File
# End Group
# End Target
# End Project
