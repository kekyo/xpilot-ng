# Microsoft Developer Studio Project File - Name="XPilot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=XPilot - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xpilot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xpilot.mak" CFG="XPilot - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XPilot - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "XPilot - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "XPilot - Win32 ReleasePentium" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "XPilot - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /I "..\..\common" /I "..\..\common\NT" /I "..\..\..\zlib\include" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /FR /FD /c
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
# ADD LINK32 winmm.lib ..\..\..\zlib\lib\libz.lib /nologo /subsystem:windows /map /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\XPilot.exe C:\XPilot
# End Special Build Tool

!ELSEIF  "$(CFG)" == "XPilot - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\common" /I "..\..\common\NT" /I "..\..\..\zlib\include" /D "_DEBUG" /D "_MEMPOD" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /FR /FD /c
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
# ADD LINK32 winmm.lib d:\xpilotbuild\zlib\lib\zdll.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "XPilot - Win32 ReleasePentium"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\XPilot__"
# PROP BASE Intermediate_Dir ".\XPilot__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\XPilot__"
# PROP Intermediate_Dir ".\XPilot__"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zd /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "x_BETAEXPIRE" /FR /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /G5 /MD /W3 /GX /Zd /O2 /I "..\..\common" /I "..\..\common\NT" /I "..\..\..\..\zlib" /D "NDEBUG" /D "_MBCS" /D "x_BETAEXPIRE" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /D "ZLIB_DLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /map /machine:I386 /out:"Release/XPilotNT.exe"
# ADD LINK32 winmm.lib d:\xpilotbuild\zlib\lib\zdll.lib /nologo /subsystem:windows /map /machine:I386 /out:".\Release\XPilot.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\XPilot.exe c:\XPilot
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "XPilot - Win32 Release"
# Name "XPilot - Win32 Debug"
# Name "XPilot - Win32 ReleasePentium"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "client"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\.cvsignore
# End Source File
# Begin Source File

SOURCE=.\about.c
# End Source File
# Begin Source File

SOURCE=.\bitmaps.c
# End Source File
# Begin Source File

SOURCE=.\bitmaps.h
# End Source File
# Begin Source File

SOURCE=..\client.c
# End Source File
# Begin Source File

SOURCE=..\client.h
# End Source File
# Begin Source File

SOURCE=..\clientcommand.c
# End Source File
# Begin Source File

SOURCE=..\clientcommand.h
# End Source File
# Begin Source File

SOURCE=..\clientrank.c
# End Source File
# Begin Source File

SOURCE=..\clientrank.h
# End Source File
# Begin Source File

SOURCE=.\colors.c
# End Source File
# Begin Source File

SOURCE=.\configure.c
# End Source File
# Begin Source File

SOURCE=..\configure.h
# End Source File
# Begin Source File

SOURCE=..\connectparam.h
# End Source File
# Begin Source File

SOURCE=..\datagram.c
# End Source File
# Begin Source File

SOURCE=..\datagram.h
# End Source File
# Begin Source File

SOURCE=.\dbuff.h
# End Source File
# Begin Source File

SOURCE=..\default.c
# End Source File
# Begin Source File

SOURCE=..\default.h
# End Source File
# Begin Source File

SOURCE=..\event.c
# End Source File
# Begin Source File

SOURCE=..\gfx2d.c
# End Source File
# Begin Source File

SOURCE=..\gfx2d.h
# End Source File
# Begin Source File

SOURCE=.\guimap.c
# End Source File
# Begin Source File

SOURCE=..\guimap.h
# End Source File
# Begin Source File

SOURCE=.\guiobjects.c
# End Source File
# Begin Source File

SOURCE=..\guiobjects.h
# End Source File
# Begin Source File

SOURCE=.\icon.h
# End Source File
# Begin Source File

SOURCE=.\join.c
# End Source File
# Begin Source File

SOURCE=..\mapdata.c
# End Source File
# Begin Source File

SOURCE=..\messages.c
# End Source File
# Begin Source File

SOURCE=..\netclient.c
# End Source File
# Begin Source File

SOURCE=..\netclient.h
# End Source File
# Begin Source File

SOURCE=..\option.c
# End Source File
# Begin Source File

SOURCE=..\option.h
# End Source File
# Begin Source File

SOURCE=..\paint.c
# End Source File
# Begin Source File

SOURCE=..\paint.h
# End Source File
# Begin Source File

SOURCE=.\paintdata.c
# End Source File
# Begin Source File

SOURCE=..\paintdata.h
# End Source File
# Begin Source File

SOURCE=.\painthud.c
# End Source File
# Begin Source File

SOURCE=..\paintmacros.h
# End Source File
# Begin Source File

SOURCE=..\paintmap.c
# End Source File
# Begin Source File

SOURCE=..\paintobjects.c
# End Source File
# Begin Source File

SOURCE=.\paintradar.c
# End Source File
# Begin Source File

SOURCE=..\query.c
# End Source File
# Begin Source File

SOURCE=.\record.c
# End Source File
# Begin Source File

SOURCE=.\record.h
# End Source File
# Begin Source File

SOURCE=.\recordfmt.h
# End Source File
# Begin Source File

SOURCE=..\talk.h
# End Source File
# Begin Source File

SOURCE=..\talkmacros.c
# End Source File
# Begin Source File

SOURCE=..\textinterface.c
# End Source File
# Begin Source File

SOURCE=..\usleep.c
# End Source File
# Begin Source File

SOURCE=.\widget.c
# End Source File
# Begin Source File

SOURCE=.\widget.h
# End Source File
# Begin Source File

SOURCE=.\xdefault.c
# End Source File
# Begin Source File

SOURCE=.\xevent.c
# End Source File
# Begin Source File

SOURCE=.\xevent.h
# End Source File
# Begin Source File

SOURCE=.\xeventhandlers.c
# End Source File
# Begin Source File

SOURCE=.\xeventhandlers.h
# End Source File
# Begin Source File

SOURCE=.\xinit.c
# End Source File
# Begin Source File

SOURCE=.\xinit.h
# End Source File
# Begin Source File

SOURCE=.\xpaint.c
# End Source File
# Begin Source File

SOURCE=..\xpclient.h
# End Source File
# Begin Source File

SOURCE=.\xpclient_x11.h
# End Source File
# Begin Source File

SOURCE=.\xpilot.c
# End Source File
# Begin Source File

SOURCE=..\xpmread.h
# End Source File
# End Group
# Begin Group "clientNT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\.cvsignore
# End Source File
# Begin Source File

SOURCE=.\BSString.cpp
# End Source File
# Begin Source File

SOURCE=.\BSString.h
# End Source File
# Begin Source File

SOURCE=.\credits.inc.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\RecordDummy.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\TalkWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\TalkWindow.h
# End Source File
# Begin Source File

SOURCE=.\winAbout.cpp
# End Source File
# Begin Source File

SOURCE=.\winAbout.h
# End Source File
# Begin Source File

SOURCE=.\winAudio.c
# End Source File
# Begin Source File

SOURCE=.\winAudio.h
# End Source File
# Begin Source File

SOURCE=.\winBitmap.c
# End Source File
# Begin Source File

SOURCE=.\winBitmap.h
# End Source File
# Begin Source File

SOURCE=.\winClient.h
# End Source File
# Begin Source File

SOURCE=.\winColors.c
# End Source File
# Begin Source File

SOURCE=.\wintalk.c
# End Source File
# Begin Source File

SOURCE=.\winXThread.c
# End Source File
# Begin Source File

SOURCE=.\winXThread.h
# End Source File
# Begin Source File

SOURCE=.\winXXPilot.h
# End Source File
# Begin Source File

SOURCE=.\xpilot.rc
# End Source File
# Begin Source File

SOURCE=.\xpilotDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\xpilotDoc.h
# End Source File
# Begin Source File

SOURCE=.\XPilotNT.cpp
# End Source File
# Begin Source File

SOURCE=.\XPilotNT.h
# End Source File
# Begin Source File

SOURCE=.\xpilotView.cpp
# End Source File
# Begin Source File

SOURCE=.\xpilotView.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\.cvsignore
# End Source File
# Begin Source File

SOURCE=..\..\common\astershape.h
# End Source File
# Begin Source File

SOURCE=..\..\common\audio.h
# End Source File
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

SOURCE=..\..\common\wreckshape.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xpconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xpmemory.c
# End Source File
# End Group
# Begin Group "commonNT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\NT\.cvsignore
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winNet.c
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winNet.h
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winX.c
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winX.h
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winX11.c
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winX_.h
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winXKey.c
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\winXKey.h
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\wsockerrs.c
# End Source File
# End Group
# Begin Group "items"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\items\itemAfterburner.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemArmor.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemAutopilot.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemCloakingDevice.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemDeflector.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemEcm.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemEmergencyShield.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemEmergencyThrust.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemEnergyPack.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemHyperJump.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemLaser.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemMinePack.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemMirror.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemPhasingDevice.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemRearShot.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemRocketPack.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemSensorPack.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemTank.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemTractorBeam.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemTransporter.xbm
# End Source File
# Begin Source File

SOURCE=..\items\itemWideangleShot.xbm
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\lib\textures\ball.xpm
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\textures\rock4.xpm
# End Source File
# Begin Source File

SOURCE=.\res\Splsh16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\xpilot.ico
# End Source File
# Begin Source File

SOURCE=.\res\xpilot.rc2
# End Source File
# Begin Source File

SOURCE=..\..\server\NT\res\xpilots.ico
# End Source File
# End Group
# End Target
# End Project
# Section XPilot : {0FEA6ED2-A2FE-11CF-80DA-00A02423A46C}
# 	0:8:Splash.h:D:\users\dick\buckosof\xpilot\contrib\NT\xpilot\Splash.h
# 	0:10:Splash.cpp:D:\users\dick\buckosof\xpilot\contrib\NT\xpilot\Splash.cpp
# 	1:10:IDB_SPLASH:102
# 	2:10:ResHdrName:resource.h
# 	2:11:ProjHdrName:stdafx.h
# 	2:10:WrapperDef:_SPLASH_SCRN_
# 	2:12:SplClassName:CSplashWnd
# 	2:21:SplashScreenInsertKey:4.0
# 	2:10:HeaderName:Splash.h
# 	2:10:ImplemName:Splash.cpp
# 	2:7:BmpID16:IDB_SPLASH
# End Section
