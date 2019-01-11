#!/usr/bin/env python
from __future__ import division, generators

KEEP_MAPDATA = 0
WALL_SHRINK_HACK = 0

# Requires python 2.2 or newer

# Some parts could be written more clearly with higher-level constructs,
# but have been optimized for speed (converting big maps can still take
# several seconds).

removedopts = """
analyticalcollisiondetection edgebounce extraborder
maxshieldedplayerwallbounceangle maxunshieldedplayerwallbounceangle
oneplayeronly scoretablefilename teamassign
""".split()

# 'mapdata' has been removed too, but isn't ignored by this script.
# 'numberofrounds', 'numrounds' and 'roundstoplay' have been used for the same
# thing

illegalopts = "noquit plockserver timerresolution".split()

knownopts = """
allowclusters allowheatseekers allowlasermodifiers allowmodifiers
allownukes allowplayerbounces allowplayercrashes allowplayerkilling
allowshields allowshipshapes allowsmartmissiles allowtorpedoes
allowviewing ballkillscoremult ballradius ballswallbounce baseminerange
cannonitemprobmult cannonsmartness cannonsuseitems checkpointradius
cloakedexhaust clusterkillscoremult contactport
crashscoremult debriswallbounce defaultsfilename denyhosts
destroyitemincollisionprob detonateitemonkillprob distinguishmissiles
dropitemonkillprob dump ecmsreprogrammines edgewrap
explosionkillscoremult firerepeatrate framespersecond friction
gameduration gravity gravityangle gravityanticlockwise
gravityclockwise gravitypoint gravitypointsource gravityvisible
heatkillscoremult help identifymines idlerun ignore20maxfps
ignore20maxfps initialafterburners initialarmor initialautopilots
initialcloaks initialdeflectors initialecms initialemergencyshields
initialemergencythrusts initialfuel initialhyperjumps initiallasers
initialmines initialmirrors initialmissiles initialphasings
initialrearshots initialsensors initialtanks initialtractorbeams
initialtransporters initialwideangles itemafterburnerprob
itemarmorprob itemautopilotprob itemcloakprob itemconcentratorprob
itemconcentratorradius itemconcentratorvisible itemdeflectorprob
itemecmprob itememergencyshieldprob itememergencythrustprob
itemenergypackprob itemhyperjumpprob itemlaserprob itemmineprob
itemmirrorprob itemmissileprob itemphasingprob itemprobmult
itemrearshotprob itemsensorprob itemswallbounce itemtankprob
itemtractorbeamprob itemtransporterprob itemwideangleprob keepshots
laserisstungun laserkillscoremult limitedlives limitedvisibility
lockotherteam loseitemdestroys mapauthor mapfilename mapheight mapname
mapwidth maxafterburners maxarmor maxautopilots maxcloaks
maxdeflectors maxecms maxemergencyshields maxemergencythrusts maxfuel
maxhyperjumps maxitemdensity maxlasers maxmines maxminesperpack
maxmirrors maxmissiles maxmissilesperpack maxobjectwallbouncespeed
maxphasings maxplayershots maxrearshots maxroundtime maxsensors
maxshieldedwallbouncespeed maxtanks maxtractorbeams maxtransporters
maxunshieldedwallbouncespeed maxvisibilitydistance maxwideangles
minefusetime minelife minescoremult minesonradar mineswallbounce
minvisibilitydistance missilelife missilesonradar missileswallbounce
movingitemprob noquit nukeclusterdamage nukeminmines nukeminsmarts
nukesonradar numberofrounds numrounds roundstoplay
objectwallbouncebrakefactor objectwallbouncelifefactor
password playerlimit playersonradar
playerstartsshielded playerwallbouncebrakefactor plockserver racelaps
recordmode reporttometaserver reset resetonhuman rogueheatprob
roguemineprob rounddelay runoverkillscoremult searchdomainforxpilot
shieldeditempickup shieldedmining shipmass shotkillscoremult shotlife
shotmass shotsgravity shotspeed shotswallbounce shovekillscoremult
smartkillscoremult sparkswallbounce tankkillscoremult targetkillteam
targetsync targetteamcollision teamcannons teamfuel teamimmunity
teamplay timerresolution timing torpedokillscoremult
treasurecollisiondestroys treasurecollisionmaykill treasurekillteam
treasuresonradar version wallbouncedestroyitemprob
wallbouncefueldrainmult worldlives wormholevisible wormtime
wreckagecollisionmaykill mapdata baselesspausing
pausedframespersecond pausedfps waitingframespersecond waitingfps
robots maxrobots minrobots robotfile robotstalk robotsleave robotleavelife
robotleavescore robotleaveratio robotteam restrictrobots reserverobotteam
robotrealname robothostname ecmsreprogramrobots
maxdefensiveitems maxoffensiveitems usewreckage
""".split()

def checkopts(options):
    dany = 0
    unknown = []
    illegal = []
    for opt in options.keys():
	if opt in removedopts:
	    if not dany:
		dany = 1
		print >> sys.stderr, "Removing the following options:"
	    print >>sys.stderr, opt + "  ",
	    del options[opt]
	elif opt in illegalopts:
	    del options[opt]
	    illegal.append(opt)
	elif opt not in knownopts:
	    del options[opt]
	    unknown.append(opt)
    if dany:
	print >> sys.stderr
    if illegal:
	print >> sys.stderr, "The following options may no more be set "\
	      "in map file"
	print >> sys.stderr, illegal
    for opt in unknown:
	print >> sys.stderr, "WARNING: did not recognize option %s, "\
	      "removed!" % opt

def parse(lines):
    options = {}
    ln = 0
    while ln < len(lines):
	line = lines[ln].split("#")[0].strip() # comments & s/e whitespace
	ln += 1
	if line == "":
	    continue
	option, value = line.split(':', 1)
	option = option.strip().lower()
	value = value.strip()
	if value[:11] == "\multiline:":
	    delim = value[11:].strip() + "\n"
	    value = ""
	    while ln < len(lines) and lines[ln] != delim:
		value += lines[ln]
		ln += 1
	    ln += 1
	options[option] = value
    return options

FILLED = 'x#' # Consider fuel stations as a filled block
REC_UL = 's'
REC_UR = 'a'
REC_DL = 'w'
REC_DR = 'q'
WALL = FILLED + REC_UL + REC_UR + REC_DL + REC_DR
ATTRACT = '$'

BCLICKS = 35 * 64

dirs = ((1, 0), (1, -1), (0, -1), (-1, -1), (-1, 0), (-1, 1), (0, 1), (1, 1))

class Wrapcoords(object):
    def __init__(self, width, height, x = 0, y = 0):
	self.width = width
	self.height = height
	self.x = x % width
	self.y = y % height
    def __eq__(self, other):
	return self.x == other.x and self.y == other.y
    def copy(self):
	return Wrapcoords(self.width, self.height, self.x, self.y)
    def godir(self, dir):
	self.x += dirs[dir][0]
	self.y += dirs[dir][1]
	self.x %= self.width
	self.y %= self.height
    def dist2(self, other):
	x = (self.x - other.x + self.width // 2) % self.width - self.width // 2
	y = (self.y - other.y + self.height//2) % self.height - self.height //2
	return x * x + y * y
# Give class Wrapcoords methods called .u() etc that return a separate
# object one block above etc
dirnames = "r ur u ul l dl d dr".split()
for i in range(8):
    def movedir(self, dir = i):
	result = self.copy()
	result.godir(dir)
	return result
    setattr(Wrapcoords, dirnames[i], movedir)
del i

class Map(object):
    def __init__(self, data, width, height):
	self.data = data
	self.width = width
	self.height = height
    def __getitem__(self, coords):
	return self.data[coords.y][coords.x]
    def __setitem__(self, coords, value):
	self.data[coords.y][coords.x] = value
    def noedge(self, loc, d):
	# Return whether there's solid wall to the right of the
	# direction d. Used when following polygon edge.
	if d == 0 :
	    return self[loc] in FILLED + REC_UL + REC_UR
	elif d == 1:
	    return self[loc.u()] in FILLED + REC_DL + REC_DR
	elif d == 2:
	    return self[loc.u()] in FILLED + REC_UL + REC_DL
	elif d == 3:
	    return self[loc.ul()] in FILLED + REC_UR + REC_DR
	elif d == 4:
	    return self[loc.ul()] in FILLED + REC_DL + REC_DR
	elif d == 5:
	    return self[loc.l()] in FILLED + REC_UR + REC_UL
	elif d == 6:
	    return self[loc.l()] in FILLED + REC_DR + REC_UR
	elif d == 7:
	    return self[loc] in FILLED + REC_DL + REC_UL
    def coords(self):
	for y in range(self.height):
	    for x in range(self.width):
		yield Wrapcoords(self.width, self.height, x, y)
    def ncoords(self):
	return [(x, y) for y in range(self.height) for x in range(self.width)]
    def scratch(self):
	return Map([[None] * self.width for _ in range(self.height)],
		   self.width, self.height)

class Struct:
    pass

def dist2(x1, y1, x2, y2, width, height):
    x = (x1 - x2 + (width >> 1)) % width - (width >> 1)
    y = (y1 - y2 + (height >> 1)) % height - (height >> 1)
    return 1. * x * x + 1. * y * y

def closestteam(loc, bases):
    maxd = 30000 * 30000
    for bs in bases:
	if loc.dist2(bs.loc) < maxd:
	    maxd = loc.dist2(bs.loc)
	    ans = bs.team
    return ans

def poly_partition(map):
    ways = {}
    for i in FILLED:
	ways[i] = [0, 2, 4, 6]
    ways[REC_UR] = [0, 2]
    ways[REC_UL] = [2, 4]
    ways[REC_DL] = [4, 6]
    ways[REC_DR] = [6, 0]
    mapparts = map.scratch()
    partnum = 0
    width, height = map.width, map.height
    for x, y in map.ncoords():
	if map.data[y][x] in WALL and mapparts.data[y][x] is None:
	    left = [(x, y)]
	    mapparts.data[y][x] = (partnum, x, y)
	    while left:
		x2, y2 = left.pop()
		c = map.data[y2 % height][x2 % width]
		for d in ways[c]:
		    x3 = x2 + dirs[d][0]
		    y3 = y2 + dirs[d][1]
		    xw = x3 % width
		    yw = y3 % height
		    if map.data[yw][xw] not in WALL or \
			   ((d + 4) % 8) not in ways[map.data[yw][xw]]:
			continue
		    if mapparts.data[yw][xw] is None:
			left.append((x3, y3))
			mapparts.data[yw][xw] = (partnum, x3, y3)
			continue
	    partnum += 1
    return mapparts, partnum

def poly_findedges(map, mapparts, partcount):
    partpolys = [[] for _ in range(partcount)]
    done = map.scratch()
    # Follow the edge of a polygon and return the polygon.
    def tracepoly(x, y, startdir, okdir = None):
	dir = startdir
	l = Wrapcoords(map.width, map.height, x, y)
	startloc = l.copy()
	poly = []
	while 1:
	    poly.append((x, y, dirs[dir]))
	    x += dirs[dir][0]
	    y += dirs[dir][1]
	    if dir in [6, 7]:
		done[l] = 1
	    elif dir == 5:
		done[l.l()] = 1
	    elif dir == 3:
		done[l.ul()] = 1
	    elif dir == 1:
		done[l.u()] = 1
	    l.godir(dir)
	    dir = (dir + 3) % 8  # always turn as much left as possible
	    while map.noedge(l, dir):
		dir = (dir - 1) % 8  # that much wasn't possible
	    if l == startloc and dir == startdir:
		break
	    if okdir is not None:
		if dir not in okdir:
		    return None
		if dir == 2:
		    done[l.u()] = -1
	return poly

    width, height = map.width, map.height
    for x, y in map.ncoords():
	if done.data[y][x]:
	    continue
	block = map.data[y][x]
	if block not in WALL:
	    continue
	if block == REC_UR:
	    dir = 7
	    dx = dy = 0
	elif block == REC_DR:
	    dx = 1
	    dy = 0
	    dir = 5
	elif block == REC_DL:
	    dx = dy = 1
	    dir = 3
	elif block == REC_UL:
	    dx = 0
	    dy = 1
	    dir = 1
	elif block in FILLED + REC_DL + REC_UL and \
	       map.data[y][(x-1)%width] not in FILLED + REC_DR + REC_UR:
	    dx = dy = 0
	    dir = 6
	else:
	    continue
	sl = mapparts.data[y][x]
	partpolys[sl[0]].append(tracepoly(sl[1] + dx, sl[2] + dy, dir))

    # Handle special (wrapping) edges that aren't detected above
    for loc in map.coords():
	if loc.y > 0:
	    break
	if map[loc] not in FILLED + REC_DL + REC_UL and \
	   map[loc.l()] in FILLED and done[loc] != -1:
	    done[loc] = -1
	    sl = mapparts[loc.l()]
	    poly = tracepoly(sl[1] + 1, sl[2] + 1, 2, (0, 2, 4))
	    if poly:
		partpolys[sl[0]].append(poly)

    loc = Wrapcoords(map.width, map.height, 0, 0)
    while 1:
	if map[loc] in FILLED and map[loc.u()] not in FILLED + REC_DL + REC_DR:
	    sl = mapparts[loc]
	    poly = tracepoly(sl[1] + 1, sl[2], 4, (4,))
	    if poly:
		partpolys[sl[0]].append(poly)
	if map[loc] in FILLED and map[loc.d()] not in FILLED + REC_UR + REC_UL:
	    sl = mapparts[loc]
	    poly = tracepoly(sl[1], sl[2] + 1, 0, (0,))
	    if poly:
		partpolys[sl[0]].append(poly)
	loc.godir(6)
	if loc.y == 0:
	    break
    return partpolys

def polydir(poly):
    xd, yd = poly[-1][2]
    wind = 0
    for point in poly:
	if (yd >= 0) ^ (point[2][1] >= 0):
	    side = xd * point[2][1] - point[2][0] * yd
	    if side != 0:
		if side > 0:
		    wind -= 1
		else:
		    wind += 1
	xd, yd = point[2]
    return wind // 2

def findmin(part1, part2, mindist, distfunc2):
    from math import sqrt
    result = None
    mind = sqrt(mindist)
    far = (mind + 4) ** 2
    swap = 0
    if len(part1) > len(part2):
	part1, part2 = part2, part1
	swap = 1
    for k in range(len(part1)):
	x = part1[k][0]
	y = part1[k][1]
	l = 0
	while l < len(part2):
	    dx, dy = distfunc2(part2[l][0] - x, part2[l][1] - y)
	    d = dx **2 + dy ** 2
	    if d < mindist:
		mindist = d
		mind = sqrt(mindist)
		far = (mind + 4) ** 2
		result = (mindist, k, l, dx, dy)
		l += 1
	    elif d < far:
		l += 1
	    else:
		l += int((sqrt(d) - mind) / 1.4143)
    if not swap or not result:
	return result
    return (result[0], result[2], result[1], -result[3], -result[4])

class Polynode(object):
    def __init__(self, poly):
	self.poly = poly
	self.linkstarts = []
	self.linktargets = []
	self.dist = (1e98, 0, 0)
    def traverse(self, x, y, start, reslist):
	i = start
	while 1:
	    while i in self.linkstarts:
		j = self.linkstarts.index(i)
		lt = self.linktargets[j]
		reslist.append((x, y, (lt.dx, lt.dy), 1))
		lt.targetnode.traverse(x + lt.dx, y + lt.dy,
				       lt.targetindex, reslist)
		reslist.append((x + lt.dx, y + lt.dy, (-lt.dx, -lt.dy), 1))
		del self.linkstarts[j]
		del self.linktargets[j]
	    dx, dy = self.poly[i][2][0], self.poly[i][2][1]
	    reslist.append((x, y, (dx, dy), 0))
	    x += dx
	    y += dy
	    i = (i + 1) % len(self.poly)
	    if i == start:
		break

def poly_handle_totalwrap(poly, map):
    res = []
    prev = (0, 0)
    for r in poly:
	if max(prev[0], prev[1]) > 1:
	    for _ in range(max(prev[2][0], prev[2][1]) - 1):
		# Don't let findmin skip optimization break
		res.append((r[0], r[1], (0, 0), r[3]))
	res.append(r)
	prev = r
    def minwrap(dx, dy, wx = map.width, wy = map.height):
	dx2 = (dx + (wx >> 1)) % wx - (wx >> 1)
	dy2 = (dy + (wy >> 1)) % wy - (wy >> 1)
	if dx2 == dx and dy2 == dy:
	    if abs(dx2) > abs(dy2):
		if dx2 > 0:
		    dx2 -= wx
		else:
		    dx2 += wx
	    else:
		if dy2 > 0:
		    dy2 -= wy
		else:
		    dy2 += wy
	return dx2, dy2
    r = findmin(res, res, 1e99, minwrap)
    wx = res[r[2]][0] - res[r[1]][0] - r[3]
    wy = res[r[2]][1] - res[r[1]][1] - r[4]
    assert wx % map.width == 0
    assert wy % map.height == 0
    a = (1, 0, wx // map.width)
    b = (0, 1, wy // map.height)
    while a[2]:
	d = b[2] // a[2]
	a, b = (b[0] - d * a[0], b[1] - d * a[1], b[2] - d * a[2]), a
    assert abs(b[2]) == 1
    wx2 = b[1] * b[2]
    wy2 = -b[0] * b[2]
    assert wy // map.height * wx2 + (-wx // map.width) * wy2 == 1
    if r[1] < r[2]:
	p1 = res[r[1]:r[2] + 1]
	p2 = res[r[2]:] + res[:r[1] + 1]
    else:
	p1 = res[r[1]:] + res[:r[2] + 1]
	p2 = res[r[2]:r[1] + 1]
    p1[-1] = (p1[-1][0], p1[-1][1], (-r[3], -r[4]), 1)
    p2[-1] = (p2[-1][0], p2[-1][1], (r[3], r[4]), 1)
    wx2 *= map.width
    wy2 *= map.height
    p2 = [(p[0] + wx2, p[1] + wy2, p[2], p[3]) for p in p2]
    def minwrap(dx, dy, wx = wx, wy = wy, l2 = wx **2 + wy ** 2):
	d = int(round((wx * dx + wy * dy) / l2))
	return dx - d * wx, dy - d * wy
    r2 = findmin(p1, p2, 1e99, minwrap)
    assert (p1[r2[1]][0] + r2[3] - p2[r2[2]][0]) % map.width == 0
    assert (p1[r2[1]][1] + r2[4] - p2[r2[2]][1]) % map.height == 0
    res = p1[:r2[1]]
    res.append((p1[r2[1]][0], p1[r2[1]][1], (r2[3], r2[4]), 1))
    res += p2[r2[2]:] + p2[:r2[2]]
    res.append((p2[r2[2]][0], p2[r2[2]][1], (-r2[3], -r2[4]), 1))
    res += p1[r2[1]:]
    x, y = res[0][0], res[0][1]
    result = []
    for p in res:
	result.append((x, y, p[2], p[3]))
	x += p[2][0]
	y += p[2][1]
    return result

def poly_link(polys, map):
    neglist = []
    poslist = []
    zerolist = []
    for p in polys:
	direction = polydir(p)
	if direction > 0:
	    poslist.append(p)
	elif direction < 0:
	    neglist.append(p)
	else:
	    zerolist.append(p)
    if poslist and not zerolist:
	assert len(poslist) <= 1, "Can't have multiple + polygons in one part"
	def minwrap(dx, dy):
	    return dx, dy
	extrawrap = 0
    elif not zerolist and not poslist:
	def minwrap(dx, dy, wx = map.width, wy = map.height):
	    dx = (dx + (wx >> 1)) % wx - (wx >> 1)
	    dy = (dy + (wy >> 1)) % wy - (wy >> 1)
	    return dx, dy
	extrawrap = 1
    else:
	assert not poslist, "Can't have both zero and + polygons in one part"
	assert len(zerolist) == 2, "BUG"
	dx = dy = 0
	for p in zerolist[0]:
	    dx += p[2][0]
	    dy += p[2][1]
	assert abs(dx) + abs(dy) > 0, "Wrapping polygon with no wrap?"
	assert dx % map.width == 0, "Garbled coordinates"
	assert dy % map.height == 0, "Garbled coordinates"
	def minwrap(dx, dy, wx = dx, wy = dy, l2 = dx **2 + dy ** 2):
	    d = int(round((wx * dx + wy * dy) / l2))
	    return dx - d * wx, dy - d * wy
	extrawrap = 0
    totlist = [Polynode(p) for p in neglist + poslist + zerolist]
    res = last = totlist.pop()

    while totlist:
	for j in totlist:
	    result = findmin(last.poly, j.poly, j.dist[0], minwrap)
	    if result:
		j.dist = (result[0], result[1], result[2], result[3],
			  result[4], last)
	mindist = 1e99
	mink = -2.1
	for j in totlist:
	    if j.dist[0] < mindist:
		mink = j
		mindist = j.dist[0]
	totlist.remove(mink)
	r = mink.dist[5]
	r.linkstarts.append(mink.dist[1])
	s = Struct()
	(s.targetnode, s.targetindex, s.dx, s.dy) = (mink, mink.dist[2],
						    mink.dist[3], mink.dist[4])
	r.linktargets.append(s)
	last = mink
    result = []
    res.traverse(res.poly[0][0], res.poly[0][1], 0, result)
    if extrawrap:
	result = poly_handle_totalwrap(result, map)
    return result

def makepolys(map):
    print >> sys.stderr, "   Partitioning map...",
    mapparts, partcount = poly_partition(map)
    print >> sys.stderr, partcount, "parts."
    print >> sys.stderr, "   Finding edges...",
    partpolys = poly_findedges(map, mapparts, partcount)
    print >> sys.stderr, "done."
    allpolys = []
    print >> sys.stderr, "   Linking each part...",
    allpolys = [poly_link(polys, map) for polys in partpolys]
    print >> sys.stderr, "done."
    return allpolys

def convert(options):
    height = int(options['mapheight'])
    width = int(options['mapwidth'])
    map = options['mapdata'].splitlines()
    if options.get('edgewrap') not in ['yes', 'on', 'true']: #default off
	height += 2
	width += 2
	map = [' ' + line + ' ' for line in map]
	map = [' ' * width] + map + [' ' * width]

	options['mapdata'] = "\n".join(map)+'\n'
    options['mapwidth'] = `width * 35`
    options['mapheight'] = `height * 35`
    map = Map(map, width, height)

    print >> sys.stderr, "Finding special map features...",
    # First, find the locations of different map features other than walls
    # and put them in these lists.
    asteroidconcs = []
    bases = []
    balls = []
    cannons = []
    checks = [None] * 27  # 1 extra so it always ends with None
    decors = []
    emptytreasures = []
    fuels = []
    frictions = []
    gravs = []
    itemconcs = []
    targets = []
    wormholes = []
    ASTEROIDCONC = '&'
    BASES = '_0123456789'
    BALL = '*'
    CANNONS = 'rdfc'
    CHECKPOINTS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    DECOR = 'bhgyt'
    EMPTYTREASURE = '^'
    FUEL = '#'
    FRICTION = 'z'
    GRAVS = '+-><imkj'
    ITEMCONC = '%'
    SPACE = ' '
    TARGET = '!'
    WALLS = 'xsawq'       # WALL includes fuel block
    WORMHOLES = '@()'
    ALL = ASTEROIDCONC + BASES + BALL + CHECKPOINTS + CANNONS + DECOR + EMPTYTREASURE + FUEL + FRICTION + GRAVS + ITEMCONC + TARGET + WORMHOLES
    IGNORE = ATTRACT + SPACE + WALLS
    for x, y in map.ncoords():
	block = map.data[y][x]
	if block in IGNORE:
	    continue
	if block not in ALL:
	    print >>sys.stderr, "WARNING: Unknown block: '%s'"    % block
	    continue
	loc = Wrapcoords(map.width, map.height, x, y)
	thing = Struct()
	thing.x = loc.x * BCLICKS + BCLICKS // 2
	thing.y = (height - loc.y - 1) % height * BCLICKS + BCLICKS // 2
	thing.loc = loc
	thing.team = -1
	thing.type = ''
	if block == ASTEROIDCONC:
	    asteroidconcs.append(thing)
	elif block in BASES:
	    if map[loc] == '_':
		thing.team = 0
	    else:
		thing.team = ord(map[loc]) - ord('0')
	    thing.dir = 32
	    if map[loc.u()] in ATTRACT:
		thing.dir = 32
	    elif map[loc.d()] in ATTRACT:
		thing.dir = 96
	    elif map[loc.r()] in ATTRACT:
		thing.dir = 0
	    elif map[loc.l()] in ATTRACT:
		thing.dir = 64
	    thing.loc = loc
	    bases.append(thing)
	elif block == BALL:
	    balls.append(thing)
	elif block in CANNONS:
	    thing.dir = 'frdc'.index(block) * 32
	    cannons.append(thing)
	elif block in CHECKPOINTS:
	    checks[ord(map[loc]) - ord('A')] = thing
	elif block in DECOR:
	    print >>sys.stderr, "WARNING: can't yet convert decor."
	    continue
	elif block == EMPTYTREASURE:
	    print >>sys.stderr, "WARNING: can't yet convert empty treasure."
	    continue
	elif block == FUEL:
	    fuels.append(thing)
	elif block == FRICTION:
	    print >>sys.stderr, "WARNING: can't yet convert friction block."
	    continue
	elif block in GRAVS:
	    print >>sys.stderr, "WARNING: can't yet convert gravs."
	    continue
	elif block == ITEMCONC:
	    itemconcs.append(thing)
	elif block == TARGET:
	    targets.append(thing)
	elif block in WORMHOLES:
	    if map[loc] == '@':
		thing.type = 'normal'
	    elif map[loc] == '(':
		thing.type = 'in'
	    elif map[loc] == ')':
		thing.type = 'out'
	    wormholes.append(thing)
    if not bases:
	print >>sys.stderr, "Map has no bases???"
	sys.exit(1)
    if KEEP_MAPDATA:
	def sort_for_old(things):
	    things[:] = [(thing.x, thing.y, thing) for thing in things]
	    things.sort()
	    things[:] = [thing[-1] for thing in things]
	sort_for_old(bases)
	sort_for_old(fuels)
	sort_for_old(cannons)
    # Balls belong to the team that has the nearest base
    if options.get('teamplay'):
	for ball in balls:
	    ball.team = closestteam(ball.loc, bases)
	for target in targets:
	    target.team = closestteam(target.loc, bases)
    # In race mode, bases are ordered according to the distance from the
    # first checkpoint. Confuses order for old clients (above)
    if (options.get('timing') or options.get('race')) in ['yes', 'on', 'true']:
	bases = [(checks[0].loc.dist2(b.loc), b) for b in bases]
	bases.sort()
	bases = [b[1] for b in bases]
    # If teamfuel is on, the fuel belongs to the team with the nearest base
    if options.get('teamfuel') in ['yes', 'on', 'true']: #default off
	for fuel in fuels:
	    fuel.team = closestteam(fuel.loc, bases)
    if options.get('teamcannons') in ['yes', 'on', 'true']: #default off
	for cannon in cannons:
	    cannon.team = closestteam(cannon.loc, bases)

    print >> sys.stderr, "done."
    print >> sys.stderr, "Creating polygons:"
    polys = makepolys(map)

    # Turn this on if you want to randomize the map edges
    # Needs some changes to work now because of other changes elsewhere
    if 0:
	from math import sin
	for p in polys:
	    for l in p:
		l[0] = int(l[0] + sin(1. * l[0] * l[1] / 2786) * (2240 / 3)) % mxc
		l[1] = int(l[1] + sin(1. * l[0] * l[1] / 1523) * (2240 / 3)) % myc

    print >> sys.stderr, "Writing converted map...",
    print '<XPilotMap version="1.2">'

    if not KEEP_MAPDATA:
	del options['mapdata']

    print '<GeneralOptions>'

    def xmlencode(text):
	repl = (('&', '&amp;'), ('"', '&quot;'), ("'", '&apos;'),
		('<', '&lt;'), ('>', '&gt;'), ('\n', '&#xA;'))
	for t, r in repl:
	    text = text.replace(t, r)
	return text
    for name, value in options.items():
	print '<Option name="%s" value="%s"/>' % (name, xmlencode(value))
    print '</GeneralOptions>'
    print '<Edgestyle id="xpbluehidden" width="-1" color="4E7CFF" style="0"/>'
    print '<Edgestyle id="xpredhidden" width="-1" color="FF3A27" style="0"/>'
    print '<Edgestyle id="yellow" width="2" color="FFFF00" style="0"/>'
    print '<Edgestyle id="white" width="2" color="FFFFFF" style="0"/>'
    print '<Edgestyle id="invisiedge" width="-1" color="0" style="0"/>'
    print '<Polystyle id="xpblue" color="4E7CFF" defedge="xpbluehidden" flags="1"/>'
    print '<Polystyle id="xpred" color="FF3A27" defedge="xpredhidden" flags="1"/>'
    print '<Polystyle id="emptyyellow" color="FF" defedge="yellow" flags="0"/>'
    print '<Polystyle id="emptywhite" color="FF" defedge="white" flags="0"/>'
    print '<Polystyle id="invisible" color="0" defedge="invisiedge" flags="12"/>'

    def printedge(dx, dy, prevh, curh):
	if curh and not prevh:
	    sstr = ' style="internal"'
	elif prevh and not curh:
	    sstr = ' style="xpbluehidden"'
	else:
	    sstr = ''
	print '<Offset x="%d" y="%d"%s/>' % (dx, dy, sstr)

    for p in polys:
	while p[-1][2][0] * p[0][2][1] == p[-1][2][1] * p[0][2][0]:
	    p.insert(0, p.pop())
	edges = [(point[2][0] * BCLICKS, -point[2][1]* BCLICKS, point[3])
		 for point in p if point[2] != (0, 0)]
	x = (p[0][0] % map.width) * BCLICKS
	y = (-p[0][1] % map.height) * BCLICKS

	if WALL_SHRINK_HACK:
	    s = WALL_SHRINK_HACK
	    ch = [(-edge[1] // BCLICKS, edge[0] // BCLICKS) for edge in edges]
	    ch2 = []
	    for ((x1, y1), (x2, y2)) in zip(ch[-1:] + ch[:-1], ch):
		if x1 * x2 >= 0 and y1 * y2 >= 0:
		    ch2.append((x1 or x2, y1 or y2))
		elif x1 and x2 and y1 and y2:
		    ch2.append((x1 + x2, y1 + y2))
		else:
		    ch2.append(((x1 + x2) * 2 + x1 * (not y1) + x2 * (not y2
			), (y1 + y2) * 2 + y1 * (not x1) + y2 * (not x2)))
	    ch2 = [(x1 * s, y1 * s) for (x1, y1) in ch2]
	    x = (x + ch2[0][0]) % (map.width * BCLICKS)
	    y = (y + ch2[0][1]) % (map.height * BCLICKS)
	    ch2 = [(x2 - x1, y2 - y1) for ((x1,y1), (x2,y2)) in zip(ch2,
		ch2[1:] + ch2[:1])]
	    edges = [(edge[0] + ch[0], edge[1] + ch[1], edge[2]) for edge, ch
			in zip(edges, ch2)]
	print '<Polygon x="%d" y="%d" style="xpblue">' % (x, y)
	prevh = 0
	curh = edges[0][2]
	curx = cury = 0
	sstr = ''
	for edge in edges:
	    (dx, dy, h) = edge
	    if dx * cury != dy * curx or curh != h:
		printedge(curx, cury, prevh, curh)
		curx = dx
		cury = dy
		prevh = curh
		curh = h
	    else:
		curx += dx
		cury += dy
	printedge(curx, cury, prevh, curh)
	print "</Polygon>"
# The styles of these polygons will be changed later...
    for aconc in asteroidconcs:
	print '<AsteroidConcentrator x="%d" y="%d"/>' % (aconc.x, aconc.y)
    for ball in balls:
	print '<Ball team="%d" x="%d" y="%d"/>' % (ball.team, ball.x, ball.y - 479)
	print '<BallArea>'
	print '<Polygon x="%d" y="%d" style="xpblue">' % (ball.x - 1120, ball.y - 1120)
	print '<Offset x="2240" y="0"/> <Offset x="0" y="2240"/>'
	print '<Offset x="-2240" y="0"/> <Offset x="0" y="-2240"/>'
	print '</Polygon></BallArea>'
	print '<BallTarget team="%d">' % ball.team
	print '<Polygon x="%d" y="%d" style="xpred">' % (ball.x - 480, ball.y - 480)
	print '<Offset x="960" y="0"/> <Offset x="0" y="960"/>'
	print '<Offset x="-960" y="0"/> <Offset x="0" y="-960"/>'
	print '</Polygon></BallTarget>'
    for base in bases:
	print '<Base team="%d" x="%d" y="%d" dir = "%d"/>' % \
	      (base.team, base.x, base.y, base.dir)
    for fuel in fuels:
	print ('<Fuel x="%d" y="%d"' % (fuel.x, fuel.y)),
	sys.stdout.softspace = 0
	if fuel.team != -1:
	    print (' team="%d"' % fuel.team),
	print '/>'
    for target in targets:
	text = '<Target x="%d" y="%d"' % (target.x, target.y)
	if target.team != -1:
	    text += ' team="%d"' % target.team
	text += '>'
	print text
	print '<Polygon x="%d" y="%d" style="xpred">' % (target.x - 1120, target.y - 1120)
	print '<Style state="destroyed" id="invisible"/>'
	print '<Offset x="2240" y="0"/> <Offset x="0" y="2240"/>'
	print '<Offset x="-2240" y="0"/> <Offset x="0" y="-2240"/>'
	print '</Polygon></Target>'
    for check in checks:
	if not check:
	    break
	print '<Check x="%d" y="%d"/>' % (check.x, check.y)
    for cannon in cannons:
	offsets = ((-373, 0), (-747, 1120), (0, -2240), (747, 1120))
	for _ in range(cannon.dir // 32):
	    offsets = [(-y, x) for x, y in offsets]
	cannon.x += offsets[0][0]
	cannon.y += offsets[0][1]
	print ('<Cannon x="%d" y="%d" dir="%d"' % \
	       (cannon.x, cannon.y, cannon.dir)),
	sys.stdout.softspace = 0
	if cannon.team != -1:
	    print (' team="%d"' % cannon.team),
	print '>\n<Polygon x="%d" y="%d" style="emptywhite">' % \
	      (cannon.x, cannon.y)
	print '<Style state="destroyed" id="invisible"/>'
	for x, y in offsets[1:]:
	    print '<Offset x="%d" y="%d"/>' % (x, y)
	print '</Polygon></Cannon>'
    for iconc in itemconcs:
	print '<ItemConcentrator x="%d" y="%d"/>' % (iconc.x, iconc.y)
    for wormhole in wormholes:
	print '<Wormhole x="%d" y="%d" type="%s">' % \
		(wormhole.x, wormhole.y, wormhole.type)
	if wormhole.type != 'out':
	    print '<Polygon x="%d" y="%d" style="xpred">' % \
		(wormhole.x - 1120, wormhole.y - 1120)
	    print '<Offset x="2240" y="0"/> <Offset x="0" y="2240"/>'
	    print '<Offset x="-2240" y="0"/> <Offset x="0" y="-2240"/>'
	    print '</Polygon>'
	print '</Wormhole>'
    print "</XPilotMap>"
    print >> sys.stderr, "done."

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
	print "Give 1 argument, the map file name"
	sys.exit(1)
    file = open(sys.argv[1])
    lines = file.readlines()
    file.close()
    options = parse(lines)
    checkopts(options)
    convert(options)
