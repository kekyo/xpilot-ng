/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2000-2004 by
 *
 *      Uoti Urpala          <uau@users.sourceforge.net>
 *      Kristian Söderblom   <kps@users.sourceforge.net>
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

#include "xpserver.h"

/*
 * Globals.
 */
world_t World, *world;
bool is_polygon_map = false;

static void Find_base_direction(void);


static void Check_map_object_counters(void)
{
    int i;

    /*assert(world->NumCannons == 0);*/

    /*assert(world->NumGravs == 0);*/
    /*assert(world->NumWormholes == 0);*/
    /*assert(world->NumTreasures == 0);*/
    /*assert(world->NumTargets == 0);*/
    /*assert(world->NumBases == 0);*/
    /*assert(world->NumItemConcs == 0);
      assert(world->NumAsteroidConcs == 0);
      assert(world->NumFrictionAreas == 0);*/
    /*assert(world->NumEcms == 0);*/
    /*assert(world->NumTransporters == 0);*/

    for (i = 0; i < MAX_TEAMS; i++) {
	assert(world->teams[i].NumMembers == 0);
	assert(world->teams[i].NumRobots == 0);
	assert(world->teams[i].NumBases == 0);
	assert(world->teams[i].NumTreasures == 0);
	assert(world->teams[i].NumEmptyTreasures == 0);
	assert(world->teams[i].TreasuresDestroyed == 0);
	assert(world->teams[i].TreasuresLeft == 0);
	assert(world->teams[i].SwapperId == NO_ID);
    }
}

static void shrink(void **pp, size_t size)
{
    void *p;

    p = realloc(*pp, size);
    if (!p) {
	warn("Realloc failed!");
	exit(1);
    }
    *pp = p;
}

#define SHRINK(T,P,N,M) { \
if ((M) > (N)) { \
  shrink((void **)&(P), (N) * sizeof(T)); \
  M = (N); \
} } \


static void Realloc_map_objects(void)
{
    /*SHRINK(cannon_t, world->cannons, world->NumCannons, world->MaxCannons);*/
    /*SHRINK(fuel_t, world->fuels, world->NumFuels, world->MaxFuels);*/
    /*SHRINK(grav_t, world->gravs, world->NumGravs, world->MaxGravs);*/
    /*SHRINK(wormhole_t, world->wormholes,
      world->NumWormholes, world->MaxWormholes);*/
    /*SHRINK(treasure_t, world->treasures,
      world->NumTreasures, world->MaxTreasures);*/
    /*SHRINK(target_t, world->targets, world->NumTargets, world->MaxTargets);*/
    /*SHRINK(base_t, world->bases, world->NumBases, world->MaxBases);*/
    /*SHRINK(item_concentrator_t, world->itemConcs,
      world->NumItemConcs, world->MaxItemConcs);
      SHRINK(asteroid_concentrator_t, world->asteroidConcs,
      world->NumAsteroidConcs, world->MaxAsteroidConcs);
      SHRINK(friction_area_t, world->frictionAreas,
      world->NumFrictionAreas, world->MaxFrictionAreas);*/
}

int World_place_cannon(clpos_t pos, int dir, int team)
{
    cannon_t t, *cannon;
    int ind = Num_cannons(), i;

    t.pos = pos;
    t.dir = dir;
    t.team = team;
    t.dead_ticks = 0;
    t.conn_mask = ~0;
    t.group = NO_GROUP;
    t.score = CANNON_SCORE;
    t.id = ind + MIN_CANNON_ID;
    assert(Is_cannon_id(t.id));
    if (t.id > MAX_CANNON_ID) {
	warn("The server supports only %d cannons per map.", NUM_CANNON_IDS);
	exit(1);
    }
    for (i = 0; i < NUM_ITEMS; i++)
	t.initial_items[i] = -1;
    t.shot_speed = -1;
    t.smartness = -1;
    Arraylist_add(world->cannons, &t);
    cannon = Cannon_by_index(ind);
    assert(Cannon_by_id(t.id) == cannon);

    return ind;
}

int World_place_fuel(clpos_t pos, int team)
{
    fuel_t t;
    int ind = Num_fuels();

    t.pos = pos;
    t.fuel = START_STATION_FUEL;
    t.conn_mask = ~0;
    t.last_change = frame_loops;
    t.team = team;
    Arraylist_add(world->fuels, &t);

    return ind;
}

int World_place_base(clpos_t pos, int dir, int team, int order)
{
    base_t t;
    int ind = Num_bases(), i;

    t.pos = pos;
    t.order = order;
    /*
     * The direction of the base should be so that it points
     * up with respect to the gravity in the region.  This
     * is fixed in Find_base_direction() when the gravity has
     * been computed.
     */
    if (dir < 0 || dir >= RES) {
	warn("Base with direction %d in map.", dir);
	warn("Valid base directions are from 0 to %d.", RES-1);
	while (dir < 0)
	    dir += RES;
	while (dir >= RES)
	    dir -= RES;
	warn("Using direction %d for this base.", dir);
    }

    t.dir = dir;
    if (BIT(world->rules->mode, TEAM_PLAY)) {
	if (team < 0 || team >= MAX_TEAMS)
	    team = 0;
	t.team = team;
	world->teams[team].NumBases++;
	if (world->teams[team].NumBases == 1)
	    world->NumTeamBases++;
    } else
	t.team = TEAM_NOT_SET;
    t.ind = Num_bases();

    for (i = 0; i < NUM_ITEMS; i++)
	t.initial_items[i] = -1;
    Arraylist_add(world->bases, &t);

    return ind;
}

int World_place_treasure(clpos_t pos, int team, bool empty,
			 int ball_style)
{
    treasure_t t;
    int ind = Num_treasures();

    t.pos = pos;
    t.have = false;
    t.destroyed = 0;
    t.team = team;
    t.empty = empty;
    t.ball_style = ball_style;
    if (team != TEAM_NOT_SET) {
	world->teams[team].NumTreasures++;
	world->teams[team].TreasuresLeft++;
    }
    Arraylist_add(world->treasures, &t);

    return ind;
}

int World_place_target(clpos_t pos, int team)
{
    target_t t;
    int ind = Num_targets();

    t.pos = pos;
    /*
     * If we have a block based map, the team is determined in
     * in Xpmap_find_map_object_teams().
     */
    t.team = team;
    t.dead_ticks = 0;
    t.damage = TARGET_DAMAGE;
    t.conn_mask = ~0;
    t.update_mask = 0;
    t.last_change = frame_loops;
    t.group = NO_GROUP;
    Arraylist_add(world->targets, &t);

    return ind;
}

int World_place_wormhole(clpos_t pos, wormtype_t type)
{
    wormhole_t t;
    int ind = Num_wormholes();

    t.pos = pos;
    t.countdown = 0;
    t.lastdest = NO_IND;
    t.type = type;
    t.lastblock = SPACE;
    t.lastID = NO_ID;
    t.group = NO_GROUP;
    Arraylist_add(world->wormholes, &t);

    return ind;
}


/*
 * Allocate checkpoints for an xp map.
 */
static void alloc_old_checks(void)
{
    int i;
    check_t t;
    clpos_t pos = { -1, -1 };

    t.pos = pos;

    for (i = 0; i < OLD_MAX_CHECKS; i++)
	STORE(check_t, world->checks, world->NumChecks, world->MaxChecks, t);

    SHRINK(check_t, world->checks, world->NumChecks, world->MaxChecks);
    world->NumChecks = 0;
}

int World_place_check(clpos_t pos, int ind)
{
    check_t t;

    if (!BIT(world->rules->mode, TIMING)) {
	warn("Checkpoint on map with no timing.");
	return NO_IND;
    }

    /* kps - need to do this for other map object types ? */
    if (!World_contains_clpos(pos)) {
	warn("Checkpoint outside world, ignoring.");
	return NO_IND;
    }

    /*
     * On xp maps we can have only 26 checkpoints.
     */
    if (ind >= 0 && ind < OLD_MAX_CHECKS) {
	check_t *check;

	if (world->NumChecks == 0)
	    alloc_old_checks();

	/*
	 * kps hack - we can't use Check_by_index because it might return
	 * NULL since ind can here be >= world->NumChecks.
	 */
	check = &world->checks[ind];
	if (World_contains_clpos(check->pos)) {
	    warn("Map contains too many '%c' checkpoints.", 'A' + ind);
	    return NO_IND;
	}

	check->pos = pos;
	world->NumChecks++;
	return ind;
    }

    ind = world->NumChecks;
    t.pos = pos;
    STORE(check_t, world->checks, world->NumChecks, world->MaxChecks, t);
    return ind;
}

int World_place_item_concentrator(clpos_t pos)
{
    item_concentrator_t t;
    int ind = Num_itemConcs();

    t.pos = pos;
    Arraylist_add(world->itemConcs, &t);

    return ind;
}

int World_place_asteroid_concentrator(clpos_t pos)
{
    asteroid_concentrator_t t;
    int ind = Num_asteroidConcs();

    t.pos = pos;
    Arraylist_add(world->asteroidConcs, &t);

    return ind;
}

int World_place_grav(clpos_t pos, double force, int type)
{
    grav_t t;
    int ind = Num_gravs();

    t.pos = pos;
    t.force = force;
    t.type = type;
    Arraylist_add(world->gravs, &t);

    return ind;
}

int World_place_friction_area(clpos_t pos, double fric)
{
    friction_area_t t;
    int ind = Num_frictionAreas();

    t.pos = pos;
    t.friction_setting = fric;
    /*t.friction = ... ; handled in timing setup */
    Arraylist_add(world->frictionAreas, &t);

    return ind;
}

shape_t filled_wire;
clpos_t filled_coords[4];

static void Filled_wire_init(void)
{
    int i, h;

    filled_wire.num_points = 4;

    for (i = 0; i < 4; i++)
	filled_wire.pts[i] = &filled_coords[i];

    h = BLOCK_CLICKS / 2;

    /* whole (filled) block */
    filled_coords[0].cx = -h;
    filled_coords[0].cy = -h;
    filled_coords[1].cx = h - 1;
    filled_coords[1].cy = -h;
    filled_coords[2].cx = h - 1;
    filled_coords[2].cy = h - 1;
    filled_coords[3].cx = -h;
    filled_coords[3].cy = h - 1;
}

int World_init(void)
{
    int i;

    memset(world, 0, sizeof(world_t));

    if ((world->asteroidConcs
	 = Arraylist_alloc(sizeof(asteroid_concentrator_t))) == NULL)
	return -1;
    if ((world->bases = Arraylist_alloc(sizeof(base_t))) == NULL)
	return -1;
    if ((world->cannons = Arraylist_alloc(sizeof(cannon_t))) == NULL)
	return -1;
    if ((world->ecms = Arraylist_alloc(sizeof(ecm_t))) == NULL)
	return -1;
    if ((world->frictionAreas
	 = Arraylist_alloc(sizeof(friction_area_t))) == NULL)
	return -1;
    if ((world->fuels = Arraylist_alloc(sizeof(fuel_t))) == NULL)
	return -1;
    if ((world->itemConcs
	 = Arraylist_alloc(sizeof(item_concentrator_t))) == NULL)
	return -1;
    if ((world->gravs = Arraylist_alloc(sizeof(grav_t))) == NULL)
	return -1;
    if ((world->targets = Arraylist_alloc(sizeof(target_t))) == NULL)
	return -1;
    if ((world->treasures = Arraylist_alloc(sizeof(treasure_t))) == NULL)
	return -1;
    if ((world->transporters
	 = Arraylist_alloc(sizeof(transporter_t))) == NULL)
	return -1;
    if ((world->wormholes = Arraylist_alloc(sizeof(wormhole_t))) == NULL)
	return -1;

    for (i = 0; i < MAX_TEAMS; i++)
	Team_by_index(i)->SwapperId = NO_ID;

    Filled_wire_init();

    return 0;
}

void World_free(void)
{
    XFREE(world->block);
    XFREE(world->gravity);
    /*XFREE(world->gravs);*/
    /*XFREE(world->bases);*/
    /*XFREE(world->cannons);*/
    XFREE(world->checks);
    /*XFREE(world->fuels);*/
    /*XFREE(world->wormholes);*/
    /*XFREE(world->itemConcs);
    XFREE(world->asteroidConcs);
    XFREE(world->frictionAreas);*/
}

static bool World_alloc(void)
{
    int x;
    unsigned char *map_line;
    unsigned char **map_pointer;
    vector_t *grav_line;
    vector_t **grav_pointer;

    assert(world->block == NULL);
    assert(world->gravity == NULL);

    world->block = (unsigned char **)
	malloc(sizeof(unsigned char *) * world->x
	       + world->x * sizeof(unsigned char) * world->y);
    world->gravity = (vector_t **)
	malloc(sizeof(vector_t *) * world->x
	       + world->x * sizeof(vector_t) * world->y);

    /*assert(world->gravs == NULL);*/
    /*assert(world->bases == NULL);*/
    /*assert(world->fuels == NULL);*/
    /*assert(world->cannons == NULL);*/
    assert(world->checks == NULL);
    /*assert(world->wormholes == NULL);*/
    /*assert(world->itemConcs == NULL);*/
    /*assert(world->asteroidConcs == NULL);*/

    if (world->block == NULL || world->gravity == NULL) {
	World_free();
	error("Couldn't allocate memory for map");
	return false;
    }

    map_pointer = world->block;
    map_line = (unsigned char*) ((unsigned char**)map_pointer + world->x);

    grav_pointer = world->gravity;
    grav_line = (vector_t*) ((vector_t**)grav_pointer + world->x);

    for (x = 0; x < world->x; x++) {
	*map_pointer = map_line;
	map_pointer += 1;
	map_line += world->y;
	*grav_pointer = grav_line;
	grav_pointer += 1;
	grav_line += world->y;
    }

    return true;
}

/*
 * This function can be called after the map options have been read.
 */
static bool Grok_map_size(void)
{
    bool bad = false;
    int w = options.mapWidth, h = options.mapHeight;

    if (!is_polygon_map) {
	w *= BLOCK_SZ;
	h *= BLOCK_SZ;
    }

    if (w < MIN_MAP_SIZE) {
	warn("mapWidth too small, minimum is %d pixels (%d blocks).\n",
	     MIN_MAP_SIZE, MIN_MAP_SIZE / BLOCK_SZ + 1);
	bad = true;
    }
    if (w > MAX_MAP_SIZE) {
	warn("mapWidth too big, maximum is %d pixels (%d blocks).\n",
	     MAX_MAP_SIZE, MAX_MAP_SIZE / BLOCK_SZ);
	bad = true;
    }
    if (h < MIN_MAP_SIZE) {
	warn("mapHeight too small, minimum is %d pixels (%d blocks).\n",
	     MIN_MAP_SIZE, MIN_MAP_SIZE / BLOCK_SZ + 1);
	bad = true;
    }
    if (h > MAX_MAP_SIZE) {
	warn("mapWidth too big, maximum is %d pixels (%d blocks).\n",
	     MAX_MAP_SIZE, MAX_MAP_SIZE / BLOCK_SZ);
	bad = true;
    }

    if (bad)
	return false;

    /* pixel sizes */
    world->width = w;
    world->height = h;
    if (!is_polygon_map && options.extraBorder) {
	world->width += 2 * BLOCK_SZ;
	world->height += 2 * BLOCK_SZ;
    }
    world->hypotenuse = LENGTH(world->width, world->height);

    /* click sizes */
    world->cwidth = world->width * CLICK;
    world->cheight = world->height * CLICK;

    /* block sizes */
    world->x = (world->width - 1) / BLOCK_SZ + 1; /* !@# */
    world->y = (world->height - 1) / BLOCK_SZ + 1;
    world->diagonal = LENGTH(world->x, world->y);
    world->bwidth_floor = world->width / BLOCK_SZ;
    world->bheight_floor = world->height / BLOCK_SZ;

    return true;
}

bool Grok_map_options(void)
{
    if (world->have_options)
	return true;

    Check_map_object_counters();

    if (!Grok_map_size())
	return false;

    strlcpy(world->name, options.mapName, sizeof(world->name));
    strlcpy(world->author, options.mapAuthor, sizeof(world->author));
    strlcpy(world->dataURL, options.dataURL, sizeof(world->dataURL));

    if (!World_alloc())
	return false;

    Set_world_rules();
    Set_world_items();
    Set_world_asteroids();

    if (BIT(world->rules->mode, TEAM_PLAY|TIMING) == (TEAM_PLAY|TIMING)) {
	warn("Cannot teamplay while in race mode -- ignoring teamplay");
	CLR_BIT(world->rules->mode, TEAM_PLAY);
    }

    world->have_options = true;

    return true;
}

bool Grok_map(void)
{
    if (!Grok_map_options())
	return false;

    if (!is_polygon_map) {
	Xpmap_grok_map_data();
	Xpmap_tags_to_internal_data();
	Xpmap_find_map_object_teams();
    }

    if (!Verify_wormhole_consistency())
	return false;

    if (BIT(world->rules->mode, TIMING) && world->NumChecks == 0) {
	warn("No checkpoints found while race mode (timing) was set.");
	warn("Turning off race mode.");
	CLR_BIT(world->rules->mode, TIMING);
    }

    /* kps - what are these doing here ? */
    if (options.maxRobots == -1)
	options.maxRobots = Num_bases();

    if (options.minRobots == -1)
	options.minRobots = options.maxRobots;

    Realloc_map_objects();

    if (Num_bases() <= 0)
	fatal("Map has no bases!");

    xpprintf("World....: %s\nBases....: %d\nMapsize..: %dx%d pixels\n"
	     "Team play: %s\n",
	     world->name, Num_bases(), world->width, world->height,
	     BIT(world->rules->mode, TEAM_PLAY) ? "on" : "off");

    if (!is_polygon_map)
	Xpmap_blocks_to_polygons();

    Compute_gravity();
    Find_base_direction();

    return true;
}

/*
 * Return the team that is closest to this click position.
 */
int Find_closest_team(clpos_t pos)
{
    int team = TEAM_NOT_SET, i;
    double closest = FLT_MAX, l;

    for (i = 0; i < Num_bases(); i++) {
	base_t *base = Base_by_index(i);

	if (base->team == TEAM_NOT_SET)
	    continue;

	l = Wrap_length(pos.cx - base->pos.cx, pos.cy - base->pos.cy);
	if (l < closest) {
	    team = base->team;
	    closest = l;
	}
    }

    return team;
}


static void Find_base_direction(void)
{
    /* kps - this might go wrong if we run in -options.polygonMode ? */
    if (!is_polygon_map)
	Xpmap_find_base_direction();
}

double Wrap_findDir(double dx, double dy)
{
    dx = WRAP_DX(dx);
    dy = WRAP_DY(dy);
    return findDir(dx, dy);
}

double Wrap_cfindDir(int dcx, int dcy)
{
    dcx = WRAP_DCX(dcx);
    dcy = WRAP_DCY(dcy);
    return findDir((double)dcx, (double)dcy);
}

double Wrap_length(int dcx, int dcy)
{
    dcx = WRAP_DCX(dcx);
    dcy = WRAP_DCY(dcy);
    return LENGTH(dcx, dcy);
}
