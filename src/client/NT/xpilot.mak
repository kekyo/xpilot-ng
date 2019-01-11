# Microsoft Developer Studio Generated NMAKE File, Based on xpilot.dsp
!IF "$(CFG)" == ""
CFG=XPilot - Win32 Release
!MESSAGE No configuration specified. Defaulting to XPilot - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "XPilot - Win32 Release" && "$(CFG)" != "XPilot - Win32 Debug" && "$(CFG)" != "XPilot - Win32 ReleasePentium"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "XPilot - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\xpilot.exe" "$(OUTDIR)\xpilot.bsc"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\blockbitmaps.obj"
	-@erase "$(INTDIR)\blockbitmaps.sbr"
	-@erase "$(INTDIR)\BSString.obj"
	-@erase "$(INTDIR)\BSString.sbr"
	-@erase "$(INTDIR)\caudio.obj"
	-@erase "$(INTDIR)\caudio.sbr"
	-@erase "$(INTDIR)\checknames.obj"
	-@erase "$(INTDIR)\checknames.sbr"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client.sbr"
	-@erase "$(INTDIR)\colors.obj"
	-@erase "$(INTDIR)\colors.sbr"
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\config.sbr"
	-@erase "$(INTDIR)\configure.obj"
	-@erase "$(INTDIR)\configure.sbr"
	-@erase "$(INTDIR)\datagram.obj"
	-@erase "$(INTDIR)\datagram.sbr"
	-@erase "$(INTDIR)\default.obj"
	-@erase "$(INTDIR)\default.sbr"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\error.sbr"
	-@erase "$(INTDIR)\gfx2d.obj"
	-@erase "$(INTDIR)\gfx2d.sbr"
	-@erase "$(INTDIR)\gfx3d.obj"
	-@erase "$(INTDIR)\gfx3d.sbr"
	-@erase "$(INTDIR)\guimap.obj"
	-@erase "$(INTDIR)\guimap.sbr"
	-@erase "$(INTDIR)\guiobjects.obj"
	-@erase "$(INTDIR)\guiobjects.sbr"
	-@erase "$(INTDIR)\join.obj"
	-@erase "$(INTDIR)\join.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\math.obj"
	-@erase "$(INTDIR)\math.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\netclient.obj"
	-@erase "$(INTDIR)\netclient.sbr"
	-@erase "$(INTDIR)\paint.obj"
	-@erase "$(INTDIR)\paint.sbr"
	-@erase "$(INTDIR)\paintdata.obj"
	-@erase "$(INTDIR)\paintdata.sbr"
	-@erase "$(INTDIR)\painthud.obj"
	-@erase "$(INTDIR)\painthud.sbr"
	-@erase "$(INTDIR)\paintmap.obj"
	-@erase "$(INTDIR)\paintmap.sbr"
	-@erase "$(INTDIR)\paintobjects.obj"
	-@erase "$(INTDIR)\paintobjects.sbr"
	-@erase "$(INTDIR)\paintradar.obj"
	-@erase "$(INTDIR)\paintradar.sbr"
	-@erase "$(INTDIR)\portability.obj"
	-@erase "$(INTDIR)\portability.sbr"
	-@erase "$(INTDIR)\query.obj"
	-@erase "$(INTDIR)\query.sbr"
	-@erase "$(INTDIR)\randommt.obj"
	-@erase "$(INTDIR)\randommt.sbr"
	-@erase "$(INTDIR)\record.obj"
	-@erase "$(INTDIR)\record.sbr"
	-@erase "$(INTDIR)\shipshape.obj"
	-@erase "$(INTDIR)\shipshape.sbr"
	-@erase "$(INTDIR)\sim.obj"
	-@erase "$(INTDIR)\sim.sbr"
	-@erase "$(INTDIR)\socklib.obj"
	-@erase "$(INTDIR)\socklib.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\strdup.obj"
	-@erase "$(INTDIR)\strdup.sbr"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\strlcpy.sbr"
	-@erase "$(INTDIR)\syslimit.obj"
	-@erase "$(INTDIR)\syslimit.sbr"
	-@erase "$(INTDIR)\talkmacros.obj"
	-@erase "$(INTDIR)\talkmacros.sbr"
	-@erase "$(INTDIR)\TalkWindow.obj"
	-@erase "$(INTDIR)\TalkWindow.sbr"
	-@erase "$(INTDIR)\textinterface.obj"
	-@erase "$(INTDIR)\textinterface.sbr"
	-@erase "$(INTDIR)\texture.obj"
	-@erase "$(INTDIR)\texture.sbr"
	-@erase "$(INTDIR)\usleep.obj"
	-@erase "$(INTDIR)\usleep.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\widget.obj"
	-@erase "$(INTDIR)\widget.sbr"
	-@erase "$(INTDIR)\winAbout.obj"
	-@erase "$(INTDIR)\winAbout.sbr"
	-@erase "$(INTDIR)\winAudio.obj"
	-@erase "$(INTDIR)\winAudio.sbr"
	-@erase "$(INTDIR)\winBitmap.obj"
	-@erase "$(INTDIR)\winBitmap.sbr"
	-@erase "$(INTDIR)\winConfig.obj"
	-@erase "$(INTDIR)\winConfig.sbr"
	-@erase "$(INTDIR)\winNet.obj"
	-@erase "$(INTDIR)\winNet.sbr"
	-@erase "$(INTDIR)\winX.obj"
	-@erase "$(INTDIR)\winX.sbr"
	-@erase "$(INTDIR)\winX11.obj"
	-@erase "$(INTDIR)\winX11.sbr"
	-@erase "$(INTDIR)\winXKey.obj"
	-@erase "$(INTDIR)\winXKey.sbr"
	-@erase "$(INTDIR)\winXThread.obj"
	-@erase "$(INTDIR)\winXThread.sbr"
	-@erase "$(INTDIR)\wsockerrs.obj"
	-@erase "$(INTDIR)\wsockerrs.sbr"
	-@erase "$(INTDIR)\xevent.obj"
	-@erase "$(INTDIR)\xevent.sbr"
	-@erase "$(INTDIR)\xeventhandlers.obj"
	-@erase "$(INTDIR)\xeventhandlers.sbr"
	-@erase "$(INTDIR)\xinit.obj"
	-@erase "$(INTDIR)\xinit.sbr"
	-@erase "$(INTDIR)\xpilot.obj"
	-@erase "$(INTDIR)\xpilot.res"
	-@erase "$(INTDIR)\xpilot.sbr"
	-@erase "$(INTDIR)\xpilotDoc.obj"
	-@erase "$(INTDIR)\xpilotDoc.sbr"
	-@erase "$(INTDIR)\XPilotNT.obj"
	-@erase "$(INTDIR)\XPilotNT.sbr"
	-@erase "$(INTDIR)\xpilotView.obj"
	-@erase "$(INTDIR)\xpilotView.sbr"
	-@erase "$(INTDIR)\xpmemory.obj"
	-@erase "$(INTDIR)\xpmemory.sbr"
	-@erase "$(OUTDIR)\xpilot.bsc"
	-@erase "$(OUTDIR)\xpilot.exe"
	-@erase "$(OUTDIR)\xpilot.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /I "..\..\common" /I "..\..\common\NT" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\xpilot.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xpilot.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\blockbitmaps.sbr" \
	"$(INTDIR)\caudio.sbr" \
	"$(INTDIR)\client.sbr" \
	"$(INTDIR)\colors.sbr" \
	"$(INTDIR)\configure.sbr" \
	"$(INTDIR)\datagram.sbr" \
	"$(INTDIR)\default.sbr" \
	"$(INTDIR)\gfx2d.sbr" \
	"$(INTDIR)\gfx3d.sbr" \
	"$(INTDIR)\guimap.sbr" \
	"$(INTDIR)\guiobjects.sbr" \
	"$(INTDIR)\join.sbr" \
	"$(INTDIR)\netclient.sbr" \
	"$(INTDIR)\paint.sbr" \
	"$(INTDIR)\paintdata.sbr" \
	"$(INTDIR)\painthud.sbr" \
	"$(INTDIR)\paintmap.sbr" \
	"$(INTDIR)\paintobjects.sbr" \
	"$(INTDIR)\paintradar.sbr" \
	"$(INTDIR)\query.sbr" \
	"$(INTDIR)\record.sbr" \
	"$(INTDIR)\sim.sbr" \
	"$(INTDIR)\syslimit.sbr" \
	"$(INTDIR)\talkmacros.sbr" \
	"$(INTDIR)\textinterface.sbr" \
	"$(INTDIR)\texture.sbr" \
	"$(INTDIR)\usleep.sbr" \
	"$(INTDIR)\widget.sbr" \
	"$(INTDIR)\xevent.sbr" \
	"$(INTDIR)\xeventhandlers.sbr" \
	"$(INTDIR)\xinit.sbr" \
	"$(INTDIR)\xpilot.sbr" \
	"$(INTDIR)\BSString.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\TalkWindow.sbr" \
	"$(INTDIR)\winAbout.sbr" \
	"$(INTDIR)\winAudio.sbr" \
	"$(INTDIR)\winBitmap.sbr" \
	"$(INTDIR)\winConfig.sbr" \
	"$(INTDIR)\winXThread.sbr" \
	"$(INTDIR)\xpilotDoc.sbr" \
	"$(INTDIR)\XPilotNT.sbr" \
	"$(INTDIR)\xpilotView.sbr" \
	"$(INTDIR)\checknames.sbr" \
	"$(INTDIR)\config.sbr" \
	"$(INTDIR)\error.sbr" \
	"$(INTDIR)\math.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\portability.sbr" \
	"$(INTDIR)\randommt.sbr" \
	"$(INTDIR)\shipshape.sbr" \
	"$(INTDIR)\socklib.sbr" \
	"$(INTDIR)\strdup.sbr" \
	"$(INTDIR)\strlcpy.sbr" \
	"$(INTDIR)\xpmemory.sbr" \
	"$(INTDIR)\winNet.sbr" \
	"$(INTDIR)\winX.sbr" \
	"$(INTDIR)\winX11.sbr" \
	"$(INTDIR)\winXKey.sbr" \
	"$(INTDIR)\wsockerrs.sbr"

"$(OUTDIR)\xpilot.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\xpilot.pdb" /map:"$(INTDIR)\xpilot.map" /machine:I386 /out:"$(OUTDIR)\xpilot.exe" 
LINK32_OBJS= \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\blockbitmaps.obj" \
	"$(INTDIR)\caudio.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\colors.obj" \
	"$(INTDIR)\configure.obj" \
	"$(INTDIR)\datagram.obj" \
	"$(INTDIR)\default.obj" \
	"$(INTDIR)\gfx2d.obj" \
	"$(INTDIR)\gfx3d.obj" \
	"$(INTDIR)\guimap.obj" \
	"$(INTDIR)\guiobjects.obj" \
	"$(INTDIR)\join.obj" \
	"$(INTDIR)\netclient.obj" \
	"$(INTDIR)\paint.obj" \
	"$(INTDIR)\paintdata.obj" \
	"$(INTDIR)\painthud.obj" \
	"$(INTDIR)\paintmap.obj" \
	"$(INTDIR)\paintobjects.obj" \
	"$(INTDIR)\paintradar.obj" \
	"$(INTDIR)\query.obj" \
	"$(INTDIR)\record.obj" \
	"$(INTDIR)\sim.obj" \
	"$(INTDIR)\syslimit.obj" \
	"$(INTDIR)\talkmacros.obj" \
	"$(INTDIR)\textinterface.obj" \
	"$(INTDIR)\texture.obj" \
	"$(INTDIR)\usleep.obj" \
	"$(INTDIR)\widget.obj" \
	"$(INTDIR)\xevent.obj" \
	"$(INTDIR)\xeventhandlers.obj" \
	"$(INTDIR)\xinit.obj" \
	"$(INTDIR)\xpilot.obj" \
	"$(INTDIR)\BSString.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\TalkWindow.obj" \
	"$(INTDIR)\winAbout.obj" \
	"$(INTDIR)\winAudio.obj" \
	"$(INTDIR)\winBitmap.obj" \
	"$(INTDIR)\winConfig.obj" \
	"$(INTDIR)\winXThread.obj" \
	"$(INTDIR)\xpilotDoc.obj" \
	"$(INTDIR)\XPilotNT.obj" \
	"$(INTDIR)\xpilotView.obj" \
	"$(INTDIR)\checknames.obj" \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\math.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\portability.obj" \
	"$(INTDIR)\randommt.obj" \
	"$(INTDIR)\shipshape.obj" \
	"$(INTDIR)\socklib.obj" \
	"$(INTDIR)\strdup.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\xpmemory.obj" \
	"$(INTDIR)\winNet.obj" \
	"$(INTDIR)\winX.obj" \
	"$(INTDIR)\winX11.obj" \
	"$(INTDIR)\winXKey.obj" \
	"$(INTDIR)\wsockerrs.obj" \
	"$(INTDIR)\xpilot.res"

"$(OUTDIR)\xpilot.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\xpilot.exe" "$(OUTDIR)\xpilot.bsc"
   copy Release\XPilot.exe C:\XPilot
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "XPilot - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\xpilot.exe" "$(OUTDIR)\xpilot.bsc"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\blockbitmaps.obj"
	-@erase "$(INTDIR)\blockbitmaps.sbr"
	-@erase "$(INTDIR)\BSString.obj"
	-@erase "$(INTDIR)\BSString.sbr"
	-@erase "$(INTDIR)\caudio.obj"
	-@erase "$(INTDIR)\caudio.sbr"
	-@erase "$(INTDIR)\checknames.obj"
	-@erase "$(INTDIR)\checknames.sbr"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client.sbr"
	-@erase "$(INTDIR)\colors.obj"
	-@erase "$(INTDIR)\colors.sbr"
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\config.sbr"
	-@erase "$(INTDIR)\configure.obj"
	-@erase "$(INTDIR)\configure.sbr"
	-@erase "$(INTDIR)\datagram.obj"
	-@erase "$(INTDIR)\datagram.sbr"
	-@erase "$(INTDIR)\default.obj"
	-@erase "$(INTDIR)\default.sbr"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\error.sbr"
	-@erase "$(INTDIR)\gfx2d.obj"
	-@erase "$(INTDIR)\gfx2d.sbr"
	-@erase "$(INTDIR)\gfx3d.obj"
	-@erase "$(INTDIR)\gfx3d.sbr"
	-@erase "$(INTDIR)\guimap.obj"
	-@erase "$(INTDIR)\guimap.sbr"
	-@erase "$(INTDIR)\guiobjects.obj"
	-@erase "$(INTDIR)\guiobjects.sbr"
	-@erase "$(INTDIR)\join.obj"
	-@erase "$(INTDIR)\join.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\math.obj"
	-@erase "$(INTDIR)\math.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\netclient.obj"
	-@erase "$(INTDIR)\netclient.sbr"
	-@erase "$(INTDIR)\paint.obj"
	-@erase "$(INTDIR)\paint.sbr"
	-@erase "$(INTDIR)\paintdata.obj"
	-@erase "$(INTDIR)\paintdata.sbr"
	-@erase "$(INTDIR)\painthud.obj"
	-@erase "$(INTDIR)\painthud.sbr"
	-@erase "$(INTDIR)\paintmap.obj"
	-@erase "$(INTDIR)\paintmap.sbr"
	-@erase "$(INTDIR)\paintobjects.obj"
	-@erase "$(INTDIR)\paintobjects.sbr"
	-@erase "$(INTDIR)\paintradar.obj"
	-@erase "$(INTDIR)\paintradar.sbr"
	-@erase "$(INTDIR)\portability.obj"
	-@erase "$(INTDIR)\portability.sbr"
	-@erase "$(INTDIR)\query.obj"
	-@erase "$(INTDIR)\query.sbr"
	-@erase "$(INTDIR)\randommt.obj"
	-@erase "$(INTDIR)\randommt.sbr"
	-@erase "$(INTDIR)\record.obj"
	-@erase "$(INTDIR)\record.sbr"
	-@erase "$(INTDIR)\shipshape.obj"
	-@erase "$(INTDIR)\shipshape.sbr"
	-@erase "$(INTDIR)\sim.obj"
	-@erase "$(INTDIR)\sim.sbr"
	-@erase "$(INTDIR)\socklib.obj"
	-@erase "$(INTDIR)\socklib.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\strdup.obj"
	-@erase "$(INTDIR)\strdup.sbr"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\strlcpy.sbr"
	-@erase "$(INTDIR)\syslimit.obj"
	-@erase "$(INTDIR)\syslimit.sbr"
	-@erase "$(INTDIR)\talkmacros.obj"
	-@erase "$(INTDIR)\talkmacros.sbr"
	-@erase "$(INTDIR)\TalkWindow.obj"
	-@erase "$(INTDIR)\TalkWindow.sbr"
	-@erase "$(INTDIR)\textinterface.obj"
	-@erase "$(INTDIR)\textinterface.sbr"
	-@erase "$(INTDIR)\texture.obj"
	-@erase "$(INTDIR)\texture.sbr"
	-@erase "$(INTDIR)\usleep.obj"
	-@erase "$(INTDIR)\usleep.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\widget.obj"
	-@erase "$(INTDIR)\widget.sbr"
	-@erase "$(INTDIR)\winAbout.obj"
	-@erase "$(INTDIR)\winAbout.sbr"
	-@erase "$(INTDIR)\winAudio.obj"
	-@erase "$(INTDIR)\winAudio.sbr"
	-@erase "$(INTDIR)\winBitmap.obj"
	-@erase "$(INTDIR)\winBitmap.sbr"
	-@erase "$(INTDIR)\winConfig.obj"
	-@erase "$(INTDIR)\winConfig.sbr"
	-@erase "$(INTDIR)\winNet.obj"
	-@erase "$(INTDIR)\winNet.sbr"
	-@erase "$(INTDIR)\winX.obj"
	-@erase "$(INTDIR)\winX.sbr"
	-@erase "$(INTDIR)\winX11.obj"
	-@erase "$(INTDIR)\winX11.sbr"
	-@erase "$(INTDIR)\winXKey.obj"
	-@erase "$(INTDIR)\winXKey.sbr"
	-@erase "$(INTDIR)\winXThread.obj"
	-@erase "$(INTDIR)\winXThread.sbr"
	-@erase "$(INTDIR)\wsockerrs.obj"
	-@erase "$(INTDIR)\wsockerrs.sbr"
	-@erase "$(INTDIR)\xevent.obj"
	-@erase "$(INTDIR)\xevent.sbr"
	-@erase "$(INTDIR)\xeventhandlers.obj"
	-@erase "$(INTDIR)\xeventhandlers.sbr"
	-@erase "$(INTDIR)\xinit.obj"
	-@erase "$(INTDIR)\xinit.sbr"
	-@erase "$(INTDIR)\xpilot.obj"
	-@erase "$(INTDIR)\xpilot.res"
	-@erase "$(INTDIR)\xpilot.sbr"
	-@erase "$(INTDIR)\xpilotDoc.obj"
	-@erase "$(INTDIR)\xpilotDoc.sbr"
	-@erase "$(INTDIR)\XPilotNT.obj"
	-@erase "$(INTDIR)\XPilotNT.sbr"
	-@erase "$(INTDIR)\xpilotView.obj"
	-@erase "$(INTDIR)\xpilotView.sbr"
	-@erase "$(INTDIR)\xpmemory.obj"
	-@erase "$(INTDIR)\xpmemory.sbr"
	-@erase "$(OUTDIR)\xpilot.bsc"
	-@erase "$(OUTDIR)\xpilot.exe"
	-@erase "$(OUTDIR)\xpilot.ilk"
	-@erase "$(OUTDIR)\xpilot.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\common" /I "..\..\common\NT" /D "_DEBUG" /D "_MEMPOD" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\xpilot.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xpilot.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\blockbitmaps.sbr" \
	"$(INTDIR)\caudio.sbr" \
	"$(INTDIR)\client.sbr" \
	"$(INTDIR)\colors.sbr" \
	"$(INTDIR)\configure.sbr" \
	"$(INTDIR)\datagram.sbr" \
	"$(INTDIR)\default.sbr" \
	"$(INTDIR)\gfx2d.sbr" \
	"$(INTDIR)\gfx3d.sbr" \
	"$(INTDIR)\guimap.sbr" \
	"$(INTDIR)\guiobjects.sbr" \
	"$(INTDIR)\join.sbr" \
	"$(INTDIR)\netclient.sbr" \
	"$(INTDIR)\paint.sbr" \
	"$(INTDIR)\paintdata.sbr" \
	"$(INTDIR)\painthud.sbr" \
	"$(INTDIR)\paintmap.sbr" \
	"$(INTDIR)\paintobjects.sbr" \
	"$(INTDIR)\paintradar.sbr" \
	"$(INTDIR)\query.sbr" \
	"$(INTDIR)\record.sbr" \
	"$(INTDIR)\sim.sbr" \
	"$(INTDIR)\syslimit.sbr" \
	"$(INTDIR)\talkmacros.sbr" \
	"$(INTDIR)\textinterface.sbr" \
	"$(INTDIR)\texture.sbr" \
	"$(INTDIR)\usleep.sbr" \
	"$(INTDIR)\widget.sbr" \
	"$(INTDIR)\xevent.sbr" \
	"$(INTDIR)\xeventhandlers.sbr" \
	"$(INTDIR)\xinit.sbr" \
	"$(INTDIR)\xpilot.sbr" \
	"$(INTDIR)\BSString.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\TalkWindow.sbr" \
	"$(INTDIR)\winAbout.sbr" \
	"$(INTDIR)\winAudio.sbr" \
	"$(INTDIR)\winBitmap.sbr" \
	"$(INTDIR)\winConfig.sbr" \
	"$(INTDIR)\winXThread.sbr" \
	"$(INTDIR)\xpilotDoc.sbr" \
	"$(INTDIR)\XPilotNT.sbr" \
	"$(INTDIR)\xpilotView.sbr" \
	"$(INTDIR)\checknames.sbr" \
	"$(INTDIR)\config.sbr" \
	"$(INTDIR)\error.sbr" \
	"$(INTDIR)\math.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\portability.sbr" \
	"$(INTDIR)\randommt.sbr" \
	"$(INTDIR)\shipshape.sbr" \
	"$(INTDIR)\socklib.sbr" \
	"$(INTDIR)\strdup.sbr" \
	"$(INTDIR)\strlcpy.sbr" \
	"$(INTDIR)\xpmemory.sbr" \
	"$(INTDIR)\winNet.sbr" \
	"$(INTDIR)\winX.sbr" \
	"$(INTDIR)\winX11.sbr" \
	"$(INTDIR)\winXKey.sbr" \
	"$(INTDIR)\wsockerrs.sbr"

"$(OUTDIR)\xpilot.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\xpilot.pdb" /debug /machine:I386 /out:"$(OUTDIR)\xpilot.exe" 
LINK32_OBJS= \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\blockbitmaps.obj" \
	"$(INTDIR)\caudio.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\colors.obj" \
	"$(INTDIR)\configure.obj" \
	"$(INTDIR)\datagram.obj" \
	"$(INTDIR)\default.obj" \
	"$(INTDIR)\gfx2d.obj" \
	"$(INTDIR)\gfx3d.obj" \
	"$(INTDIR)\guimap.obj" \
	"$(INTDIR)\guiobjects.obj" \
	"$(INTDIR)\join.obj" \
	"$(INTDIR)\netclient.obj" \
	"$(INTDIR)\paint.obj" \
	"$(INTDIR)\paintdata.obj" \
	"$(INTDIR)\painthud.obj" \
	"$(INTDIR)\paintmap.obj" \
	"$(INTDIR)\paintobjects.obj" \
	"$(INTDIR)\paintradar.obj" \
	"$(INTDIR)\query.obj" \
	"$(INTDIR)\record.obj" \
	"$(INTDIR)\sim.obj" \
	"$(INTDIR)\syslimit.obj" \
	"$(INTDIR)\talkmacros.obj" \
	"$(INTDIR)\textinterface.obj" \
	"$(INTDIR)\texture.obj" \
	"$(INTDIR)\usleep.obj" \
	"$(INTDIR)\widget.obj" \
	"$(INTDIR)\xevent.obj" \
	"$(INTDIR)\xeventhandlers.obj" \
	"$(INTDIR)\xinit.obj" \
	"$(INTDIR)\xpilot.obj" \
	"$(INTDIR)\BSString.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\TalkWindow.obj" \
	"$(INTDIR)\winAbout.obj" \
	"$(INTDIR)\winAudio.obj" \
	"$(INTDIR)\winBitmap.obj" \
	"$(INTDIR)\winConfig.obj" \
	"$(INTDIR)\winXThread.obj" \
	"$(INTDIR)\xpilotDoc.obj" \
	"$(INTDIR)\XPilotNT.obj" \
	"$(INTDIR)\xpilotView.obj" \
	"$(INTDIR)\checknames.obj" \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\math.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\portability.obj" \
	"$(INTDIR)\randommt.obj" \
	"$(INTDIR)\shipshape.obj" \
	"$(INTDIR)\socklib.obj" \
	"$(INTDIR)\strdup.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\xpmemory.obj" \
	"$(INTDIR)\winNet.obj" \
	"$(INTDIR)\winX.obj" \
	"$(INTDIR)\winX11.obj" \
	"$(INTDIR)\winXKey.obj" \
	"$(INTDIR)\wsockerrs.obj" \
	"$(INTDIR)\xpilot.res"

"$(OUTDIR)\xpilot.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "XPilot - Win32 ReleasePentium"

OUTDIR=.\XPilot__
INTDIR=.\XPilot__
# Begin Custom Macros
OutDir=.\XPilot__
# End Custom Macros

ALL : ".\Release\xpilot.exe" "$(OUTDIR)\xpilot.bsc"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\blockbitmaps.obj"
	-@erase "$(INTDIR)\blockbitmaps.sbr"
	-@erase "$(INTDIR)\BSString.obj"
	-@erase "$(INTDIR)\BSString.sbr"
	-@erase "$(INTDIR)\caudio.obj"
	-@erase "$(INTDIR)\caudio.sbr"
	-@erase "$(INTDIR)\checknames.obj"
	-@erase "$(INTDIR)\checknames.sbr"
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\client.sbr"
	-@erase "$(INTDIR)\colors.obj"
	-@erase "$(INTDIR)\colors.sbr"
	-@erase "$(INTDIR)\config.obj"
	-@erase "$(INTDIR)\config.sbr"
	-@erase "$(INTDIR)\configure.obj"
	-@erase "$(INTDIR)\configure.sbr"
	-@erase "$(INTDIR)\datagram.obj"
	-@erase "$(INTDIR)\datagram.sbr"
	-@erase "$(INTDIR)\default.obj"
	-@erase "$(INTDIR)\default.sbr"
	-@erase "$(INTDIR)\error.obj"
	-@erase "$(INTDIR)\error.sbr"
	-@erase "$(INTDIR)\gfx2d.obj"
	-@erase "$(INTDIR)\gfx2d.sbr"
	-@erase "$(INTDIR)\gfx3d.obj"
	-@erase "$(INTDIR)\gfx3d.sbr"
	-@erase "$(INTDIR)\guimap.obj"
	-@erase "$(INTDIR)\guimap.sbr"
	-@erase "$(INTDIR)\guiobjects.obj"
	-@erase "$(INTDIR)\guiobjects.sbr"
	-@erase "$(INTDIR)\join.obj"
	-@erase "$(INTDIR)\join.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\math.obj"
	-@erase "$(INTDIR)\math.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\netclient.obj"
	-@erase "$(INTDIR)\netclient.sbr"
	-@erase "$(INTDIR)\paint.obj"
	-@erase "$(INTDIR)\paint.sbr"
	-@erase "$(INTDIR)\paintdata.obj"
	-@erase "$(INTDIR)\paintdata.sbr"
	-@erase "$(INTDIR)\painthud.obj"
	-@erase "$(INTDIR)\painthud.sbr"
	-@erase "$(INTDIR)\paintmap.obj"
	-@erase "$(INTDIR)\paintmap.sbr"
	-@erase "$(INTDIR)\paintobjects.obj"
	-@erase "$(INTDIR)\paintobjects.sbr"
	-@erase "$(INTDIR)\paintradar.obj"
	-@erase "$(INTDIR)\paintradar.sbr"
	-@erase "$(INTDIR)\portability.obj"
	-@erase "$(INTDIR)\portability.sbr"
	-@erase "$(INTDIR)\query.obj"
	-@erase "$(INTDIR)\query.sbr"
	-@erase "$(INTDIR)\randommt.obj"
	-@erase "$(INTDIR)\randommt.sbr"
	-@erase "$(INTDIR)\record.obj"
	-@erase "$(INTDIR)\record.sbr"
	-@erase "$(INTDIR)\shipshape.obj"
	-@erase "$(INTDIR)\shipshape.sbr"
	-@erase "$(INTDIR)\sim.obj"
	-@erase "$(INTDIR)\sim.sbr"
	-@erase "$(INTDIR)\socklib.obj"
	-@erase "$(INTDIR)\socklib.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\strdup.obj"
	-@erase "$(INTDIR)\strdup.sbr"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\strlcpy.sbr"
	-@erase "$(INTDIR)\syslimit.obj"
	-@erase "$(INTDIR)\syslimit.sbr"
	-@erase "$(INTDIR)\talkmacros.obj"
	-@erase "$(INTDIR)\talkmacros.sbr"
	-@erase "$(INTDIR)\TalkWindow.obj"
	-@erase "$(INTDIR)\TalkWindow.sbr"
	-@erase "$(INTDIR)\textinterface.obj"
	-@erase "$(INTDIR)\textinterface.sbr"
	-@erase "$(INTDIR)\texture.obj"
	-@erase "$(INTDIR)\texture.sbr"
	-@erase "$(INTDIR)\usleep.obj"
	-@erase "$(INTDIR)\usleep.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\widget.obj"
	-@erase "$(INTDIR)\widget.sbr"
	-@erase "$(INTDIR)\winAbout.obj"
	-@erase "$(INTDIR)\winAbout.sbr"
	-@erase "$(INTDIR)\winAudio.obj"
	-@erase "$(INTDIR)\winAudio.sbr"
	-@erase "$(INTDIR)\winBitmap.obj"
	-@erase "$(INTDIR)\winBitmap.sbr"
	-@erase "$(INTDIR)\winConfig.obj"
	-@erase "$(INTDIR)\winConfig.sbr"
	-@erase "$(INTDIR)\winNet.obj"
	-@erase "$(INTDIR)\winNet.sbr"
	-@erase "$(INTDIR)\winX.obj"
	-@erase "$(INTDIR)\winX.sbr"
	-@erase "$(INTDIR)\winX11.obj"
	-@erase "$(INTDIR)\winX11.sbr"
	-@erase "$(INTDIR)\winXKey.obj"
	-@erase "$(INTDIR)\winXKey.sbr"
	-@erase "$(INTDIR)\winXThread.obj"
	-@erase "$(INTDIR)\winXThread.sbr"
	-@erase "$(INTDIR)\wsockerrs.obj"
	-@erase "$(INTDIR)\wsockerrs.sbr"
	-@erase "$(INTDIR)\xevent.obj"
	-@erase "$(INTDIR)\xevent.sbr"
	-@erase "$(INTDIR)\xeventhandlers.obj"
	-@erase "$(INTDIR)\xeventhandlers.sbr"
	-@erase "$(INTDIR)\xinit.obj"
	-@erase "$(INTDIR)\xinit.sbr"
	-@erase "$(INTDIR)\xpilot.obj"
	-@erase "$(INTDIR)\xpilot.res"
	-@erase "$(INTDIR)\xpilot.sbr"
	-@erase "$(INTDIR)\xpilotDoc.obj"
	-@erase "$(INTDIR)\xpilotDoc.sbr"
	-@erase "$(INTDIR)\XPilotNT.obj"
	-@erase "$(INTDIR)\XPilotNT.sbr"
	-@erase "$(INTDIR)\xpilotView.obj"
	-@erase "$(INTDIR)\xpilotView.sbr"
	-@erase "$(INTDIR)\xpmemory.obj"
	-@erase "$(INTDIR)\xpmemory.sbr"
	-@erase "$(OUTDIR)\xpilot.bsc"
	-@erase "$(OUTDIR)\XPilot.map"
	-@erase ".\Release\xpilot.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /MD /W3 /GX /Zd /O2 /I "..\..\common" /I "..\..\common\NT" /D "NDEBUG" /D "_MBCS" /D "x_BETAEXPIRE" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "X_SOUND" /D PAINT_FREE=0 /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\xpilot.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xpilot.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\blockbitmaps.sbr" \
	"$(INTDIR)\caudio.sbr" \
	"$(INTDIR)\client.sbr" \
	"$(INTDIR)\colors.sbr" \
	"$(INTDIR)\configure.sbr" \
	"$(INTDIR)\datagram.sbr" \
	"$(INTDIR)\default.sbr" \
	"$(INTDIR)\gfx2d.sbr" \
	"$(INTDIR)\gfx3d.sbr" \
	"$(INTDIR)\guimap.sbr" \
	"$(INTDIR)\guiobjects.sbr" \
	"$(INTDIR)\join.sbr" \
	"$(INTDIR)\netclient.sbr" \
	"$(INTDIR)\paint.sbr" \
	"$(INTDIR)\paintdata.sbr" \
	"$(INTDIR)\painthud.sbr" \
	"$(INTDIR)\paintmap.sbr" \
	"$(INTDIR)\paintobjects.sbr" \
	"$(INTDIR)\paintradar.sbr" \
	"$(INTDIR)\query.sbr" \
	"$(INTDIR)\record.sbr" \
	"$(INTDIR)\sim.sbr" \
	"$(INTDIR)\syslimit.sbr" \
	"$(INTDIR)\talkmacros.sbr" \
	"$(INTDIR)\textinterface.sbr" \
	"$(INTDIR)\texture.sbr" \
	"$(INTDIR)\usleep.sbr" \
	"$(INTDIR)\widget.sbr" \
	"$(INTDIR)\xevent.sbr" \
	"$(INTDIR)\xeventhandlers.sbr" \
	"$(INTDIR)\xinit.sbr" \
	"$(INTDIR)\xpilot.sbr" \
	"$(INTDIR)\BSString.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\TalkWindow.sbr" \
	"$(INTDIR)\winAbout.sbr" \
	"$(INTDIR)\winAudio.sbr" \
	"$(INTDIR)\winBitmap.sbr" \
	"$(INTDIR)\winConfig.sbr" \
	"$(INTDIR)\winXThread.sbr" \
	"$(INTDIR)\xpilotDoc.sbr" \
	"$(INTDIR)\XPilotNT.sbr" \
	"$(INTDIR)\xpilotView.sbr" \
	"$(INTDIR)\checknames.sbr" \
	"$(INTDIR)\config.sbr" \
	"$(INTDIR)\error.sbr" \
	"$(INTDIR)\math.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\portability.sbr" \
	"$(INTDIR)\randommt.sbr" \
	"$(INTDIR)\shipshape.sbr" \
	"$(INTDIR)\socklib.sbr" \
	"$(INTDIR)\strdup.sbr" \
	"$(INTDIR)\strlcpy.sbr" \
	"$(INTDIR)\xpmemory.sbr" \
	"$(INTDIR)\winNet.sbr" \
	"$(INTDIR)\winX.sbr" \
	"$(INTDIR)\winX11.sbr" \
	"$(INTDIR)\winXKey.sbr" \
	"$(INTDIR)\wsockerrs.sbr"

"$(OUTDIR)\xpilot.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=winmm.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\XPilot.pdb" /map:"$(INTDIR)\XPilot.map" /machine:I386 /out:".\Release\XPilot.exe" 
LINK32_OBJS= \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\blockbitmaps.obj" \
	"$(INTDIR)\caudio.obj" \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\colors.obj" \
	"$(INTDIR)\configure.obj" \
	"$(INTDIR)\datagram.obj" \
	"$(INTDIR)\default.obj" \
	"$(INTDIR)\gfx2d.obj" \
	"$(INTDIR)\gfx3d.obj" \
	"$(INTDIR)\guimap.obj" \
	"$(INTDIR)\guiobjects.obj" \
	"$(INTDIR)\join.obj" \
	"$(INTDIR)\netclient.obj" \
	"$(INTDIR)\paint.obj" \
	"$(INTDIR)\paintdata.obj" \
	"$(INTDIR)\painthud.obj" \
	"$(INTDIR)\paintmap.obj" \
	"$(INTDIR)\paintobjects.obj" \
	"$(INTDIR)\paintradar.obj" \
	"$(INTDIR)\query.obj" \
	"$(INTDIR)\record.obj" \
	"$(INTDIR)\sim.obj" \
	"$(INTDIR)\syslimit.obj" \
	"$(INTDIR)\talkmacros.obj" \
	"$(INTDIR)\textinterface.obj" \
	"$(INTDIR)\texture.obj" \
	"$(INTDIR)\usleep.obj" \
	"$(INTDIR)\widget.obj" \
	"$(INTDIR)\xevent.obj" \
	"$(INTDIR)\xeventhandlers.obj" \
	"$(INTDIR)\xinit.obj" \
	"$(INTDIR)\xpilot.obj" \
	"$(INTDIR)\BSString.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\TalkWindow.obj" \
	"$(INTDIR)\winAbout.obj" \
	"$(INTDIR)\winAudio.obj" \
	"$(INTDIR)\winBitmap.obj" \
	"$(INTDIR)\winConfig.obj" \
	"$(INTDIR)\winXThread.obj" \
	"$(INTDIR)\xpilotDoc.obj" \
	"$(INTDIR)\XPilotNT.obj" \
	"$(INTDIR)\xpilotView.obj" \
	"$(INTDIR)\checknames.obj" \
	"$(INTDIR)\config.obj" \
	"$(INTDIR)\error.obj" \
	"$(INTDIR)\math.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\portability.obj" \
	"$(INTDIR)\randommt.obj" \
	"$(INTDIR)\shipshape.obj" \
	"$(INTDIR)\socklib.obj" \
	"$(INTDIR)\strdup.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\xpmemory.obj" \
	"$(INTDIR)\winNet.obj" \
	"$(INTDIR)\winX.obj" \
	"$(INTDIR)\winX11.obj" \
	"$(INTDIR)\winXKey.obj" \
	"$(INTDIR)\wsockerrs.obj" \
	"$(INTDIR)\xpilot.res"

".\Release\xpilot.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\XPilot__
# End Custom Macros

$(DS_POSTBUILD_DEP) : ".\Release\xpilot.exe" "$(OUTDIR)\xpilot.bsc"
   copy Release\XPilot.exe c:\XPilot
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("xpilot.dep")
!INCLUDE "xpilot.dep"
!ELSE 
!MESSAGE Warning: cannot find "xpilot.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "XPilot - Win32 Release" || "$(CFG)" == "XPilot - Win32 Debug" || "$(CFG)" == "XPilot - Win32 ReleasePentium"
SOURCE=..\about.c

"$(INTDIR)\about.obj"	"$(INTDIR)\about.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\blockbitmaps.c

"$(INTDIR)\blockbitmaps.obj"	"$(INTDIR)\blockbitmaps.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\caudio.c

"$(INTDIR)\caudio.obj"	"$(INTDIR)\caudio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\client.c

"$(INTDIR)\client.obj"	"$(INTDIR)\client.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\colors.c

"$(INTDIR)\colors.obj"	"$(INTDIR)\colors.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\configure.c

"$(INTDIR)\configure.obj"	"$(INTDIR)\configure.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\datagram.c

"$(INTDIR)\datagram.obj"	"$(INTDIR)\datagram.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\default.c

"$(INTDIR)\default.obj"	"$(INTDIR)\default.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\gfx2d.c

"$(INTDIR)\gfx2d.obj"	"$(INTDIR)\gfx2d.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\gfx3d.c

"$(INTDIR)\gfx3d.obj"	"$(INTDIR)\gfx3d.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\guimap.c

"$(INTDIR)\guimap.obj"	"$(INTDIR)\guimap.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\guiobjects.c

"$(INTDIR)\guiobjects.obj"	"$(INTDIR)\guiobjects.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\join.c

"$(INTDIR)\join.obj"	"$(INTDIR)\join.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\netclient.c

"$(INTDIR)\netclient.obj"	"$(INTDIR)\netclient.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\paint.c

"$(INTDIR)\paint.obj"	"$(INTDIR)\paint.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\paintdata.c

"$(INTDIR)\paintdata.obj"	"$(INTDIR)\paintdata.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\painthud.c

"$(INTDIR)\painthud.obj"	"$(INTDIR)\painthud.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\paintmap.c

"$(INTDIR)\paintmap.obj"	"$(INTDIR)\paintmap.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\paintobjects.c

"$(INTDIR)\paintobjects.obj"	"$(INTDIR)\paintobjects.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\paintradar.c

"$(INTDIR)\paintradar.obj"	"$(INTDIR)\paintradar.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\query.c

"$(INTDIR)\query.obj"	"$(INTDIR)\query.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\record.c

"$(INTDIR)\record.obj"	"$(INTDIR)\record.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\sim.c

"$(INTDIR)\sim.obj"	"$(INTDIR)\sim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\syslimit.c

"$(INTDIR)\syslimit.obj"	"$(INTDIR)\syslimit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\talkmacros.c

"$(INTDIR)\talkmacros.obj"	"$(INTDIR)\talkmacros.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\textinterface.c

"$(INTDIR)\textinterface.obj"	"$(INTDIR)\textinterface.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\texture.c

"$(INTDIR)\texture.obj"	"$(INTDIR)\texture.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\usleep.c

"$(INTDIR)\usleep.obj"	"$(INTDIR)\usleep.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\welcome.c
SOURCE=..\widget.c

"$(INTDIR)\widget.obj"	"$(INTDIR)\widget.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\xevent.c

"$(INTDIR)\xevent.obj"	"$(INTDIR)\xevent.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\xeventhandlers.c

"$(INTDIR)\xeventhandlers.obj"	"$(INTDIR)\xeventhandlers.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\xinit.c

"$(INTDIR)\xinit.obj"	"$(INTDIR)\xinit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\xpilot.c

"$(INTDIR)\xpilot.obj"	"$(INTDIR)\xpilot.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\BSString.cpp

"$(INTDIR)\BSString.obj"	"$(INTDIR)\BSString.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj"	"$(INTDIR)\MainFrm.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\RecordDummy.c
SOURCE=.\Splash.cpp

"$(INTDIR)\Splash.obj"	"$(INTDIR)\Splash.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TalkWindow.cpp

"$(INTDIR)\TalkWindow.obj"	"$(INTDIR)\TalkWindow.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\winAbout.cpp

"$(INTDIR)\winAbout.obj"	"$(INTDIR)\winAbout.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\winAudio.c

"$(INTDIR)\winAudio.obj"	"$(INTDIR)\winAudio.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\winBitmap.c

"$(INTDIR)\winBitmap.obj"	"$(INTDIR)\winBitmap.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\winConfig.c

"$(INTDIR)\winConfig.obj"	"$(INTDIR)\winConfig.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\winXThread.c

"$(INTDIR)\winXThread.obj"	"$(INTDIR)\winXThread.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xpilot.rc

"$(INTDIR)\xpilot.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\xpilotDoc.cpp

"$(INTDIR)\xpilotDoc.obj"	"$(INTDIR)\xpilotDoc.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\XPilotNT.cpp

"$(INTDIR)\XPilotNT.obj"	"$(INTDIR)\XPilotNT.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xpilotView.cpp

"$(INTDIR)\xpilotView.obj"	"$(INTDIR)\xpilotView.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\common\checknames.c

"$(INTDIR)\checknames.obj"	"$(INTDIR)\checknames.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\config.c

"$(INTDIR)\config.obj"	"$(INTDIR)\config.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\error.c

"$(INTDIR)\error.obj"	"$(INTDIR)\error.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\math.c

"$(INTDIR)\math.obj"	"$(INTDIR)\math.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\net.c

"$(INTDIR)\net.obj"	"$(INTDIR)\net.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\portability.c

"$(INTDIR)\portability.obj"	"$(INTDIR)\portability.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\randommt.c

"$(INTDIR)\randommt.obj"	"$(INTDIR)\randommt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\shipshape.c

"$(INTDIR)\shipshape.obj"	"$(INTDIR)\shipshape.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\socklib.c

"$(INTDIR)\socklib.obj"	"$(INTDIR)\socklib.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\strdup.c

"$(INTDIR)\strdup.obj"	"$(INTDIR)\strdup.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\strlcpy.c

"$(INTDIR)\strlcpy.obj"	"$(INTDIR)\strlcpy.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\xpmemory.c

"$(INTDIR)\xpmemory.obj"	"$(INTDIR)\xpmemory.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\NT\winNet.c

"$(INTDIR)\winNet.obj"	"$(INTDIR)\winNet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\NT\winX.c

"$(INTDIR)\winX.obj"	"$(INTDIR)\winX.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\NT\winX11.c

"$(INTDIR)\winX11.obj"	"$(INTDIR)\winX11.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\NT\winXKey.c

"$(INTDIR)\winXKey.obj"	"$(INTDIR)\winXKey.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\NT\wsockerrs.c

"$(INTDIR)\wsockerrs.obj"	"$(INTDIR)\wsockerrs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

