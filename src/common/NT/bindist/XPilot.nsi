# XPilot.nsi - the script to NSIS, the NullSoft Install System.
# $Id: XPilot.nsi,v 1.1 2003/04/10 12:24:31 kps Exp $
#              Copyright 2001 Jarno van der Kolk <jarno@j-a-r-n-o.nl>
#              Released under GNU General Public License Version 2 
# The NullSoft Install System can be found here http://www.nullsoft.com/free/nsis/
#
# Header stuff
Name "XPilot"
Icon "xpilot.ico"
#OutFile XPilot-440NT14.exe
!include outfilename.txt

# License text seems to be next
LicenseText "XPilot works under the GPL license"
LicenseData "License.txt"

# Component page configuration commands
InstType "Typical"
InstType "Compact"
EnabledBitmap "xp_on.bmp"
DisabledBitmap "xp_off.bmp"
ComponentText "This will install [RELEASE] on your computer."

# Directory selection page configuration commands
InstallDir "C:\XPilot"
DirText "Select the directory to install [RELEASE] in"
DirShow show
InstallDirRegKey HKEY_LOCAL_MACHINE SOFTWARE\XPilot "Install_Dir"

# Install page configuration commands
AutoCloseWindow false

# Installation execution commands
# This seems to be the real stuff. :)

Section "Essential game files"
SetOutPath "$INSTDIR"
CreateDirectory "$INSTDIR\doc"
CreateDirectory "$INSTDIR\lib"
CreateDirectory "$INSTDIR\lib\maps"
CreateDirectory "$INSTDIR\lib\textures"
IfFileExists "$INSTDIR\XPilot.shp" noshp
File "XPilot.shp"
noshp:
IfFileExists "$INSTDIR\XPilot.ini" noini
File "XPilot.ini"
noini:
File "License.txt"
File "README.txt"
File "XPilot.exe"
File "XPilotServer.exe"
File "XPwho.exe"
File "XPreplay.exe"
File "XPreplay.reg"
File "msvcr70.dll"
SetOutPath "$INSTDIR\doc"
#File "doc\Bugs.txt"
File "doc\ChangeLog.txt"
#File "doc\ClientOpts.txt"
File "doc\Credits.txt"
File "doc\Faq.txt"
#File "doc\Fixed.txt"
File "doc\README.MAPS.txt"
File "doc\README.MAPS2.txt"
File "doc\README.SHIPS.txt"
File "doc\README.talkmacros.txt"
#File "doc\ServerDump.txt"
File "doc\ServerOpts.txt"
File "doc\Todo.txt"
File "doc\The XPilot Page.url"
File "doc\Newbie Guide.url"
SetOutPath "$INSTDIR\lib"
File "lib\defaults.txt"
File "lib\robots.txt"
SetOutPath "$INSTDIR\lib\textures"
File "lib\textures\allitems.ppm"
File "lib\textures\asteroidconcentrator.ppm"
File "lib\textures\ball.ppm"
File "lib\textures\base_down.ppm"
File "lib\textures\base_left.ppm"
File "lib\textures\base_right.ppm"
File "lib\textures\base_up.ppm"
File "lib\textures\bullet.ppm"
File "lib\textures\bullet_blue.ppm"
File "lib\textures\bullet_green.ppm"
File "lib\textures\bullet2.ppm"
File "lib\textures\cannon_down.ppm"
File "lib\textures\cannon_left.ppm"
File "lib\textures\cannon_right.ppm"
File "lib\textures\cannon_up.ppm"
File "lib\textures\checkpoint.ppm"
File "lib\textures\clouds.ppm"
File "lib\textures\concentrator.ppm"
File "lib\textures\fuel2.ppm"
File "lib\textures\fuelcell.ppm"
File "lib\textures\holder1.ppm"
File "lib\textures\holder2.ppm"
File "lib\textures\logo.ppm"
File "lib\textures\meter.ppm"
File "lib\textures\mine_other.ppm"
File "lib\textures\mine_team.ppm"
File "lib\textures\minus.ppm"
File "lib\textures\paused.ppm"
File "lib\textures\plus.ppm"
File "lib\textures\radar.ppm"
File "lib\textures\radar2.ppm"
File "lib\textures\radar3.ppm"
File "lib\textures\refuel.ppm"
File "lib\textures\ship.ppm"
File "lib\textures\ship_blue.ppm"
File "lib\textures\ship_red.ppm"
File "lib\textures\ship_red2.ppm"
File "lib\textures\ship_red3.ppm"
File "lib\textures\sparks.ppm"
File "lib\textures\wall_bottom.ppm"
File "lib\textures\wall_dl.ppm"
File "lib\textures\wall_dr.ppm"
File "lib\textures\wall_fi.ppm"
File "lib\textures\wall_left.ppm"
File "lib\textures\wall_right.ppm"
File "lib\textures\wall_top.ppm"
File "lib\textures\wall_ul.ppm"
File "lib\textures\wall_ull.ppm"
File "lib\textures\wall_ur.ppm"
File "lib\textures\wall_url.ppm"
File "lib\textures\wormhole.ppm"

SetOutPath "$INSTDIR"
WriteUninstaller "uninstall.exe"

# Write the installation path into the registry
WriteRegStr HKEY_LOCAL_MACHINE SOFTWARE\XPilot "Install_Dir" "$INSTDIR"

# The stuff for XPwho
WriteRegStr HKEY_CURRENT_USER SOFTWARE\BuckoSoft\XPwho\Settings "XPilot App" "$INSTDIR\XPilot.exe"

# Write the uninstall keys for Windows
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\XPilot" "DisplayName" "XPilot"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\XPilot" "UninstallString" '"$INSTDIR\uninstall.exe"'

SectionEnd

# Jarrod's mapeditor
Section "Mapeditor"
SectionIn 1
CreateDirectory "$INSTDIR\mapXpress"
SetOutPath "$INSTDIR\mapXpress"
File "mapXpress\mapXpress.exe"
File "mapXpress\mapXpress.chm"
File "mapXpress\Changelog.txt"
File "mapXpress\Copyright.txt"
File "mapXpress\License.txt"
File "mapXpress\readme.txt"
File "mapXpress\todo.txt"
SectionEnd

# Lewis' shipeditor
Section "Shipeditor"
SectionIn 1
CreateDirectory "$INSTDIR\XPShipEditor"
SetOutPath "$INSTDIR\XPShipEditor"
File "XPShipEditor\XPShipEditor.txt"
File "XPShipEditor\XPShipEditor.exe"
SectionEnd

# Default maps take in half-a-meg!
Section "Mappack"
SectionIn 1
SetOutPath "$INSTDIR\lib\maps"
File "lib\maps\blood-music2.xp"
File "lib\maps\CAMD.xp"
File "lib\maps\cloudscape.xp"
File "lib\maps\default.xp"
File "lib\maps\doggy.xp"
File "lib\maps\fireball.xp"
File "lib\maps\fuzz.xp"
File "lib\maps\fuzz2.xp"
File "lib\maps\globe.xp"
File "lib\maps\grandprix.xp"
File "lib\maps\newdarkhell.xp"
File "lib\maps\newdarkhell2.xp"
File "lib\maps\pad.xp"
File "lib\maps\pit.xp"
File "lib\maps\planetx.xp"
File "lib\maps\teamball.xp"
File "lib\maps\tourmination.xp"
File "lib\maps\tournament.xp"
File "lib\maps\war.xp"
SectionEnd

# Now to make some lovely shortcuts in the already oh so crowded Startmenu.
Section "Start Menu Shortcuts"
SectionIn 12
SetOutPath "$INSTDIR"
CreateDirectory "$SMPROGRAMS\XPilot"
CreateDirectory "$SMPROGRAMS\XPilot\doc"
CreateShortCut "$SMPROGRAMS\XPilot\XPilot.lnk" "$INSTDIR\XPilot.exe" "" "$INSTDIR\XPilot.exe" 0
CreateShortCut "$SMPROGRAMS\XPilot\XPilot.ini.lnk" "$INSTDIR\XPilot.ini" "" "$INSTDIR\XPilot.ini" 0
CreateShortCut "$SMPROGRAMS\XPilot\XPilotServer.lnk" "$INSTDIR\XPilotServer.exe" "" "$INSTDIR\XPilotServer.exe" 0
CreateShortCut "$SMPROGRAMS\XPilot\XPwho.lnk" "$INSTDIR\XPwho.exe" "" "$INSTDIR\XPwho.exe" 0
CreateShortCut "$SMPROGRAMS\XPilot\XPreplay.lnk" "$INSTDIR\XPreplay.exe" "" "$INSTDIR\XPreplay.exe" 0
CreateShortCut "$SMPROGRAMS\XPilot\doc\README.lnk" "$INSTDIR\README.txt" ""
CreateShortCut "$SMPROGRAMS\XPilot\doc\License.lnk" "$INSTDIR\License.txt" ""
CreateShortCut "$SMPROGRAMS\XPilot\doc\ChangeLog.lnk" "$INSTDIR\doc\ChangeLog.txt" ""
CreateShortCut "$SMPROGRAMS\XPilot\doc\Server Settings.lnk" "$INSTDIR\doc\ServerOpts.txt" ""
CreateShortCut "$SMPROGRAMS\XPilot\doc\The XPilot Page.lnk" "$INSTDIR\doc\The XPilot Page.url" ""
CreateShortCut "$SMPROGRAMS\XPilot\doc\Newbie Guide.lnk" "$INSTDIR\doc\Newbie Guide.url" ""

IfFileExists $INSTDIR\XPShipEditor 0 skipShipEditor
CreateShortCut "$SMPROGRAMS\XPilot\XPShipEditor.lnk" "$INSTDIR\XPShipEditor\XPShipEditor.exe" "" "$INSTDIR\XPShipEditor\XPShipEditor.exe" 0
skipShipEditor:

IfFileExists $INSTDIR\MapEditor 0 skipMapEditor
CreateShortCut "$SMPROGRAMS\XPilot\MapXpress.lnk" "$INSTDIR\mapXpress\MapXpress.exe" "" "$INSTDIR\mapXpress\MapXpress.exe" 0
skipMapEditor:

#put uninstall at the bottom
CreateShortCut "$SMPROGRAMS\XPilot\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

##############################################################################
# Uninstall stuff. Sad but true, some people actually uninstall XPilot...

; special uninstall section.
Section "Uninstall"
MessageBox MB_YESNO|MB_ICONEXCLAMATION "Press 'Yes' to remove XPilot from your system" IDYES removeYes IDNO removeNo
Goto removeNo

removeYes:
; remove registry keys
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\XPilot"
DeleteRegKey HKEY_LOCAL_MACHINE SOFTWARE\XPilot
DeleteRegKey HKEY_CURRENT_USER SOFTWARE\BuckoSoft\XPilot

; remove files
Delete $INSTDIR\doc\*.*
Delete $INSTDIR\lib\*.*
Delete $INSTDIR\lib\maps\*.*
Delete $INSTDIR\lib\textures\*.*
Delete $INSTDIR\mapXpress\*.*
Delete $INSTDIR\XPShipEditor\*.*
Delete $INSTDIR\*.*

; remove shortcuts, if any.
Delete "$SMPROGRAMS\XPilot\*.*"
Delete "$SMPROGRAMS\XPilot\doc\*.*"
; remove directories used.
RMDir "$SMPROGRAMS\XPilot\doc"
RMDir "$SMPROGRAMS\XPilot"
RMDir "$INSTDIR\XPShipEditor"
RMDir "$INSTDIR\mapXpress"
RMDir "$INSTDIR\lib\textures"
RMDir "$INSTDIR\lib\maps"
RMDir "$INSTDIR\lib"
RMDir "$INSTDIR\doc"
RMDir "$INSTDIR"

removeNo:
SectionEnd


Function .onInstSuccess	
MessageBox MB_YESNO|MB_ICONINFORMATION "XPilot installed successfully.  \
  Would you like to connect to the Internet to join a game?  \
  You can do this later by running XPWho." IDNO NoXPwho
Exec $INSTDIR\XPWho.exe
NoXPwho:

MessageBox MB_YESNO "You can also start your own server and then connect to that.  \
  Would you like to start a server on this machine?  \
  You can do this later by running XPilotServer." IDNO NoServer
Exec $INSTDIR\XPilotServer.exe
NoServer:
FunctionEnd

; eof
