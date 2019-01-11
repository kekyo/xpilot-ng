/*
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2004 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
 *      Erik Andersson       <maximan@users.sourceforge.net>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#include "xpclient_x11.h"

int hudColor;		/* Color index for HUD drawing, has to be global for windoze */
static int hudHLineColor;	/* Color index for horiz. HUD line drawing */
static int hudVLineColor;	/* Color index for vert. HUD line drawing */
static int hudItemsColor;	/* Color index for HUD items drawing */
static int hudRadarEnemyColor;	/* Color index for enemy hudradar dots */
static int hudRadarOtherColor;	/* Color index for other hudradar dots */
static int hudLockColor;	/* Color index for lock on HUD drawing */
static int fuelGaugeColor;	/* Color index for fuel gauge drawing */
static int dirPtrColor;		/* Color index for dirptr drawing */
static int messagesColor;	/* Color index for messages */
static int oldMessagesColor;	/* Color index for old messages */
static int msgScanBallColor;	/* Color index for ball msg */
static int msgScanSafeColor;	/* Color index for safe msg */
static int msgScanCoverColor;	/* Color index for cover msg */
static int msgScanPopColor;	/* Color index for pop msg */
static int fuelMeterColor;	/* Color index for fuel meter */
static int powerMeterColor;	/* Color index for power meter */
static int turnSpeedMeterColor;	/* Color index for turnspeed meter */
static int packetSizeMeterColor;/* Color index for packet size meter */
static int packetLossMeterColor;/* Color index for packet loss meter */
static int packetDropMeterColor;/* Color index for packet drop meter */
static int packetLagMeterColor;	/* Color index for packet lag meter */
static int temporaryMeterColor;	/* Color index for temporary meter drawing */
static int meterBorderColor;	/* Color index for meter border drawing */
static int scoreObjectColor;	/* Color index for map score objects */

radar_t	*old_radar_ptr;
int	old_num_radar, old_max_radar;

static int meterWidth = 60;
static int meterHeight = 10;

/*
 * Draw a meter of some kind on screen.
 * When the x-offset is specified as a negative value then
 * the meter is drawn relative to the right side of the screen,
 * otherwise from the normal left side.
 */
static void Paint_meter(int xoff, int y, const char *title, int val, int max,
			int meter_color)
{
    const int	mw1_4 = meterWidth/4,
		mw2_4 = meterWidth/2,
		mw3_4 = 3*meterWidth/4,
		mw4_4 = meterWidth,
		BORDER = 5;
    int		x, xstr;

    if (xoff >= 0) {
	x = xoff;
        xstr = WINSCALE(x + (int)meterWidth) + BORDER;
    } else {
	x = ext_view_width - ((int)meterWidth - xoff);
        xstr = WINSCALE(x)
	    - (BORDER + XTextWidth(gameFont, title, (int)strlen(title)));
    }

    Rectangle_add(meter_color,
		  x+2, y+2,
		  (int)(((meterWidth-3)*val)/(max?max:1)), meterHeight-3);

    /* meterBorderColor = 0 obviously means no meter borders are drawn */
    if (meterBorderColor) {
	int color = meterBorderColor;

	SET_FG(colors[color].pixel);
	rd.drawRectangle(dpy, drawPixmap, gameGC,
			 WINSCALE(x), WINSCALE(y),
			 UWINSCALE(meterWidth), UWINSCALE(meterHeight));

	/* Paint scale levels(?) */
	Segment_add(color, x,       y-4,	x,       y+meterHeight+4);
	Segment_add(color, x+mw4_4, y-4,	x+mw4_4, y+meterHeight+4);
	Segment_add(color, x+mw2_4, y-3,	x+mw2_4, y+meterHeight+3);
	Segment_add(color, x+mw1_4, y-1,	x+mw1_4, y+meterHeight+1);
	Segment_add(color, x+mw3_4, y-1,	x+mw3_4, y+meterHeight+1);
    }

    if (!meterBorderColor)
	SET_FG(colors[meter_color].pixel);

    rd.drawString(dpy, drawPixmap, gameGC,
		  xstr, WINSCALE(y)+(gameFont->ascent+meterHeight)/2,
		  title, (int)strlen(title));

    /* texturedObjects - TODO */
    /*int width = WINSCALE((int)(((meterWidth-3)*val)/(max?max:1)));*/

    /*printf("TODO: implement paint meter\n");*/
    /*PaintMeter(drawPixmap, BM_METER,
      WINSCALE(x), WINSCALE(y),
      WINSCALE(meterWidth), WINSCALE(11),
      width);*/
    /*SET_FG(colors[color].pixel);*/
}


static int wrap(int *xp, int *yp)
{
    int			x = *xp, y = *yp;

    if (x < world.x || x > world.x + ext_view_width) {
	if (x < realWorld.x || x > realWorld.x + ext_view_width)
	    return 0;
	*xp += world.x - realWorld.x;
    }
    if (y < world.y || y > world.y + ext_view_height) {
	if (y < realWorld.y || y > realWorld.y + ext_view_height)
	    return 0;
	*yp += world.y - realWorld.y;
    }
    return 1;
}


void Paint_score_objects(void)
{
    int		i, x, y;

    if (!scoreObjectColor)
	return;

    for (i = 0; i < MAX_SCORE_OBJECTS; i++) {
	score_object_t*	sobj = &score_objects[i];
	if (sobj->life_time > 0) {
	    if (loopsSlow % 3) {
		x = sobj->x * BLOCK_SZ + BLOCK_SZ/2;
		y = sobj->y * BLOCK_SZ + BLOCK_SZ/2;
		if (wrap(&x, &y)) {
		    if (sobj->msg_width == -1)
			sobj->msg_width = 
			    XTextWidth(gameFont, sobj->msg, sobj->msg_len);
		    SET_FG(colors[scoreObjectColor].pixel);
		    x = WINSCALE(X(x)) - sobj->msg_width / 2,
		    y = WINSCALE(Y(y)) + gameFont->ascent / 2,
		    rd.drawString(dpy, drawPixmap, gameGC,
				x, y,
				sobj->msg,
				sobj->msg_len);
		}
	    }
	    sobj->life_time -= timePerFrame;
	    if (sobj->life_time <= 0.0) {
		sobj->life_time = 0.0;
		sobj->hud_msg_len = 0;
	    }
	}
    }
}


void Paint_meters(void)
{
    int y = 20, color;

    if (fuelMeterColor)
	Paint_meter(-10, y += 20, "Fuel",
		    (int)fuelSum, (int)fuelMax, fuelMeterColor);

    if (powerMeterColor)
	color = powerMeterColor;
    else if (controlTime > 0.0)
	color = temporaryMeterColor;
    else
	color = 0;

    if (color)
	Paint_meter(-10, y += 20, "Power",
		    (int)displayedPower, (int)MAX_PLAYER_POWER, color);

    if (turnSpeedMeterColor)
	color = turnSpeedMeterColor;
    else if (controlTime > 0.0)
	color = temporaryMeterColor;
    else
	color = 0;

    if (color)
	Paint_meter(-10, y += 20, "Turnspeed",
		    (int)displayedTurnspeed, (int)MAX_PLAYER_TURNSPEED, color);

    if (controlTime > 0.0) {
	controlTime -= timePerFrame;
	if (controlTime <= 0.0)
	    controlTime = 0.0;
    }

    if (packetSizeMeterColor)
	Paint_meter(-10, y += 20, "Packet",
		   (packet_size >= 4096) ? 4096 : packet_size, 4096,
		    packetSizeMeterColor);
    if (packetLossMeterColor)
	Paint_meter(-10, y += 20, "Loss", packet_loss, FPS,
		    packetLossMeterColor);
    if (packetDropMeterColor)
	Paint_meter(-10, y += 20, "Drop", packet_drop, FPS,
		    packetDropMeterColor);
    if (packetLagMeterColor)
	Paint_meter(-10, y += 20, "Lag", MIN(packet_lag, 1 * FPS), 1 * FPS,
		    packetLagMeterColor);

    if (temporaryMeterColor) {
	if (thrusttime >= 0 && thrusttimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3,
			"Thrust Left",
			(thrusttime >= thrusttimemax
			 ? thrusttimemax : thrusttime),
			thrusttimemax, temporaryMeterColor);

	if (shieldtime >= 0 && shieldtimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 20,
			"Shields Left",
			(shieldtime >= shieldtimemax
			 ? shieldtimemax : shieldtime),
			shieldtimemax, temporaryMeterColor);

	if (phasingtime >= 0 && phasingtimemax > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 40,
			"Phasing left",
			(phasingtime >= phasingtimemax
			 ? phasingtimemax : phasingtime),
			phasingtimemax, temporaryMeterColor);

	if (destruct > 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 60,
			"Self destructing", destruct, (int)SELF_DESTRUCT_DELAY,
			temporaryMeterColor);

	if (shutdown_count >= 0)
	    Paint_meter((ext_view_width-300)/2 -32, 2*ext_view_height/3 + 80,
			"SHUTDOWN", shutdown_count, shutdown_delay,
			temporaryMeterColor);
    }
}


static void Paint_lock(int hud_pos_x, int hud_pos_y)
{
    const int	BORDER = 2;
    int		x, y;
    other_t	*target;
    char	str[50];
    static int	mapdiag = 0;

    if (mapdiag == 0)
	mapdiag = LENGTH(Setup->width, Setup->height);

    /*
     * Display direction arrow and miscellaneous target information.
     */
    if ((target = Other_by_id(lock_id)) == NULL)
	return;

    if (hudColor) {
	int color = Life_color(target);

	if (!color)
	    color = hudColor;
	SET_FG(colors[color].pixel);

	Check_name_string(target);
	rd.drawString(dpy, drawPixmap, gameGC,
		      WINSCALE(hud_pos_x) - target->name_width / 2,
		      WINSCALE(hud_pos_y - hudSize + HUD_OFFSET - BORDER )
		      - gameFont->descent ,
		      target->id_string, target->name_len);

	/* lives left is a better info than distance in team games MM */
	if (BIT(Setup->mode, LIMITED_LIVES))
	    sprintf(str, "%03d", target->life);
	else
	    sprintf(str, "%03d", lock_dist / BLOCK_SZ);

	if (BIT(Setup->mode, LIMITED_LIVES) || lock_dist !=0) {
	    if (BIT(Setup->mode, LIMITED_LIVES) && target->life == 0)
		SET_FG(colors[RED].pixel);
	    else
		SET_FG(colors[hudColor].pixel);

	    rd.drawString(dpy, drawPixmap, gameGC,
			  WINSCALE(hud_pos_x + hudSize - HUD_OFFSET + BORDER),
			  WINSCALE(hud_pos_y - hudSize + HUD_OFFSET - BORDER)
			  - gameFont->descent,
			  str, 3);
	}
    }

    if (lock_dist != 0 && hudLockColor) {
	if (lock_dist > WARNING_DISTANCE || (loopsSlow & 1)) {
	    int size = MIN(mapdiag / lock_dist, 10);

	    if (size == 0)
		size = 1;

	    if (self != NULL
		&& ((self->team == target->team
		     && BIT(Setup->mode, TEAM_PLAY))
		    || (self->alliance != ' '
			&& self->alliance == target->alliance))) {
		Arc_add(BLUE,
			(int)(hud_pos_x + MIN_HUD_SIZE * 0.6 * tcos(lock_dir)
			      - size * 0.5),
			(int)(hud_pos_y - MIN_HUD_SIZE * 0.6 * tsin(lock_dir)
			      - size * 0.5),
			size, size, 0, 64*360);
	    } else {
		SET_FG(colors[hudLockColor].pixel);
		x = (int)(hud_pos_x + MIN_HUD_SIZE * 0.6 * tcos(lock_dir)
			  - size * 0.5),
		y = (int)(hud_pos_y - MIN_HUD_SIZE * 0.6 * tsin(lock_dir)
			  - size * 0.5),
		rd.fillArc(dpy, drawPixmap, gameGC,
			   WINSCALE(x), WINSCALE(y),
			   UWINSCALE(size), UWINSCALE(size), 0, 64*360);
	    }
	}
    }
}

static void Paint_hudradar(double hrscale, double xlimit, double ylimit,
			   int sz)
{
    int i, x, y;
    int hrw = hrscale * 256;
    int hrh = hrscale * RadarHeight;
    double xf = (double) hrw / (double) Setup->width;
    double yf = (double) hrh / (double) Setup->height;

    for (i = 0; i < num_radar; i++) {
	x = radar_ptr[i].x * hrscale
	    - (world.x + ext_view_width / 2) * xf;
	y = radar_ptr[i].y * hrscale
	    - (world.y + ext_view_height / 2) * yf;

	if (x < -hrw / 2)
	    x += hrw;
	else if (x > hrw / 2)
	    x -= hrw;

	if (y < -hrh / 2)
	    y += hrh;
	else if (y > hrh / 2)
	    y -= hrh;

	if (!((x <= xlimit) && (x >= -xlimit)
	      && (y <= ylimit) && (y >= -ylimit))) {

 	    x = x + ext_view_width / 2 - sz / 2;
 	    y = -y + ext_view_height / 2 - sz / 2;

	    if (radar_ptr[i].type == normal) {
		if (hudRadarEnemyColor >= 1)
		    Arc_add(hudRadarEnemyColor, x, y, sz, sz, 0, 64 * 360);
	    } else {
		if (hudRadarOtherColor >= 1)
		    Arc_add(hudRadarOtherColor, x, y, sz, sz, 0, 64 * 360);
	    }
	}
    }
}



/*called from guimap for basewarning */
/*void Paint_baseInfoOnHudRadar(int xi, int yi)*/
/*{
   float x,y,x2,y2,x3,y3;
	float hrscale = hrScale;
	float hrw = (float)hrscale * (float)256;
	float hrh = (float)hrscale * (float)RadarHeight;
	int sz = hrSize;
	float xf = (float) hrw / (float) Setup->width;
   float yf = (float) hrh / (float) Setup->height;
   
   x = X(xi) + SHIP_SZ/2;
   y = Y(yi) - SHIP_SZ*4/3;
   x2 = (ext_view_width / 2) - (sz/2);
   y2 = (ext_view_height / 2) -(sz/2);
   x3 = (x-x2)*xf + x2;
   y3 = (y-y2)*yf + y2;

   if (hrColor1 >= 1)
	   Arc_add(hrColor1, (int)x, (int)y, sz, sz, 0,
			   64 * 360);
   if (hrColor1 >= 1)
	   Arc_add(hrColor1, (int)x2,(int)y2, sz, sz, 0,
			   64 * 360);
   if (hrColor2 >= 1)
	   Arc_add(hrColor2, (int)x3, (int)y3, sz, sz, 0,
			   64 * 360);
		      		   }*//*doesn't work (yet) since client only knows visible bases */


static void Paint_HUD_items(int hud_pos_x, int hud_pos_y)
{
    const int		BORDER = 3;
    char		str[50];
    int			vert_pos, horiz_pos;
    int			i, maxWidth = -1,
			rect_x, rect_y, rect_width = 0, rect_height = 0;
    static int		vertSpacing = -1;

    SET_FG(colors[hudItemsColor].pixel);

    /* Special itemtypes */
    if (vertSpacing < 0)
	vertSpacing = MAX(ITEM_SIZE, gameFont->ascent + gameFont->descent) + 1;
    /* find the scaled location, then work in pixels */
    vert_pos = WINSCALE(hud_pos_y - hudSize+HUD_OFFSET + BORDER);
    horiz_pos = WINSCALE(hud_pos_x - hudSize+HUD_OFFSET - BORDER);
    rect_width = 0;
    rect_height = 0;
    rect_x = horiz_pos;
    rect_y = vert_pos;

    for (i = 0; i < NUM_ITEMS; i++) {
	int num = numItems[i];

	if (i == ITEM_FUEL)
	    continue;

	if (instruments.showItems) {
	    lastNumItems[i] = num;
	    if (num <= 0)
		num = -1;
	} else {
	    if (num != lastNumItems[i]) {
		numItemsTime[i] = (int)(showItemsTime * (double)FPS);
		lastNumItems[i] = num;
	    }
	    if (numItemsTime[i]-- <= 0) {
		numItemsTime[i] = 0;
		num = -1;
	    }
	}

	if (num >= 0) {
	    int len, width;

	    /* Paint item symbol */
	    Gui_paint_item_symbol((u_byte)i, drawPixmap, gameGC,
			horiz_pos - ITEM_SIZE,
			vert_pos,
			ITEM_HUD);

	    if (i == lose_item) {
		if (lose_item_active != 0) {
		    if (lose_item_active < 0)
			lose_item_active++;
		    rd.drawRectangle(dpy, drawPixmap, gameGC,
				horiz_pos-ITEM_SIZE-2,
				vert_pos-2, ITEM_SIZE+2, ITEM_SIZE+2);
		}
	    }

	    /* Paint item count */
	    sprintf(str, "%d", num);
	    len = strlen(str);
	    width = XTextWidth(gameFont, str, len);
	    rd.drawString(dpy, drawPixmap, gameGC,
			  horiz_pos - ITEM_SIZE - BORDER - width,
			  vert_pos + ITEM_SIZE/2 + gameFont->ascent/2,
			  str, len);

	    maxWidth = MAX(maxWidth, width + BORDER + ITEM_SIZE);
	    vert_pos += vertSpacing;

	    if (vert_pos+vertSpacing
		> WINSCALE(hud_pos_y+hudSize-HUD_OFFSET-BORDER)) {
		rect_width += maxWidth + 2*BORDER;
		rect_height = MAX(rect_height, vert_pos - rect_y);
		horiz_pos -= maxWidth + 2*BORDER;
		vert_pos = WINSCALE(hud_pos_y - hudSize+HUD_OFFSET + BORDER);
		maxWidth = -1;
	    }
	}
    }
    if (maxWidth != -1)
	rect_width += maxWidth + BORDER;

    if (rect_width > 0) {
	if (rect_height == 0)
	    rect_height = vert_pos - rect_y;
	rect_x -= rect_width;
    }

}

void Paint_HUD(void)
{
    const int		BORDER = 3;
    char		str[50];
    int			hud_pos_x, hud_pos_y, size;
    int			did_fuel = 0;
    int			i, j, modlen = 0;
    static char		autopilot[] = "Autopilot";

    /*
     * Show speed pointer
     */
    if (ptr_move_fact != 0.0
	&& selfVisible
	&& (selfVel.x != 0 || selfVel.y != 0))
	Segment_add(hudColor,
		    ext_view_width / 2,
		    ext_view_height / 2,
		    (int)(ext_view_width / 2 - ptr_move_fact * selfVel.x),
		    (int)(ext_view_height / 2 + ptr_move_fact * selfVel.y));

    if (selfVisible && dirPtrColor)
	Segment_add(dirPtrColor,
		    (int) (ext_view_width / 2 +
			   (100 - 15) * tcos(heading)),
		    (int) (ext_view_height / 2 -
			   (100 - 15) * tsin(heading)),
		    (int) (ext_view_width / 2 + 100 * tcos(heading)),
		    (int) (ext_view_height / 2 - 100 * tsin(heading)));

    if (hudRadarEnemyColor || hudRadarOtherColor) {
	double hudRadarMapScale = (double) Setup->width / (double) 256;
	Paint_hudradar(
	    hudRadarScale,
	    hudRadarLimit * (active_view_width / 2) * hudRadarScale
	    / hudRadarMapScale,
	    hudRadarLimit * (active_view_width / 2) * hudRadarScale
	    / hudRadarMapScale,
	    hudRadarDotSize);

	Paint_hudradar(hudRadarMapScale,
		       (double)active_view_width / 2,
		       (double)active_view_height / 2,
		       SHIP_SZ);
    }

    /* message scan hack by mara and jpv */
    if (Bms_test_state(BmsBall) && msgScanBallColor)
	Arc_add(msgScanBallColor, ext_view_width / 2 - 5,
		ext_view_height / 2 - 5, 10, 10, 0, 64 * 360);
    if (Bms_test_state(BmsCover) && msgScanCoverColor)
	Arc_add(msgScanCoverColor, ext_view_width / 2 - 4,
		ext_view_height / 2 - 4, 8, 8, 0, 64 * 360);

    /*
     * Display the HUD
     */
    hud_pos_x = (int)(ext_view_width / 2 - hud_move_fact * selfVel.x);
    hud_pos_y = (int)(ext_view_height / 2 + hud_move_fact * selfVel.y);

    /* HUD frame */
    gcv.line_style = LineOnOffDash;
    XChangeGC(dpy, gameGC, GCLineStyle | GCDashOffset, &gcv);

    if (hudHLineColor) {
	SET_FG(colors[hudHLineColor].pixel);
	rd.drawLine(dpy, drawPixmap, gameGC,
		    WINSCALE(hud_pos_x - hudSize),
		    WINSCALE(hud_pos_y - hudSize + HUD_OFFSET),
		    WINSCALE(hud_pos_x + hudSize),
		    WINSCALE(hud_pos_y - hudSize + HUD_OFFSET));
	rd.drawLine(dpy, drawPixmap, gameGC,
		    WINSCALE(hud_pos_x - hudSize),
		    WINSCALE(hud_pos_y + hudSize - HUD_OFFSET),
		    WINSCALE(hud_pos_x + hudSize),
		    WINSCALE(hud_pos_y + hudSize - HUD_OFFSET));
    }
    if (hudVLineColor) {
	SET_FG(colors[hudVLineColor].pixel);
	rd.drawLine(dpy, drawPixmap, gameGC,
		    WINSCALE(hud_pos_x - hudSize + HUD_OFFSET),
		    WINSCALE(hud_pos_y - hudSize),
		    WINSCALE(hud_pos_x - hudSize + HUD_OFFSET),
		    WINSCALE(hud_pos_y + hudSize));
	rd.drawLine(dpy, drawPixmap, gameGC,
		    WINSCALE(hud_pos_x + hudSize - HUD_OFFSET),
		    WINSCALE(hud_pos_y - hudSize),
		    WINSCALE(hud_pos_x + hudSize - HUD_OFFSET),
		    WINSCALE(hud_pos_y + hudSize));
    }
    gcv.line_style = LineSolid;
    XChangeGC(dpy, gameGC, GCLineStyle, &gcv);

    if (hudItemsColor)
	Paint_HUD_items(hud_pos_x, hud_pos_y);

    /* Fuel notify, HUD meter on */
    if (hudColor && (fuelTime > 0.0 || fuelSum < fuelNotify)) {
	SET_FG(colors[hudColor].pixel);
	did_fuel = 1;
	sprintf(str, "%04d", (int)fuelSum);
	rd.drawString(dpy, drawPixmap, gameGC,
		    WINSCALE(hud_pos_x + hudSize-HUD_OFFSET+BORDER),
		    WINSCALE(hud_pos_y + hudSize-HUD_OFFSET+BORDER)
				+ gameFont->ascent,
		    str, (int)strlen(str));
	if (numItems[ITEM_TANK]) {
	    if (fuelCurrent == 0)
		strcpy(str,"M ");
	    else
		sprintf(str, "T%d", fuelCurrent);
	    rd.drawString(dpy, drawPixmap, gameGC,
			  WINSCALE(hud_pos_x + hudSize-HUD_OFFSET + BORDER),
			  WINSCALE(hud_pos_y + hudSize-HUD_OFFSET + BORDER)
			  + gameFont->descent + 2*gameFont->ascent,
			  str, (int)strlen(str));
	}
    }

    /* Update the lock display */
    Paint_lock(hud_pos_x, hud_pos_y);

    /* Draw last score on hud if it is an message attached to it */
    if (hudColor) {
	SET_FG(colors[hudColor].pixel);

	for (i = 0, j = 0; i < MAX_SCORE_OBJECTS; i++) {
	    score_object_t*	sobj
		= &score_objects[(i+score_object)%MAX_SCORE_OBJECTS];
	    if (sobj->hud_msg_len > 0) {
		if (sobj->hud_msg_width == -1)
		    sobj->hud_msg_width = 
			XTextWidth(gameFont, 
				   sobj->hud_msg, 
				   sobj->hud_msg_len);
		if (j == 0 &&
		    sobj->hud_msg_width > WINSCALE(2*hudSize-HUD_OFFSET*2) &&
		    (did_fuel || hudVLineColor))
		    ++j;
		rd.drawString(dpy, drawPixmap, gameGC,
			      WINSCALE(hud_pos_x) - sobj->hud_msg_width/2,
			      WINSCALE(hud_pos_y + hudSize-HUD_OFFSET + BORDER)
			      + gameFont->ascent
			      + j * (gameFont->ascent + gameFont->descent),
			      sobj->hud_msg, sobj->hud_msg_len);
		j++;
	    }
	}

	if (time_left > 0) {
	    sprintf(str, "%3d:%02d",
		    (int)(time_left / 60), (int)(time_left % 60));
	    size = XTextWidth(gameFont, str, (int)strlen(str));
	    rd.drawString(dpy, drawPixmap, gameGC,
			  WINSCALE(hud_pos_x - hudSize+HUD_OFFSET - BORDER)
			  - size,
			  WINSCALE(hud_pos_y - hudSize+HUD_OFFSET - BORDER)
			  - gameFont->descent,
			  str, (int)strlen(str));
	}

	/* Update the modifiers */
	modlen = strlen(mods);
	rd.drawString(dpy, drawPixmap, gameGC,
		      WINSCALE(hud_pos_x - hudSize+HUD_OFFSET-BORDER)
		      - XTextWidth(gameFont, mods, modlen),
		      WINSCALE(hud_pos_y + hudSize-HUD_OFFSET+BORDER)
		      + gameFont->ascent,
		      mods, (int)strlen(mods));

	if (autopilotLight) {
	    int text_width = XTextWidth(gameFont, autopilot,
					sizeof(autopilot)-1);
	    rd.drawString(dpy, drawPixmap, gameGC,
			  WINSCALE(hud_pos_x) - text_width/2,
			  WINSCALE(hud_pos_y - hudSize+HUD_OFFSET - BORDER)
			  - gameFont->descent * 2 - gameFont->ascent,
			  autopilot, sizeof(autopilot)-1);
	}
    }

    if (fuelTime > 0.0) {
	fuelTime -= timePerFrame;
	if (fuelTime <= 0.0)
	    fuelTime = 0.0;
    }

    /* draw fuel gauge */
    if (fuelGaugeColor &&
	((fuelTime > 0.0)
	 || (fuelSum < fuelNotify
	     && ((fuelSum < fuelCritical && (loopsSlow % 4) < 2)
		 || (fuelSum < fuelWarning
		     && fuelSum > fuelCritical
		     && (loopsSlow % 8) < 4)
		 || (fuelSum > fuelWarning))))) {

	SET_FG(colors[fuelGaugeColor].pixel);
	rd.drawRectangle(dpy, drawPixmap, gameGC,
			 WINSCALE(hud_pos_x + hudSize - HUD_OFFSET
				  + FUEL_GAUGE_OFFSET) - 1,
			 WINSCALE(hud_pos_y - hudSize + HUD_OFFSET
				  + FUEL_GAUGE_OFFSET) - 1,
			 UWINSCALE(HUD_OFFSET - (2*FUEL_GAUGE_OFFSET)) + 3,
			 UWINSCALE(HUD_FUEL_GAUGE_SIZE) + 3);

	size = (HUD_FUEL_GAUGE_SIZE * fuelSum) / fuelMax;
	rd.fillRectangle(dpy, drawPixmap, gameGC,
			 WINSCALE(hud_pos_x + hudSize - HUD_OFFSET
				  + FUEL_GAUGE_OFFSET) + 1,
			 WINSCALE(hud_pos_y - hudSize + HUD_OFFSET
				  + FUEL_GAUGE_OFFSET + HUD_FUEL_GAUGE_SIZE
				  - size) + 1,
			 UWINSCALE(HUD_OFFSET - (2*FUEL_GAUGE_OFFSET)),
			 UWINSCALE(size));
    }
}


void Paint_messages(void)
{
    int		i, x, y, top_y, bot_y, width;
    size_t	len;
    const int	BORDER = 10,
		SPACING = messageFont->ascent+messageFont->descent+1;
    message_t	*msg;
    int		last_msg_index = 0, msg_color;

    top_y = BORDER + messageFont->ascent;
    bot_y = WINSCALE(ext_view_height) - messageFont->descent - BORDER;

    /* get number of player messages */
    while (last_msg_index < maxMessages
	   && TalkMsg[last_msg_index]->len != 0)
	last_msg_index++;
    last_msg_index--; /* make it an index */

    for (i = 0; i < 2 * maxMessages; i++) {
	if (i < maxMessages)
	    msg = TalkMsg[i];
	else
	    msg = GameMsg[i - maxMessages];
	if (msg->len == 0)
	    continue;

	/*
	 * While there is something emphasized, freeze the life time counter
	 * of a message if it is not drawn "flashed" (not in oldMessagesColor)
	 * anymore.
	 */
	if ((msg->lifeTime -= timePerFrame) <= 0.0) {
	    msg->txt[0] = '\0';
	    msg->len = 0;
	    msg->lifeTime = 0.0;
	    continue;
	}

	if (msg->lifeTime <= MSG_FLASH_TIME)
	    msg_color = oldMessagesColor;
	else {
	    /* If paused, don't bother to paint messages in mscScan* colors. */
	    if (self && strchr("P", self->mychar))
		msg_color = messagesColor;
	    else {
		switch (msg->bmsinfo) {
		case BmsBall:	msg_color = msgScanBallColor;	break;
		case BmsSafe:	msg_color = msgScanSafeColor;	break;
		case BmsCover:	msg_color = msgScanCoverColor;	break;
		case BmsPop:	msg_color = msgScanPopColor;	break;
		default:	msg_color = messagesColor;	break;
		}
	    }
	}

	/* kps ugly hack */
	if (newbie && msg->txt) {
	    const char *help = "[*Newbie help*]";
	    size_t sz = strlen(msg->txt);
	    size_t hsz = strlen(help);

	    if (sz > hsz
		&& !strcmp(help, &msg->txt[sz - hsz]))
		msg_color = WHITE;
	}

	if (msg_color == 0)
	    continue;

	if (i < maxMessages) {
	    x = BORDER;
	    y = top_y;
	    top_y += SPACING;
	} else {
	    if (!instruments.showMessages)
		continue;
	    x = BORDER;
	    y = bot_y;
	    bot_y -= SPACING;
	}
	len = charsPerSecond * (MSG_LIFE_TIME - msg->lifeTime);
	len = MIN(msg->len, len);

	{
	    XSetForeground(dpy, messageGC, colors[msg_color].pixel);
	    rd.drawString(dpy, drawPixmap, messageGC, x, y,
			  msg->txt, (int)len);
	}

	width = XTextWidth(messageFont, msg->txt, (int)MIN(len, msg->len));
    }
}


void Paint_recording(void)
{
    int			w, x, y, len;
    char		buf[32];
    double		mb;

    if (!recording || (loopsSlow % 16) < 8)
	return;

    SET_FG(colors[RED].pixel);
    mb = Record_size() / 1e6;
    sprintf(buf, "REC %.1f MB", mb);
    len = strlen(buf);
    w = XTextWidth(gameFont, buf, len);
    x = WINSCALE(ext_view_width) - 10 - w;
    y = 10 + gameFont->ascent;
    XDrawString(dpy, drawPixmap, gameGC, x, y, buf, len);
}


void Paint_HUD_values(void)
{
    int w, x, y, len;
    char buf[32];

    if (!hudColor)
	return;

    SET_FG(colors[hudColor].pixel);
    sprintf(buf, "FPS: %.3f", clientFPS);
    len = strlen(buf);
    w = XTextWidth(gameFont, buf, len);
    x = WINSCALE(ext_view_width) - 10 - w;
    y = 200 + gameFont->ascent;
    rd.drawString(dpy, drawPixmap, gameGC, x, y, buf, len);
}

xp_option_t hud_options[] = {

    COLOR_INDEX_OPTION(
	"hudColor",
	2,
	&hudColor,
	"Which color number to use for drawing the HUD.\n"),

    COLOR_INDEX_OPTION(
	"hudHLineColor",
	0,
	&hudHLineColor,
	"Which color number to use for drawing the horizontal lines\n"),

    COLOR_INDEX_OPTION(
	"hudVLineColor",
	0,
	&hudVLineColor,
	"Which color number to use for drawing the vertical lines\n"
	"in the HUD.\n"),

    COLOR_INDEX_OPTION(
	"hudItemsColor",
	2,
	&hudItemsColor,
	"Which color number to use for drawing owned items on the HUD.\n"),

    COLOR_INDEX_OPTION(
	"hudRadarEnemyColor",
	11,
	&hudRadarEnemyColor,
	"Which color number to use for drawing hudradar dots\n"
	"that represent enemy ships.\n"),

    COLOR_INDEX_OPTION(
	"hudRadarOtherColor",
	4,
	&hudRadarOtherColor,
	"Which color number to use for drawing hudradar dots\n"
	"that represent friendly ships or other objects.\n"),

    COLOR_INDEX_OPTION(
	"hudLockColor",
	0,
	&hudLockColor,
	"Which color number to use for drawing the lock on the HUD.\n"),

    COLOR_INDEX_OPTION(
	"fuelGaugeColor",
	0,
	&fuelGaugeColor,
	"Which color number to use for drawing the fuel gauge.\n"),

    COLOR_INDEX_OPTION(
	"dirPtrColor",
	0,
	&dirPtrColor,
	"Which color number to use for drawing the direction pointer hack.\n"),

    COLOR_INDEX_OPTION(
	"messagesColor",
	12,
	&messagesColor,
	"Which color number to use for drawing messages.\n"),

    COLOR_INDEX_OPTION(
	"oldMessagesColor",
	13,
	&oldMessagesColor,
	"Which color number to use for drawing old messages.\n"),

    COLOR_INDEX_OPTION(
	"msgScanBallColor",
	3,
	&msgScanBallColor,
	"Which color number to use for drawing ball message warning.\n"),

    COLOR_INDEX_OPTION(
	"msgScanSafeColor",
	4,
	&msgScanSafeColor,
	"Which color number to use for drawing safe message.\n"),

    COLOR_INDEX_OPTION(
	"msgScanCoverColor",
	2,
	&msgScanCoverColor,
	"Which color number to use for drawing cover message.\n"),

    COLOR_INDEX_OPTION(
	"msgScanPopColor",
	11,
	&msgScanPopColor,
	"Which color number to use for drawing pop message.\n"),


    COLOR_INDEX_OPTION(
	"fuelMeterColor",
	0,
	&fuelMeterColor,
	"Which color number to use for drawing the fuel meter.\n"),

    COLOR_INDEX_OPTION(
	"powerMeterColor",
	0,
	&powerMeterColor,
	"Which color number to use for drawing the power meter.\n"),

    COLOR_INDEX_OPTION(
	"turnSpeedMeterColor",
	0,
	&turnSpeedMeterColor,
	"Which color number to use for drawing the turn speed meter.\n"),

    COLOR_INDEX_OPTION(
	"packetSizeMeterColor",
	0,
	&packetSizeMeterColor,
	"Which color number to use for drawing the packet size meter.\n"
	"Each bar is equavalent to 1024 bytes, for a maximum of 4096 bytes.\n"),

    COLOR_INDEX_OPTION(
	"packetLossMeterColor",
	3,
	&packetLossMeterColor,
	"Which color number to use for drawing the packet loss meter.\n"
	"This gives the percentage of lost frames due to network failure.\n"),

    COLOR_INDEX_OPTION(
	"packetDropMeterColor",
	0,
	&packetDropMeterColor,
	"Which color number to use for drawing the packet drop meter.\n"
	"This gives the percentage of dropped frames due to display\n"
	"slowness.\n"),

    COLOR_INDEX_OPTION(
	"packetLagMeterColor",
	3,
	&packetLagMeterColor,
	"Which color number to use for drawing the packet lag meter.\n"
	"This gives the amount of lag in frames over the past one second.\n"),

    COLOR_INDEX_OPTION(
	"temporaryMeterColor",
	3,
	&temporaryMeterColor,
	"Which color number to use for drawing temporary meters.\n"),

    COLOR_INDEX_OPTION(
	"meterBorderColor",
	2,
	&meterBorderColor,
	"Which color number to use for drawing borders of meters.\n"),

    COLOR_INDEX_OPTION(
	"scoreObjectColor",
	4,
	&scoreObjectColor,
	"Which color number to use for drawing score objects.\n"),

};


void Store_hud_options(void)
{
    STORE_OPTIONS(hud_options);
}

