# Microsoft Developer Studio Project File - Name="xpilot_sdl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=xpilot_sdl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xpilot_sdl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xpilot_sdl.mak" CFG="xpilot_sdl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xpilot_sdl - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "xpilot_sdl - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xpilot_sdl - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "D:\projects\SDL_image-1.2.3\include" /I "D:\projects\zlib" /I "D:\projects\SDL_ttf-2.0.6\include" /I "D:\projects\SDL-1.2.6\include" /I ".." /I "..\..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ZLIB_DLL" /D "HAVE_SDL_IMAGE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "NDEBUG"
# ADD RSC /l 0x40b /i "d:\projects\SDL-1.2.6\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDL_ttf.lib zlib.lib opengl32.lib glu32.lib SDL_image.lib SDLmain.lib /nologo /subsystem:console /machine:I386 /libpath:"D:\projects\SDL_image-1.2.3\lib" /libpath:"D:\projects\SDL-1.2.6\lib" /libpath:"D:\projects\SDL_ttf-2.0.6\lib" /libpath:"D:\projects\zlib\dll32"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "xpilot_sdl - Win32 Debug"

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
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "D:\projects\zlib" /I "D:\projects\SDL_ttf-2.0.6\include" /I "D:\projects\SDL-1.2.6\include" /I ".." /I "..\..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ZLIB_DLL" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40b /d "_DEBUG"
# ADD RSC /l 0x40b /i "D:\projects\SDL-1.2.6\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDL_ttf.lib zlib.lib opengl32.lib glu32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"D:\projects\SDL-1.2.6\lib" /libpath:"D:\projects\SDL_ttf-2.0.6\lib" /libpath:"D:\projects\zlib\dll32"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "xpilot_sdl - Win32 Release"
# Name "xpilot_sdl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\common\checknames.c
# End Source File
# Begin Source File

SOURCE=..\client.c
# End Source File
# Begin Source File

SOURCE=..\clientcommand.c
# End Source File
# Begin Source File

SOURCE=..\clientrank.c
# End Source File
# Begin Source File

SOURCE=..\..\common\config.c
# End Source File
# Begin Source File

SOURCE=.\console.c
# End Source File
# Begin Source File

SOURCE=..\datagram.c
# End Source File
# Begin Source File

SOURCE=..\default.c
# End Source File
# Begin Source File

SOURCE=.\DT_drawtext.c
# End Source File
# Begin Source File

SOURCE=..\..\common\error.c
# End Source File
# Begin Source File

SOURCE=..\event.c
# End Source File
# Begin Source File

SOURCE=.\gameloop.c
# End Source File
# Begin Source File

SOURCE=..\gfx2d.c
# End Source File
# Begin Source File

SOURCE=.\glwidgets.c
# End Source File
# Begin Source File

SOURCE=.\images.c
# End Source File
# Begin Source File

SOURCE=..\..\common\list.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=..\mapdata.c
# End Source File
# Begin Source File

SOURCE=..\..\common\math.c
# End Source File
# Begin Source File

SOURCE=..\messages.c
# End Source File
# Begin Source File

SOURCE=..\meta.c
# End Source File
# Begin Source File

SOURCE=..\meta.h
# End Source File
# Begin Source File

SOURCE=..\..\common\net.c
# End Source File
# Begin Source File

SOURCE=..\netclient.c
# End Source File
# Begin Source File

SOURCE=..\option.c
# End Source File
# Begin Source File

SOURCE=..\paint.c
# End Source File
# Begin Source File

SOURCE=..\paintmap.c
# End Source File
# Begin Source File

SOURCE=..\paintobjects.c
# End Source File
# Begin Source File

SOURCE=..\..\common\portability.c
# End Source File
# Begin Source File

SOURCE=..\query.c
# End Source File
# Begin Source File

SOURCE=.\radar.c
# End Source File
# Begin Source File

SOURCE=..\..\common\randommt.c
# End Source File
# Begin Source File

SOURCE=.\SDL_console.c
# End Source File
# Begin Source File

SOURCE=.\SDL_gfxPrimitives.c
# End Source File
# Begin Source File

SOURCE=.\sdlevent.c
# End Source File
# Begin Source File

SOURCE=.\sdlgui.c
# End Source File
# Begin Source File

SOURCE=.\sdlinit.c
# End Source File
# Begin Source File

SOURCE=.\sdlinit.h
# End Source File
# Begin Source File

SOURCE=.\sdlkeys.c
# End Source File
# Begin Source File

SOURCE=.\sdlmeta.c
# End Source File
# Begin Source File

SOURCE=.\sdlmeta.h
# End Source File
# Begin Source File

SOURCE=.\sdlpaint.c
# End Source File
# Begin Source File

SOURCE=.\sdlwindow.c
# End Source File
# Begin Source File

SOURCE=..\..\common\shipshape.c
# End Source File
# Begin Source File

SOURCE=..\..\common\socklib.c
# End Source File
# Begin Source File

SOURCE=..\..\common\strcasecmp.c
# End Source File
# Begin Source File

SOURCE=..\..\common\strdup.c
# End Source File
# Begin Source File

SOURCE=..\..\common\strlcpy.c
# End Source File
# Begin Source File

SOURCE=..\talkmacros.c
# End Source File
# Begin Source File

SOURCE=.\text.c
# End Source File
# Begin Source File

SOURCE=..\textinterface.c
# End Source File
# Begin Source File

SOURCE=.\todo.c
# End Source File
# Begin Source File

SOURCE=..\usleep.c
# End Source File
# Begin Source File

SOURCE=.\win32hacks.c
# End Source File
# Begin Source File

SOURCE=..\..\common\xpmemory.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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

SOURCE=..\..\common\checknames.h
# End Source File
# Begin Source File

SOURCE=..\client.h
# End Source File
# Begin Source File

SOURCE=..\clientrank.h
# End Source File
# Begin Source File

SOURCE=..\..\common\commonproto.h
# End Source File
# Begin Source File

SOURCE=..\connectparam.h
# End Source File
# Begin Source File

SOURCE=.\console.h
# End Source File
# Begin Source File

SOURCE=..\..\common\const.h
# End Source File
# Begin Source File

SOURCE=..\datagram.h
# End Source File
# Begin Source File

SOURCE=..\..\common\draw.h
# End Source File
# Begin Source File

SOURCE=.\DT_drawtext.h
# End Source File
# Begin Source File

SOURCE=..\..\common\error.h
# End Source File
# Begin Source File

SOURCE=..\gfx2d.h
# End Source File
# Begin Source File

SOURCE=.\glwidgets.h
# End Source File
# Begin Source File

SOURCE=.\images.h
# End Source File
# Begin Source File

SOURCE=..\..\common\item.h
# End Source File
# Begin Source File

SOURCE=..\..\common\keys.h
# End Source File
# Begin Source File

SOURCE=..\..\common\list.h
# End Source File
# Begin Source File

SOURCE=..\..\common\net.h
# End Source File
# Begin Source File

SOURCE=..\netclient.h
# End Source File
# Begin Source File

SOURCE=..\option.h
# End Source File
# Begin Source File

SOURCE=..\..\common\pack.h
# End Source File
# Begin Source File

SOURCE=..\..\common\packet.h
# End Source File
# Begin Source File

SOURCE=..\paint.h
# End Source File
# Begin Source File

SOURCE=..\..\common\portability.h
# End Source File
# Begin Source File

SOURCE=..\protoclient.h
# End Source File
# Begin Source File

SOURCE=.\radar.h
# End Source File
# Begin Source File

SOURCE=..\..\common\rules.h
# End Source File
# Begin Source File

SOURCE=.\SDL_console.h
# End Source File
# Begin Source File

SOURCE=.\SDL_gfxPrimitives.h
# End Source File
# Begin Source File

SOURCE=.\SDL_gfxPrimitives_font.h
# End Source File
# Begin Source File

SOURCE=.\sdlkeys.h
# End Source File
# Begin Source File

SOURCE=.\sdlpaint.h
# End Source File
# Begin Source File

SOURCE=.\sdlwindow.h
# End Source File
# Begin Source File

SOURCE=..\..\common\setup.h
# End Source File
# Begin Source File

SOURCE=..\..\common\socklib.h
# End Source File
# Begin Source File

SOURCE=.\text.h
# End Source File
# Begin Source File

SOURCE=..\..\common\types.h
# End Source File
# Begin Source File

SOURCE=..\..\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\common\wreckshape.h
# End Source File
# Begin Source File

SOURCE=..\xpclient.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xpcommon.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xpconfig.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
