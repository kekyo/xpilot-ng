

Summary: 	An sdl client to xpilot the gravity war game
Name: 		xpilot-infinity-sdl
Version: 	4.6.5
Release: 	1
Packager: 	Darel Cullen <darel.cullen@bostream.nu>
Url: 		http://xpilot.sf.net
Source: 	%{name}-%{version}-%{release}.tar.gz
License: 	GPL
Group: 		Amusements/Games
Vendor: 	XPilot
AutoReq: 	0
BuildRequires: 	pkgconfig, expat-devel, zlib-devel, SDL3-devel >= 3.2.0, SDL3_image-devel >= 3.2.0, SDL3_ttf-devel >= 3.2.0, mesa-libGL-devel
Requires: 	expat, SDL3 >= 3.2.0, SDL3_image >= 3.2.0, SDL3_ttf >= 3.2.0, libpng
%description
FEATURES
========

The xpilot polygon (also known as xpilot upgrade) package includes
the following differences from official xpilot.

General:

        * ./configure builds instead of xmkmf -a - easier to build.

        * .rpm .deb and .tar.gz packages available.

        * Full Backward compatability with older xpilot clients, and
          servers. (support dropped for xpilot versions prior to 4.2.1).

        * Numerous bug fixes, and code cleanups / quality improvement.

Client:

        * An XPilot client written using SDL and OpenGL - works in
          both windows/linux and unix with slick new textures, common
          code shared with the X11 client.

Server :

        * Total rewriting of the map format - walls and some other
          features can now be described as arbitrarily shaped polygons.

        * XML based map descriptions, new xp2 map format.

        * The game speed is no longer tied to the number of frames per
          second (FPS). This means that servers can run at higher FPS,
          and the game will still proceed at a reasonable rate, giving
          a smooth game update.

        * New collision code, collisions are handled properly and ships
          cannot get stuck on acute walls anymore.

        * Many new server commands, more server variables can be altered
          online such as gamespeed and fps.

        * Players can now spectate without taking up a base whilst
          pausing.

        * Improved precision in thrusting and shooting..

        * Server side recordings - more accurate, and you can review
          the recording for any player, much smaller and compact than
          the xp-replay recordings.

        * Full integration of teamcup code, run your own xpilot cup.

        * Improved ball handling, better collisions, and the ball is no
          longer treated as a point.

        * better collision detection for laser pulses and shots, added
          an option to make laser pulses bounce off walls.

Tools:

        * A Python map conversion script for converting old style maps
          to new style maps.

        * A Java map editor for the polygon map format, outputs the map
          in XML format.


To start playing, you need to connect to a server by using a client program
called xpilot. There are always servers running if you check with the meta
server, but if you for some reason do not want to join them, you'll have to
start a server of your own (see man-page xpilots(6)).

%post
echo Ensure that you have OpenGL configured, and hardware acceleration
echo otherwise you will find that this client runs too slowly.

%prep
%setup -n %{name}-%{version}-%{release}


%build
./configure --prefix=/usr/local --datadir=/usr/local/share/xpilot-infinity-sdl --enable-sdl-client --enable-sound
make

%install
cd src/client/sdl/
make install
cd ../../..
cd src/server
make install
cd ../..
cd src/replay
make install
cd ../..
cd lib
make install
cd textures
make install

rm -rf $RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc AUTHORS doc/BUGS doc/ChangeLog doc/CREDITS doc/FAQ doc/FIXED FEATURES INSTALL NEWS README TODO
/usr/local/bin/xpilot-infinity-sdl
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/fonts/VeraMoBd.ttf
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/fonts
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/acwise_grav.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/allitems.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/asteroid.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/asteroidconcentrator.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ball.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ball_gray.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ball_gray16.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/base_down.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/base_left.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/base_right.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/base_up.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/bullet.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/bullet2.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/bullet_blue.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/bullet_green.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/cannon_down.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/cannon_left.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/cannon_right.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/cannon_up.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/checkpoint.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/clouds.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/concentrator.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/cwise_grav.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/fuel2.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/fuelcell.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/holder1.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/holder2.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/logo.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/metabtndown.png
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/metabtnup.png
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/metalite.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/meter.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/mine_other.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/mine_team.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/minus.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/missile.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/moon.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/paused.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/plus.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/radar.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/radar2.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/radar3.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/refuel.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/rock4.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/sdlmetabg.png
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/shield.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ship.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ship_blue.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ship_red.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ship_red2.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/ship_red3.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/sparks.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/volcanic.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_bottom.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_dl.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_dr.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_fi.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_left.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_right.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_top.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_ul.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_ull.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_ur.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wall_url.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/wormhole.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures/asteroid.ppm
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/textures
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/bfire.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/bloop.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/buzzer.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/confirm.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/corkpop.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/done.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/doorchimes.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/Drop01.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/explo2.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/failure.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/finished.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/flare.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/gundead.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/launch1.18.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/lboom.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/magic.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/MusicaDefault.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/popclick.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/RobotzDefault.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/RobotzQuestion.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/sboom.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/sbounce.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/sounds.txt
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/sstart.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/success.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/UtopiaClose.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/waiting.wav
/usr/local/share/xpilot-infinity-sdl/xpilot-infinity/sound/zap.wav


%changelog
* Sun Jun 6 2004 Darel <darelcullen@sf.net>
- Split into sdl and x11, add sound lots of other stuff
* Mon Sep 15 2003 Darel <darelcullen@sf.net>
- Initial build.
