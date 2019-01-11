# Microsoft Developer Studio Project File - Name="XPilotFiles" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=XPilotFiles - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XPilotFiles.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XPilotFiles.mak" CFG="XPilotFiles - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XPilotFiles - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "XPilotFiles - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "XPilotFiles - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f XPilotFiles.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XPilotFiles.exe"
# PROP BASE Bsc_Name "XPilotFiles.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f XPilotFiles.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "XPilotFiles.exe"
# PROP Bsc_Name "XPilotFiles.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "XPilotFiles - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f XPilotFiles.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XPilotFiles.exe"
# PROP BASE Bsc_Name "XPilotFiles.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f XPilotFiles.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "XPilotFiles.exe"
# PROP Bsc_Name "XPilotFiles.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "XPilotFiles - Win32 Release"
# Name "XPilotFiles - Win32 Debug"

!IF  "$(CFG)" == "XPilotFiles - Win32 Release"

!ELSEIF  "$(CFG)" == "XPilotFiles - Win32 Debug"

!ENDIF 

# Begin Group "bindist"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bindist\creditsUpdate.pl
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\bindist\makeDistribution
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\bindist\README.txt.msub
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\bindist\READMEbin.txt.msub
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\bindist\ServerMOTD.txt.msub
# End Source File
# Begin Source File

SOURCE=.\XPilot.ini
# End Source File
# Begin Source File

SOURCE=..\..\common\NT\bindist\XPilot.nsi
# End Source File
# End Group
# Begin Group "./"

# PROP Default_Filter ""
# Begin Group "doc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\doc\.cvsignore
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\ChangeLog
# End Source File
# Begin Source File

SOURCE="..\..\..\doc\ChangeLog-3"
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\CREDITS
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\FAQ
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\Imakefile
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\README.MAPS
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\README.MAPS2
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\README.SHIPS
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\README.sounds
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\README.talkmacros
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\TODO
# End Source File
# Begin Source File

SOURCE="..\..\..\doc\xpilot-linux.dif"
# End Source File
# Begin Source File

SOURCE=..\..\..\doc\xpilot.spec
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\.cvsignore
# End Source File
# Begin Source File

SOURCE=..\..\..\Imakefile
# End Source File
# Begin Source File

SOURCE=..\..\..\INSTALL.txt
# End Source File
# Begin Source File

SOURCE=..\..\..\COPYING
# End Source File
# Begin Source File

SOURCE=..\..\..\Local.config
# End Source File
# Begin Source File

SOURCE=..\..\..\Local.rules
# End Source File
# Begin Source File

SOURCE=..\..\..\README.txt
# End Source File
# Begin Source File

SOURCE=..\..\..\README.txt.msub
# End Source File
# End Group
# End Target
# End Project
