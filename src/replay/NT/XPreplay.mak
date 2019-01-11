# Microsoft Developer Studio Generated NMAKE File, Based on XPreplay.dsp
!IF "$(CFG)" == ""
CFG=XPreplay - Win32 Debug
!MESSAGE No configuration specified. Defaulting to XPreplay - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "XPreplay - Win32 Release" && "$(CFG)" != "XPreplay - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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

!IF  "$(CFG)" == "XPreplay - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\XPreplay.exe"


CLEAN :
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\Properties.obj"
	-@erase "$(INTDIR)\SliderBar.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\XPreplay.obj"
	-@erase "$(INTDIR)\XPreplay.pch"
	-@erase "$(INTDIR)\XPreplay.res"
	-@erase "$(INTDIR)\XPreplayDoc.obj"
	-@erase "$(INTDIR)\XPreplayView.obj"
	-@erase "$(OUTDIR)\XPreplay.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\XPreplay.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x413 /fo"$(INTDIR)\XPreplay.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XPreplay.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\XPreplay.pdb" /machine:I386 /out:"$(OUTDIR)\XPreplay.exe" 
LINK32_OBJS= \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\Properties.obj" \
	"$(INTDIR)\SliderBar.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XPreplay.obj" \
	"$(INTDIR)\XPreplayDoc.obj" \
	"$(INTDIR)\XPreplayView.obj" \
	"$(INTDIR)\XPreplay.res"

"$(OUTDIR)\XPreplay.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "XPreplay - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\XPreplay.exe"


CLEAN :
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\Properties.obj"
	-@erase "$(INTDIR)\SliderBar.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\XPreplay.obj"
	-@erase "$(INTDIR)\XPreplay.pch"
	-@erase "$(INTDIR)\XPreplay.res"
	-@erase "$(INTDIR)\XPreplayDoc.obj"
	-@erase "$(INTDIR)\XPreplayView.obj"
	-@erase "$(OUTDIR)\XPreplay.exe"
	-@erase "$(OUTDIR)\XPreplay.ilk"
	-@erase "$(OUTDIR)\XPreplay.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\XPreplay.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x413 /fo"$(INTDIR)\XPreplay.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XPreplay.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\XPreplay.pdb" /debug /machine:I386 /out:"$(OUTDIR)\XPreplay.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\Properties.obj" \
	"$(INTDIR)\SliderBar.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XPreplay.obj" \
	"$(INTDIR)\XPreplayDoc.obj" \
	"$(INTDIR)\XPreplayView.obj" \
	"$(INTDIR)\XPreplay.res"

"$(OUTDIR)\XPreplay.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

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
!IF EXISTS("XPreplay.dep")
!INCLUDE "XPreplay.dep"
!ELSE 
!MESSAGE Warning: cannot find "XPreplay.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "XPreplay - Win32 Release" || "$(CFG)" == "XPreplay - Win32 Debug"
SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"


SOURCE=.\Properties.cpp

"$(INTDIR)\Properties.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"


SOURCE=.\SliderBar.cpp

"$(INTDIR)\SliderBar.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "XPreplay - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\XPreplay.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\XPreplay.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XPreplay - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\XPreplay.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\XPreplay.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\XPreplay.cpp

"$(INTDIR)\XPreplay.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"


SOURCE=.\XPreplay.rc

"$(INTDIR)\XPreplay.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\XPreplayDoc.cpp

"$(INTDIR)\XPreplayDoc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"


SOURCE=.\XPreplayView.cpp

"$(INTDIR)\XPreplayView.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XPreplay.pch"



!ENDIF 

