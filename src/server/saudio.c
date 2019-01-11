/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
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

/* This piece of code was provided by Greg Renda (greg@ncd.com). */

#include "xpserver.h"

#define SOUND_RANGE_FACTOR	0.5	/* factor to increase sound
					 * range by */
#define SOUND_DEFAULT_RANGE	(BLOCK_CLICKS*15)
#define SOUND_MAX_VOLUME	100
#define SOUND_MIN_VOLUME	10

static inline double sound_range(player_t *pl)
{
    return SOUND_DEFAULT_RANGE
	+ pl->item[ITEM_SENSOR] * SOUND_DEFAULT_RANGE * SOUND_RANGE_FACTOR;
}

typedef struct AudioQRec {
    int index, volume;
    struct AudioQRec *next;
} AudioQRec, *AudioQPtr;


static void queue_audio(player_t * pl, int sound_ind, int volume)
{
    AudioQPtr a, p, prev;

    p = prev = (AudioQPtr) pl->audio;

    while (p) {
	if (p->index == sound_ind) {	/* same sound already in queue */
	    if (p->volume < volume)	/* weaker version: replace volume */
		p->volume = volume;
	    return;
	}
	prev = p;
	p = p->next;
    }

    /* not found in queue: add to end */
    if (!(a = (AudioQPtr) malloc(sizeof(AudioQRec))))
	return;

    a->index = sound_ind;
    a->volume = volume;
    a->next = NULL;

    if (prev)
	prev->next = a;
    else
	pl->audio = (void *) a;
}

int sound_player_init(player_t * pl)
{
    SDBG(printf("sound_player_init %p\n", pl));

    pl->audio = NULL;

    return 0;
}

/*
 * Set (or reset) a player status flag indicating
 * that a player wants (or doesn't want) sound.
 */
void sound_player_on(player_t * pl, int on)
{
    SDBG(printf("sound_player_on %p, %d\n", pl, on));

    if (on) {
	if (!pl->want_audio) {
	    pl->want_audio = true;
	    sound_play_player(pl, START_SOUND);
	}
    } else
	pl->want_audio = false;
}

/*
 * Play a sound for a player.
 */
void sound_play_player(player_t * pl, int sound_ind)
{
    if (!options.sound)
	return;

    SDBG(printf("sound_play_player %p, %d\n", pl, sound_ind));

    if (pl->want_audio)
	queue_audio(pl, sound_ind, 100);
}

/*
 * Play a sound for all players.
 */
void sound_play_all(int sound_ind)
{
    int i;

    if (!options.sound)
	return;

    SDBG(printf("sound_play_all %d\n", sound_ind));

    for (i = 0; i < NumPlayers; i++) {
	player_t *pl_i = Player_by_index(i);

	if (pl_i->want_audio)
	    sound_play_player(pl_i, sound_ind);
    }
}

/*
 * Play a sound if location is within player's sound range. A player's sound
 * range depends on the number of sensors they have. The default sound range
 * is what the player can see on the screen. A volume is assigned to the
 * sound depending on the location within the sound range.
 */
void sound_play_sensors(clpos_t pos, int sound_ind)
{
    int i, volume;
    double dx, dy, range, factor;
    player_t *pl;

    if (!options.sound)
	return;

    SDBG(printf("sound_play_sensors %d, %d, %d\n", cx, cy, sound_ind));

    for (i = 0; i < NumPlayers; i++) {
	pl = Player_by_index(i);

	if (!pl->want_audio)
	    continue;

	dx = ABS(pl->pos.cx - pos.cx);
	dy = ABS(pl->pos.cy - pos.cy);
	range = sound_range(pl);

	if (dx >= 0 && dx <= range && dy >= 0 && dy <= range) {
	    /*
	     * scale the volume
	     */
	    factor = MAX(dx, dy) / range;
	    volume =
		(int) MAX(SOUND_MAX_VOLUME - SOUND_MAX_VOLUME * factor,
			  SOUND_MIN_VOLUME);
	    queue_audio(pl, sound_ind, volume);
	}
    }
}

void sound_play_queued(player_t * pl)
{
    AudioQPtr p, n;

    if (!options.sound)
	return;

    SDBG(printf("sound_play_sensors %p\n", pl));

    p = (AudioQPtr) pl->audio;
    pl->audio = NULL;

    while (p) {
	n = p->next;
	Send_audio(pl->conn, p->index, p->volume);
	free(p);
	p = n;
    }
}

void sound_close(player_t * pl)
{
    AudioQPtr p, n;

    SDBG(printf("sound_close %p\n", pl));

    p = (AudioQPtr) pl->audio;
    pl->audio = NULL;

    while (p) {
	n = p->next;
	free(p);
	p = n;
    }
}
