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

#ifndef OBJPOS_H
#define OBJPOS_H

void Object_position_set_clpos(object_t *obj, clpos_t pos);
void Object_position_init_clpos(object_t *obj, clpos_t pos);
void Object_position_restore(object_t *obj);
void Object_position_limit(object_t *obj);
void Player_position_debug(player_t *pl, const char *msg);

static inline void Object_position_remember(object_t *obj)
{
    obj->prevpos = obj->pos;
}

static inline void Object_position_set_clvec(object_t *obj, clvec_t vec)
{
    clpos_t pos;

    pos.cx = vec.cx;
    pos.cy = vec.cy;

    Object_position_set_clpos(obj, pos);
}

#endif
