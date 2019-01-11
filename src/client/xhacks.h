/*
 * XPilot NG, a multiplayer space war game
 *
 * Copyright (C) 2005 by
 *
 *      Lars Dieﬂelberg       <marvn@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#ifndef XHACKS_H
#define XHACKS_H

#ifdef HAVE_XF86MISC
#include <X11/Xlib.h>
#include <X11/extensions/xf86misc.h>
#endif

static void Disable_emulate3buttons(bool disable, void* display);


/*
 * If disable==true, the function tries to deactivate the Emulate3Buttons
 * X server option if it was enabled. On some systems the request for setting
 * the Emulate3Buttons option to false is ignored, it tries only setting the 
 * Emulate3Buttons-timeout to 0 then. In any case, it prints a warning for the
 * user. When disable==false is specified, the system's settings are reset
 * to its original state.
 */ 
static void Disable_emulate3buttons(bool disable, void* display)
{

/* according to reports XF86MiscGetMouseSettings crashes the client on more 
   recent Xorg servers - so I'm disabling the whole function for now.
   Will test myself whenever I upgrade to xorg8 myself KHS  */
return;

#ifdef HAVE_XF86MISC
#if 1
    /* kps - Lets not try to disable emulate3buttons. Some users have buggy
     * XF86 implementations which can't enable the disabled emulation
     * when the client quits.
     */

    XF86MiscMouseSettings m;
    Status status;

    if (!disable)
	return;

    status = XF86MiscGetMouseSettings((Display*) display, &m);
    if (status != 1) {
	warn("Failed to retrieve mouse settings from X server.");
	return;
    }

    if (m.emulate3buttons) {
	warn("*** Emulate3Buttons is enabled.");
	warn("*** This may cause lost and/or laggy mouse button events.");
	warn("*** More info at: http://xpilot.sourceforge.net/faq.html");
	return;
    }
#else
/*#define XF86DEBUG*/
    static bool first_run = true;
    static bool working = true;
    static bool already_warned = false;
    static int orig_timeout;
    XF86MiscMouseSettings m;
    Status status;
        
    if (!working) return;
    
    status = XF86MiscGetMouseSettings((Display*) display, &m);
    if (status != 1) {
	warn("Failed to retrieve mouse settings from X server.");
	working = false;
	return;
    }

    #ifdef XF86DEBUG
	warn("--- 1st get ---");
	warn("status          : %d", status);
	warn("path to device  : %s", m.device);
	warn("type            : %d", m.type);
	warn("baudrate        : %d", m.baudrate);
	warn("samplerate      : %d", m.samplerate);
	warn("resolution      : %d", m.resolution);
	warn("buttons         : %d", m.buttons);
	warn("emulate3buttons : %d", m.emulate3buttons);
	warn("emulate3timeout : %d", m.emulate3timeout);
	warn("chordmiddle     : %d", m.chordmiddle);
	warn("flags           : 0x%x", m.flags);
    #endif

    if (first_run) {
    	if (m.emulate3buttons) {
	    warn("*** Warning: Emulate3Buttons is enabled.");
	    orig_timeout = m.emulate3timeout;
	} else {
	    working = false; /* Emulate3Buttons disabled from the start, so function is turned inactive */
	    return;
	}
    }
    
    m.emulate3buttons = !disable;
    m.emulate3timeout = disable ? 0 : orig_timeout;

    #ifdef XF86DEBUG
	warn("--- set ---");
	warn("wanted 3buttons : %d", m.emulate3buttons);
	warn("wanted timeout  : %d", m.emulate3timeout);
    #endif
	
    status = XF86MiscSetMouseSettings((Display*) display, &m);
    if (status != 1) {
	warn("*** Warning: Failed to set X server mouse settings. Fix your X configuration, please.");
	working = false;
	return;
    }
    
    #ifdef XF86DEBUG
	warn("status          : %d", status);
    #endif
    
    XF86MiscGetMouseSettings((Display*) display, &m);
    
    #ifdef XF86DEBUG
	warn("--- 2nd get ---");
	warn("status          : %d", status);
	warn("path to device  : %s", m.device);
	warn("type            : %d", m.type);
	warn("baudrate        : %d", m.baudrate);
	warn("samplerate      : %d", m.samplerate);
	warn("resolution      : %d", m.resolution);
	warn("buttons         : %d", m.buttons);
	warn("emulate3buttons : %d", m.emulate3buttons);
	warn("emulate3timeout : %d", m.emulate3timeout);
	warn("chordmiddle     : %d", m.chordmiddle);
	warn("flags           : 0x%x", m.flags);
    #endif
    
    if (m.emulate3buttons != (!disable) && !already_warned) {
	warn("*** Warning: Failed to disable Emulate3Buttons. Just setting timeout to 0.");
	already_warned = true;
	if (m.emulate3timeout != (disable ? 0 : orig_timeout)) {
	    warn("*** Warning: Can't set timeout. Giving up...");
	    working = false;
	}
    }
    
    first_run = false;

#endif
#endif
}

#endif
