/*
 * XPilot NG XP-MapEdit, a map editor for xp maps.  Copyright (C) 1993 by
 *
 *      Aaron Averill           <averila@oes.orst.edu>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Modifications:
 * 1996:
 *      Robert Templeman        <mbcaprt@mphhpd.ph.man.ac.uk>
 * 1997:
 *      William Docter          <wad2@lehigh.edu>
 */

#include "xpmapedit.h"

static char *display_name = NULL;
char *progname;

Window mapwin, prefwin;

Window mapinfo, robots, visibility, cannons, rounds;
Window inititems, maxitems, probs, scoring;

Pixmap smlmap_pixmap;
int mapwin_width, mapwin_height;
int geometry_width, geometry_height;

GC Wall_GC, Decor_GC, Treasure_GC, Target_GC;
GC Item_Conc_GC, Fuel_GC, Gravity_GC, Current_GC;
GC Wormhole_GC, Base_GC, Cannon_GC, Friction_GC;
GC White_GC, Black_GC, xorgc;
char *fontname = "*-times-bold-r-*-*-14-*-*-*-*-*-*-*";

int drawicon, drawmode;
int prefssheet;
map_data_t clipdata;
xpmap_t map;

int num_default_settings = 259;
charlie default_settings[259] = {
    {"gravity", "-0.14"},
    {"shipmass", "20.0"},
    {"ballmass", "50.0"},
    {"shotmass", "0.1"},
    {"shotspeed", "21.0"},
    {"shotlife", "60"},
    {"firerepeatrate", "2"},
    {"maxrobots", "4"},
    {"minrobots", "-1"},
    {"robotstalk", "no"},
    {"robotsleave", "yes"},
    {"robotleavelife", "50"},
    {"robotleavescore", "-90"},
    {"robotleaveratio", "-5"},
    {"robotteam", "0"},
    {"restrictrobots", "yes"},
    {"reserverobotteam", "yes"},
    {"robotrealname", "robot"},
    {"robothostname", "xpilot.org"},
    {"tankrealname", "tank"},
    {"tankhostname", "tanks.org"},
    {"tankscoredecrement", "500"},
    {"turnthrust", "no"},
    {"selfimmunity", "no"},
    {"defaultshipshape",
     "(NM:Default)(AU:Unknown)(SH: 15,0 -9,8 -9,-8)(MG: 15,0)(LG: 15,0)(RG: 15,0)(EN: -9,0)(LR: -9,8)(RR: -9,-8)(LL: -9,8)(RL: -9,-8)(MR: 15,0)"},
    {"tankshipshape",
     "(NM:fueltank)(AU:John E. Norlin)(SH: 15,0 14,-5 9,-8 -5,-8 -3,-8 -3,0 2,0 2,2 -3,2 -3,6 5,6 5,8 -5,8 -5,-8 -9,-8 -14,-5 -15,0 -14,5 -9,8 9,8 14,5)(EN: -15,0)(MG: 15,0)"},
    {"maxplayershots", "256"},
    {"shotsgravity", "yes"},
    {"mapwidth", "100"},
    {"mapheight", "100"},
    {"mapname", "<New Map>"},
    {"mapauthor", "<Your Name>"},
    {"allowplayercrashes", "yes"},
    {"allowplayerbounces", "yes"},
    {"allowplayerkilling", "yes"},
    {"allowshields", "yes"},
    {"playerstartsshielded", "yes"},
    {"shotswallbounce", "no"},
    {"ballswallbounce", "yes"},
    {"ballcollisions", "no"},
    {"ballsparkcollisions", "yes"},
    {"mineswallbounce", "no"},
    {"itemswallbounce", "yes"},
    {"missileswallbounce", "no"},
    {"sparkswallbounce", "no"},
    {"debriswallbounce", "no"},
    {"asteroidswallbounce", "yes"},
    {"cloakedexhaust", "yes"},
    {"cloakedshield", "yes"},
    {"maxobjectwallbouncespeed", "40"},
    {"maxshieldedwallbouncespeed", "50"},
    {"maxunshieldedwallbouncespeed", "20"},
    {"maxshieldedplayerwallbounceangle", "90"},
    {"maxunshieldedplayerwallbounceangle", "30"},
    {"playerwallbouncebrakefactor", "0.89"},
    {"objectwallbouncebrakefactor", "0.95"},
    {"objectwallbouncelifefactor", "0.80"},
    {"wallbouncefueldrainmult", "1.0"},
    {"wallbouncedestroyitemprob", "0.0"},
    {"limitedvisibility", "no"},
    {"minvisibilitydistance", "0.0"},
    {"maxvisibilitydistance", "0.0"},
    {"limitedlives", "no"},
    {"worldlives", "0"},
    {"reset", "yes"},
    {"resetonhuman", "0"},
    {"allowalliances", "yes"},
    {"announcealliances", "no"},
    {"teamplay", "no"},
    {"teamcannons", "no"},
    {"teamfuel", "no"},
    {"cannonsmartness", "1"},
    {"cannonsuseitems", "no"},
    {"cannonsdefend", "yes"},
    {"cannonflak", "yes"},
    {"cannondeadtime", "72"},
    {"keepshots", "no"},
    {"teamassign", "yes"},
    {"teamimmunity", "yes"},
    {"teamsharescore", "no"},
    {"ecmsreprogrammines", "yes"},
    {"ecmsreprogramrobots", "yes"},
    {"targetkillteam", "no"},
    {"targetteamcollision", "yes"},
    {"targetsync", "no"},
    {"targetdeadtime", "60"},
    {"treasurekillteam", "no"},
    {"capturetheflag", "no"},
    {"treasurecollisiondestroys", "yes"},
    {"ballconnectorspringconstant", "1500.0"},
    {"ballconnectordamping", "2.0"},
    {"maxballconnectorratio", "0.30"},
    {"ballconnectorlength", "120"},
    {"connectorisstring", "no"},
    {"treasurecollisionmaykill", "no"},
    {"wreckagecollisionmaykill", "no"},
    {"asteroidcollisionmaykill", "yes"},
    {"timing", "no"},
    {"ballrace", "no"},
    {"ballraceconnected", "no"},
    {"edgewrap", "no"},
    {"edgebounce", "yes"},
    {"extraborder", "no"},
    {"gravitypoint", "0,0"},
    {"gravityangle", "90"},
    {"gravitypointsource", "no"},
    {"gravityclockwise", "no"},
    {"gravityanticlockwise", "no"},
    {"gravityvisible", "yes"},
    {"wormholevisible", "yes"},
    {"itemconcentratorvisible", "yes"},
    {"asteroidconcentratorvisible", "yes"},
    {"wormtime", "0"},
    {"framespersecond", "14"},
    {"allowsmartmissiles", "yes"},
    {"allowheatseekers", "yes"},
    {"allowtorpedoes", "yes"},
    {"allownukes", "no"},
    {"allowclusters", "no"},
    {"allowmodifiers", "no"},
    {"allowlasermodifiers", "no"},
    {"allowshipshapes", "yes"},
    {"playersonradar", "yes"},
    {"missilesonradar", "yes"},
    {"minesonradar", "no"},
    {"nukesonradar", "yes"},
    {"treasuresonradar", "no"},
    {"asteroidsonradar", "no"},
    {"distinguishmissiles", "yes"},
    {"maxmissilesperpack", "4"},
    {"maxminesperpack", "2"},
    {"identifymines", "yes"},
    {"shieldeditempickup", "no"},
    {"shieldedmining", "no"},
    {"laserisstungun", "no"},
    {"nukeminsmarts", "7"},
    {"nukeminmines", "4"},
    {"nukeclusterdamage", "1.0"},
    {"minefusetime", "0.0"},
    {"minelife", "0"},
    {"minminespeed", "0"},
    {"missilelife", "0"},
    {"baseminerange", "0"},
    {"mineshotdetonatedistance", "0"},
    {"shotkillscoremult", "1.0"},
    {"torpedokillscoremult", "1.0"},
    {"smartkillscoremult", "1.0"},
    {"heatkillscoremult", "1.0"},
    {"clusterkillscoremult", "1.0"},
    {"laserkillscoremult", "1.0"},
    {"tankkillscoremult", "0.44"},
    {"runoverkillscoremult", "0.33"},
    {"ballkillscoremult", "1.0"},
    {"explosionkillscoremult", "0.33"},
    {"shovekillscoremult", "0.5"},
    {"crashscoremult", "0.33"},
    {"minescoremult", "0.17"},
    {"asteroidpoints", "1.0"},
    {"cannonpoints", "1.0"},
    {"asteroidmaxscore", "100.0"},
    {"cannonmaxscore", "100.0"},
    {"movingitemprob", "0.2"},
    {"dropitemonkillprob", "0.5"},
    {"randomitemprob", "0.0"},
    {"detonateitemonkillprob", "0.5"},
    {"destroyitemincollisionprob", "0.0"},
    {"asteroiditemprob", "0.0"},
    {"asteroidmaxitems", "0"},
    {"itemprobmult", "1.0"},
    {"cannonitemprobmult", "1.0"},
    {"maxitemdensity", "0.00012"},
    {"asteroidprob", "5e-7"},
    {"maxasteroiddensity", "0"},
    {"itemconcentratorradius", "10"},
    {"itemconcentratorprob", "1.0"},
    {"asteroidconcentratorradius", "10"},
    {"asteroidconcentratorprob", "1.0"},
    {"rogueheatprob", "1.0"},
    {"roguemineprob", "1.0"},
    {"itemenergypackprob", "1e-9"},
    {"itemtankprob", "1e-9"},
    {"itemecmprob", "1e-9"},
    {"itemarmorprob", "1e-9"},
    {"itemmineprob", "1e-9"},
    {"itemmissileprob", "1e-9"},
    {"itemcloakprob", "1e-9"},
    {"itemsensorprob", "1e-9"},
    {"itemwideangleprob", "1e-9"},
    {"itemrearshotprob", "1e-9"},
    {"itemafterburnerprob", "1e-9"},
    {"itemtransporterprob", "1e-9"},
    {"itemmirrorprob", "1e-9"},
    {"itemdeflectorprob", "1e-9"},
    {"itemhyperjumpprob", "1e-9"},
    {"itemphasingprob", "1e-9"},
    {"itemlaserprob", "1e-9"},
    {"itememergencythrustprob", "1e-9"},
    {"itemtractorbeamprob", "1e-9"},
    {"itemautopilotprob", "1e-9"},
    {"itememergencyshieldprob", "1e-9"},
    {"initialfuel", "1000"},
    {"initialtanks", "0"},
    {"initialarmor", "0"},
    {"initialecms", "0"},
    {"initialmines", "0"},
    {"initialmissiles", "0"},
    {"initialcloaks", "0"},
    {"initialsensors", "0"},
    {"initialwideangles", "0"},
    {"initialrearshots", "0"},
    {"initialafterburners", "0"},
    {"initialtransporters", "0"},
    {"initialmirrors", "0"},
    {"maxarmor", "10"},
    {"initialdeflectors", "0"},
    {"initialhyperjumps", "0"},
    {"initialphasings", "0"},
    {"initiallasers", "0"},
    {"initialemergencythrusts", "0"},
    {"initialtractorbeams", "0"},
    {"initialautopilots", "0"},
    {"initialemergencyshields", "0"},
    {"maxfuel", "10000"},
    {"maxtanks", "8"},
    {"maxecms", "10"},
    {"maxmines", "10"},
    {"maxmissiles", "10"},
    {"maxcloaks", "10"},
    {"maxsensors", "10"},
    {"maxwideangles", "10"},
    {"maxrearshots", "10"},
    {"maxafterburners", "10"},
    {"maxtransporters", "10"},
    {"maxdeflectors", "10"},
    {"maxphasings", "10"},
    {"maxhyperjumps", "10"},
    {"maxemergencythrusts", "10"},
    {"maxlasers", "5"},
    {"maxtractorbeams", "4"},
    {"maxautopilots", "10"},
    {"maxemergencyshields", "10"},
    {"maxmirrors", "10"},
    {"gameduration", "0.0"},
    {"allowviewing", "no"},
    {"friction", "0.0"},
    {"blockfriction", "0.0"},
    {"blockfrictionvisible", "true"},
    {"coriolis", "0"},
    {"checkpointradius", "6.0"},
    {"racelaps", "3"},
    {"lockotherteam", "yes"},
    {"loseitemdestroys", "no"},
    {"usewreckage", "yes"},
    {"maxoffensiveitems", "100"},
    {"maxdefensiveitems", "100"},
    {"rounddelay", "0"},
    {"maxroundtime", "0"},
    {"roundstoplay", "0"},
    {"maxpausetime", "3600"}
};

/* JLM Reorganized for new options */
int numprefs = 260;
prefs_t prefs[260] = {
    {"mapwidth", "", "Width:", 3, MAPWIDTH, map.width_str, 0, 0, 0, 0, 0},
    {"mapheight", "", "Height:", 3, MAPHEIGHT, map.height_str, 0, 1, 0, 0,
     0},
    {"mapname", "", "Name:", 255, STRING, map.mapName, 0, 2, 0, 0, 0},
    {"mapauthor", "", "Author:", 255, STRING, map.mapAuthor, 0, 3, 0, 0,
     0},
    {"limitedlives", "", "Limited Lives?", 0, YESNO, 0, &map.limitedLives,
     8, 0, 0, 0},
    {"worldlives", "lives", "Lives:", 3, POSINT, map.worldLives, 0, 9, 0,
     0, 0},
    {"selfimmunity", "", "Self Immunity?", 0, YESNO, 0, &map.selfImmunity,
     10, 0, 0, 0},
    {"gravity", "", "Gravity:", 6, FLOAT, map.gravity, 0, 0, 1, 0, 0},
    {"gravityangle", "", "Gravity Angle:", 3, POSINT, map.gravityAngle, 0,
     1, 1, 0, 0},
    {"gravitypoint", "", "Gravity Point:", 7, COORD, map.gravityPoint, 0,
     2, 1, 0, 0},
    {"gravitypointsource", "", "Point Source?", 0, YESNO, 0,
     &map.gravityPointSource, 3, 1, 0, 0},
    {"gravityclockwise", "", "Clockwise?", 0, YESNO, 0,
     &map.gravityClockwise, 4, 1, 0, 0},
    {"gravityanticlockwise", "", "Anti-Clockwise?", 0, YESNO, 0,
     &map.gravityAnticlockwise, 5, 1, 0, 0},
    {"shotsgravity", "", "Shots Gravity?", 0, YESNO, 0, &map.shotsGravity,
     6, 1, 0, 0},
    {"gravityvisible", "", "Gravity Visible?", 6, YESNO, 0,
     &map.gravityVisible, 7, 1, 0, 0},
    {"coriolis", "", "Coriolis:", 6, INT, map.coriolis, 0, 8, 1, 0, 0},
    {"friction", "", "Friction:", 19, POSFLOAT, map.friction, 0, 10, 1, 0,
     0},
    {"blockfriction", "", "Block Friction:", 19, POSFLOAT,
     map.blockFriction, 0, 11, 1, 0, 0},
    {"defaultshipshape", "", "DefaultShipShape:", 255, STRING,
     map.defaultShipShape, 0, 13, 1, 0, 0},
    {"tankshipshape", "", "TankShipShape:", 255, STRING, map.tankShipShape,
     0, 14, 1, 0, 0},
    {"shipmass", "", "Ship Mass:", 6, POSFLOAT, map.shipMass, 0, 0, 2, 0,
     0},
    {"shotmass", "", "Shot Mass:", 6, POSFLOAT, map.shotMass, 0, 2, 2, 0,
     0},
    {"shotspeed", "", "Shot Speed:", 6, FLOAT, map.shotSpeed, 0, 3, 2, 0,
     0},
    {"shotlife", "", "Shot Life:", 3, POSINT, map.shotLife, 0, 4, 2, 0, 0},
    {"maxplayershots", "shots", "Max. Shots:", 3, POSINT,
     map.maxPlayerShots, 0, 5, 2, 0, 0},
    {"firerepeatrate", "firerepeat", "Fire Repeat Rate:", 19, POSINT,
     map.fireRepeatRate, 0, 6, 2, 0, 0},
    {"keepshots", "", "KeepShots", 0, YESNO, 0, &map.keepShots, 7, 2, 0,
     0},
    {"edgebounce", "", "Edge Bounce?", 0, YESNO, 0, &map.edgeBounce, 9, 2,
     0, 0},
    {"edgewrap", "", "Edge Wrap?", 0, YESNO, 0, &map.edgeWrap, 10, 2, 0,
     0},
    {"extraborder", "", "Extra Border?", 0, YESNO, 0, &map.extraBorder, 11,
     2, 0, 0},
    {"turnthrust", "turnfuel", "TurnThrust?", 0, YESNO, 0, &map.turnThrust,
     13, 2, 0, 0},

    {"robotstalk", "", "Robots Talk?", 0, YESNO, 0, &map.robotsTalk, 0, 0,
     1, 0},
    {"robotsleave", "", "Robots Leave?", 0, YESNO, 0, &map.robotsLeave, 1,
     0, 1, 0},
    {"robotleavelife", "", "Robot Leave Life:", 19, POSINT,
     map.robotLeaveLife, 0, 2, 0, 1, 0},
    {"robotleavescore", "", "Robot Leave Score:", 19, INT,
     map.robotLeaveScore, 0, 3, 0, 1, 0},
    {"robotleaveratio", "", "Robot Leave Ratio:", 19, POSFLOAT,
     map.robotLeaveRatio, 0, 4, 0, 1, 0},
    {"robotteam", "", "Robot Team:", 2, POSINT, map.robotTeam, 0, 5, 0, 1,
     0},
    {"restrictrobots", "", "Restrict Robots?", 0, YESNO, 0,
     &map.restrictRobots, 6, 0, 1, 0},
    {"reserverobotteam", "", "Resrve Rob Team?", 0, YESNO, 0,
     &map.reserveRobotTeam, 7, 0, 1, 0},
    {"minrobots", "", "Min. Robots", 19, FLOAT, map.minRobots, 0, 8, 0, 1,
     0},
    {"maxrobots", "robots", "Max. Robots:", 2, POSINT, map.maxRobots, 0, 9,
     0, 1, 0},
    {"robotrealname", "", "RobotRealName:", 255, STRING, map.robotRealName,
     0, 11, 0, 1, 0},
    {"robothostname", "", "RobotHostName:", 255, STRING, map.robotHostName,
     0, 12, 0, 1, 0},
    {"shotswallbounce", "", "Shots Bounce?", 0, YESNO, 0,
     &map.shotsWallBounce, 0, 1, 1, 0},
    {"ballswallbounce", "", "Balls Bounce?", 0, YESNO, 0,
     &map.ballsWallBounce, 1, 1, 1, 0},
    {"mineswallbounce", "", "Mines Bounce?", 0, YESNO, 0,
     &map.minesWallBounce, 2, 1, 1, 0},
    {"itemswallbounce", "", "Items Bounce?", 0, YESNO, 0,
     &map.itemsWallBounce, 3, 1, 1, 0},
    {"missileswallbounce", "", "Missiles Bounce?", 0, YESNO, 0,
     &map.missilesWallBounce, 4, 1, 1, 0},
    {"sparkswallbounce", "", "Sparks Bounce?", 0, YESNO, 0,
     &map.sparksWallBounce, 5, 1, 1, 0},
    {"debriswallbounce", "", "Debris Bounce?", 0, YESNO, 0,
     &map.debrisWallBounce, 6, 1, 1, 0},
    {"asteroidswallbounce", "", "Asteroids Bounce?", 0, YESNO, 0,
     &map.asteroidsWallBounce, 7, 1, 1, 0},
    {"wreckagecollisionmaykill", "wreckageunshieldedcollisionkills",
     "Wreck Col Kills?", 0, YESNO, 0, &map.wreckageCollisionMayKill, 9, 1,
     1, 0},
    {"tankrealname", "", "TankRealName:", 255, STRING, map.tankRealName, 0,
     10, 1, 1, 0},
    {"tankhostname", "", "TankHostName:", 255, STRING, map.tankHostName, 0,
     11, 1, 1, 0},
    {"maxobjectwallbouncespeed", "maxobjectbouncespeed", "MxObjBnceSpd:",
     19, POSFLOAT, map.maxObjectWallBounceSpeed, 0, 0, 2, 1, 0},
    {"maxshieldedwallbouncespeed", "maxshieldedbouncespeed",
     "MxShldBnceSpd:", 19, POSFLOAT, map.maxShieldedWallBounceSpeed, 0, 1,
     2, 1, 0},
    {"maxunshieldedwallbouncespeed", "maxunshieldedbouncespeed",
     "MxUnshBnceSpd:", 19, POSFLOAT, map.maxUnshieldedWallBounceSpeed, 0,
     2, 2, 1, 0},
    {"maxshieldedplayerwallbounceangle", "maxshieldedbounceangle",
     "MxShldBnceAng:", 19, POSFLOAT, map.maxShieldedPlayerWallBounceAngle,
     0, 3, 2, 1, 0},
    {"maxunshieldedplayerwallbounceangle", "maxunshieldedbounceangle",
     "MxUnshBnceAng:", 19, POSFLOAT,
     map.maxUnshieldedPlayerWallBounceAngle, 0, 4, 2, 1, 0},
    {"playerwallbouncebrakefactor", "playerwallbrake", "Plyr Brake Fact:",
     19, POSFLOAT, map.playerWallBounceBrakeFactor, 0, 5, 2, 1, 0},
    {"objectwallbouncebrakefactor", "objectwallbrake", "Obj Brake Fact:",
     19, POSFLOAT, map.objectWallBounceBrakeFactor, 0, 6, 2, 1, 0},
    {"objectwallbouncelifefactor", "", "Obj Life Fact:", 19, POSFLOAT,
     map.objectWallBounceLifeFactor, 0, 7, 2, 1, 0},
    {"wallbouncefueldrainmult", "wallbouncedrail", "Fuel Drain Mult:", 19,
     POSFLOAT, map.wallBounceFuelDrainMult, 0, 8, 2, 1, 0},
    {"wallbouncedestroyitemprob", "", "BnceDestItmProb:", 19, POSFLOAT,
     map.wallBounceDestroyItemProb, 0, 9, 2, 1, 0},
    {"loseitemdestroys", "", "Lose Item Dests?", 0, YESNO, 0,
     &map.loseItemDestroys, 11, 2, 1, 0},

    {"limitedvisibility", "", "Limited Visibility?", 0, YESNO, 0,
     &map.limitedVisibility, 0, 0, 2, 0},
    {"minvisibilitydistance", "minvisibility", "Min Visibility Dist:", 19,
     POSFLOAT, map.minVisibilityDistance, 0, 1, 0, 2, 0},
    {"maxvisibilitydistance", "maxvisibility", "Max Visibility Dist:", 19,
     POSFLOAT, map.maxVisibilityDistance, 0, 2, 0, 2, 0},
    {"wormholevisible", "", "Wormhole Visible?", 6, YESNO, 0,
     &map.wormholeVisible, 3, 0, 2, 0},
    {"itemconcentratorvisible", "", "Item Conc Vis?", 6, YESNO, 0,
     &map.itemConcentratorVisible, 4, 0, 2, 0},
    {"blockfrictionvisible", "", "BlockFriction Vis?", 6, YESNO, 0,
     &map.blockFrictionVisible, 5, 0, 2, 0},
    {"wormtime", "", "Wormhole Time:", 19, POSINT, map.wormTime, 0, 7, 0,
     2, 0},
    {"playerstartsshielded", "playerstartshielded", "Start Shielded?", 0,
     YESNO, 0, &map.playerStartsShielded, 9, 0, 2, 0},
    {"shieldeditempickup", "shielditem", "Shielded Pickup?", 0, YESNO, 0,
     &map.shieldedItemPickup, 10, 0, 2, 0},
    {"shieldedmining", "shieldmine", "Shielded Mining?", 0, YESNO, 0,
     &map.shieldedMining, 11, 0, 2, 0},
    {"allowalliances", "alliances", "AllowAlliances?", 0, YESNO, 0,
     &map.allowAlliances, 13, 0, 2, 0},
    {"announcealliances", "", "AnnounceAlliances?", 0, YESNO, 0,
     &map.announceAlliances, 14, 0, 2, 0},
    {"targetkillteam", "", "Target Kill Team?", 0, YESNO, 0,
     &map.targetKillTeam, 0, 1, 2, 0},
    {"targetteamcollision", "targetcollision", "Target Collision?", 0,
     YESNO, 0, &map.targetTeamCollision, 1, 1, 2, 0},
    {"targetsync", "", "Target Sync?", 0, YESNO, 0, &map.targetSync, 2, 1,
     2, 0},
    {"targetdeadtime", "", "Target Dead Time:", 19, POSINT,
     map.targetDeadTime, 0, 3, 1, 2, 0},
    {"treasurekillteam", "", "Treas. Kill Team?", 0, YESNO, 0,
     &map.treasureKillTeam, 5, 1, 2, 0},
    {"treasurecollisiondestroys", "treasurecollisiondestroy",
     "Tres Col Dests?", 0, YESNO, 0, &map.treasureCollisionDestroys, 6, 1,
     2, 0},
    {"treasurecollisionmaykill", "treasureunshieldedcollisionkills",
     "Tres Col Kills?", 0, YESNO, 0, &map.treasureCollisionMayKill, 7, 1,
     2, 0},
    {"ballconnectorlength", "", "BallCnctrLngth:", 19, POSFLOAT,
     map.ballConnectorLength, 0, 8, 1, 2, 0},
    {"maxballconnectorratio", "", "MxBallCnctrRatio:", 19, POSFLOAT,
     map.maxBallConnectorRatio, 0, 9, 1, 2, 0},
    {"ballconnectordamping", "", "BallCnctrDmping:", 19, POSFLOAT,
     map.ballConnectorDamping, 0, 10, 1, 2, 0},
    {"ballconnectorspringconstant", "", "BallCtrSprngCnst:", 19, POSFLOAT,
     map.ballConnectorSpringConstant, 0, 11, 1, 2, 0},
    {"connectorisstring", "", "Cnctr Is String?", 0, YESNO, 0,
     &map.connectorIsString, 12, 1, 2, 0},
    {"ballcollisions", "", "Ball Collisions?", 0, YESNO, 0,
     &map.ballCollisions, 13, 1, 2, 0},
    {"ballsparkcollisions", "", "Ball Spark Collisions?", 0, YESNO, 0,
     &map.ballSparkCollisions, 14, 1, 2, 0},
    {"ballmass", "", "BallMass:", 19, POSFLOAT, map.ballMass, 0, 15, 1, 2,
     0},
    {"playersonradar", "playersradar", "Players on Radar?", 0, YESNO, 0,
     &map.playersOnRadar, 0, 2, 2, 0},
    {"missilesonradar", "missilesradar", "Missles on Radar?", 0, YESNO, 0,
     &map.missilesOnRadar, 1, 2, 2, 0},
    {"minesonradar", "minesradar", "Mines on Radar?", 0, YESNO, 0,
     &map.minesOnRadar, 2, 2, 2, 0},
    {"nukesonradar", "nukesradar", "Nukes on Radar?", 0, YESNO, 0,
     &map.nukesOnRadar, 3, 2, 2, 0},
    {"treasuresonradar", "treasuresradar", "Treas. on Radar?", 0, YESNO, 0,
     &map.treasuresOnRadar, 4, 2, 2, 0},
    {"teamplay", "teams", "Team Play?", 0, YESNO, 0, &map.teamPlay, 6, 2,
     2, 0},
    {"teamassign", "", "Team Assign?", 0, YESNO, 0, &map.teamAssign, 7, 2,
     2, 0},
    {"teamimmunity", "", "Team Immunity?", 0, YESNO, 0, &map.teamImmunity,
     8, 2, 2, 0},
    {"teamcannons", "", "Team Cannons?", 0, YESNO, 0, &map.teamCannons, 9,
     2, 2, 0},
    {"teamfuel", "", "Team Fuel?", 0, YESNO, 0, &map.teamFuel, 10, 2, 2,
     0},
    {"capturetheflag", "ctf", "Capture The Flag?", 0, YESNO, 0,
     &map.captureTheFlag, 11, 2, 2, 0},
    {"cloakedexhaust", "", "Cloaked Exhaust?", 6, YESNO, 0,
     &map.cloakedExhaust, 13, 2, 2, 0},
    {"cloakedshield", "", "Cloaked Shield?", 6, YESNO, 0,
     &map.cloakedShield, 14, 2, 2, 0},

    {"cannonsuseitems", "cannonspickupitems", "Cannons Use Items?", 0,
     YESNO, 0, &map.cannonsUseItems, 0, 0, 3, 0},
    {"cannonsdefend", "", "Cannons Defend?", 0, YESNO, 0,
     &map.cannonsDefend, 1, 0, 3, 0},
    {"cannonsmartness", "", "CannonSmartness:", 19, POSINT,
     map.cannonSmartness, 0, 2, 0, 3, 0},
    {"cannondeadtime", "", "CannonDeadTime:", 19, POSINT,
     map.cannonDeadTime, 0, 3, 0, 3, 0},
    {"cannonflak", "cannonaaa", "Cannon Flak?", 0, YESNO, 0,
     &map.cannonFlak, 4, 0, 3, 0},
    {"identifymines", "", "Identify Mines?", 0, YESNO, 0,
     &map.identifyMines, 5, 0, 3, 0},
    {"maxminesperpack", "", "Mines/Pac", 19, POSINT, map.maxMinesPerPack,
     0, 6, 0, 3, 0},
    {"minelife", "", "Mine Life", 19, POSFLOAT, map.mineLife, 0, 7, 0, 3,
     0},
    {"minefusetime", "", "Mine Fuse Time:", 19, POSINT, map.mineFuseTime,
     0, 8, 0, 3, 0},
    {"baseminerange", "", "Base Mine Range:", 19, POSINT,
     map.baseMineRange, 0, 9, 0, 3, 0},
    {"mineshotdetonatedistance", "", "MineShotDetDist:", 19, POSINT,
     map.mineShotDetonateDistance, 0, 10, 0, 3, 0},
    {"roguemineprob", "", "Rogue Mine Prob:", 19, POSFLOAT,
     map.rogueMineProb, 0, 11, 0, 3, 0},
    {"nukeminmines", "", "Min Nuke Mines:", 6, POSINT, map.nukeMinMines, 0,
     12, 0, 3, 0},
    {"minminespeed", "", "Min Mine Speed:", 19, POSFLOAT, map.minMineSpeed,
     0, 13, 0, 3, 0},
    {"nukeclusterdamage", "", "Nuke Clust Dam:", 19, POSFLOAT,
     map.nukeClusterDamage, 0, 14, 0, 3, 0},
    {"ecmsreprogramrobots", "", "EcmsReprgmRbts?", 6, YESNO, 0,
     &map.ecmsReprogramRobots, 0, 1, 3, 0},
    {"ecmsreprogrammines", "", "EcmsRprgMines?", 0, YESNO, 0,
     &map.ecmsReprogramMines, 1, 1, 3, 0},
    {"distinguishmissiles", "", "Distng Missiles?", 0, YESNO, 0,
     &map.distinguishMissiles, 3, 1, 3, 0},
    {"maxmissilesperpack", "", "Missiles/Pack:", 6, INT,
     map.maxMissilesPerPack, 0, 4, 1, 3, 0},
    {"missilelife", "", "Missile Life", 19, POSFLOAT, map.missileLife, 0,
     5, 1, 3, 0},
    {"rogueheatprob", "", "Rogue Heat Prob:", 19, POSFLOAT,
     map.rogueHeatProb, 0, 6, 1, 3, 0},
    {"nukeminsmarts", "", "Min Nuke Miss:", 6, POSINT, map.nukeMinSmarts,
     0, 7, 1, 3, 0},
    {"asteroidcollisionmaykill", "asteroidunshieldedcollisionkills",
     "Asteroid Col Kills?", 0, YESNO, 0, &map.asteroidCollisionMayKill, 9,
     1, 3, 0},
    {"asteroidsonradar", "asteroidsradar", "Asteroids on Radar?", 0, YESNO,
     0, &map.asteroidsOnRadar, 10, 1, 3, 0},
    {"maxasteroiddensity", "", "MaxAsteroidDnsty:", 19, POSFLOAT,
     map.maxAsteroidDensity, 0, 11, 1, 3, 0},
    {"asteroidconcentratorvisible", "", "AsteroidConcVis?", 0, YESNO, 0,
     &map.asteroidConcentratorVisible, 12, 1, 3, 0},
    {"asteroidconcentratorradius", "asteroidconcentratorrange",
     "AstrdConcRadius:", 19, POSINT, map.asteroidConcentratorRadius, 0, 13,
     1, 3, 0},
    {"asteroidmaxitems", "", "AstrdMaxItems:", 19, POSINT,
     map.asteroidMaxItems, 0, 14, 1, 3, 0},

    {"allowshipshapes", "shipshapes", "Ship Shapes?", 0, YESNO, 0,
     &map.allowShipShapes, 0, 2, 3, 0},
    {"allowsmartmissiles", "allowsmarts", "Allow Smarts?", 6, YESNO, 0,
     &map.allowSmartMissiles, 1, 2, 3, 0},
    {"allowheatseekers", "allowheats", "Allow Heats?", 6, YESNO, 0,
     &map.allowHeatSeekers, 2, 2, 3, 0},
    {"allowtorpedoes", "allowtorps", "Allow Torps?", 6, YESNO, 0,
     &map.allowTorpedoes, 3, 2, 3, 0},
    {"allowplayercrashes", "", "Allow Crashes?", 0, YESNO, 0,
     &map.allowPlayerCrashes, 4, 2, 3, 0},
    {"allowplayerbounces", "", "Allow Bounces?", 0, YESNO, 0,
     &map.allowPlayerBounces, 5, 2, 3, 0},
    {"allowplayerkilling", "killings", "Allow Killing?", 0, YESNO, 0,
     &map.allowPlayerKilling, 6, 2, 3, 0},
    {"allowshields", "shields", "Allow Shields?", 0, YESNO, 0,
     &map.allowShields, 7, 2, 3, 0},
    {"allownukes", "nukes", "Allow Nukes?", 0, YESNO, 0, &map.allowNukes,
     8, 2, 3, 0},
    {"allowclusters", "clusters", "Allow Clusters?", 0, YESNO, 0,
     &map.allowClusters, 9, 2, 3, 0},
    {"allowmodifiers", "", "Allow Mods?", 0, YESNO, 0, &map.allowModifiers,
     10, 2, 3, 0},
    {"allowlasermodifiers", "lasermodifiers", "Laser Mods?", 0, YESNO, 0,
     &map.allowLaserModifiers, 11, 2, 3, 0},
    {"laserisstungun", "stungun", "Laser Stun Gun?", 0, YESNO, 0,
     &map.laserIsStunGun, 13, 2, 3, 0},

    {"maxroundtime", "", "MaxRoundTime:", 19, POSINT, map.maxRoundTime, 0,
     0, 0, 4, 0},
    {"gameduration", "", "Game Duration:", 19, POSFLOAT, map.gameDuration,
     0, 1, 0, 4, 0},
    {"rounddelay", "", "Round Delay:", 19, POSINT, map.roundDelay, 0, 2, 0,
     4, 0},
    {"roundstoplay", "", "Rounds2Play:", 19, POSINT, map.roundsToPlay, 0,
     3, 0, 4, 0},
    {"reset", "", "World Reset?", 0, YESNO, 0, &map.reset, 5, 0, 4, 0},
    {"resetonhuman", "humanreset", "ResetOnHuman:", 6, POSINT,
     map.resetOnHuman, 0, 6, 0, 4, 0},
    {"maxpausetime", "", "MaxPauseTime:", 19, POSINT, map.maxPauseTime, 0,
     7, 0, 4, 0},
    {"timing", "race", "Race-Timing?", 0, YESNO, 0, &map.timing, 0, 1, 4,
     0},
    {"checkpointradius", "", "Checkpoint Rad:", 19, POSFLOAT,
     map.checkpointRadius, 0, 1, 1, 4, 0},
    {"racelaps", "", "Race Laps:", 6, POSINT, map.raceLaps, 0, 2, 1, 4, 0},
    {"ballrace", "", "BallRace?", 0, YESNO, 0, &map.ballrace, 3, 1, 4, 0},
    {"ballraceconnected", "", "BallRaceCnctd?", 0, YESNO, 0,
     &map.ballraceConnected, 4, 1, 4, 0},
    {"itemconcentratorradius", "itemconcentratorrange", "Item Con Radius:",
     19, POSFLOAT, map.itemConcentratorRadius, 0, 6, 1, 4, 0},
    {"usewreckage", "", "Use Wreckage?", 6, YESNO, 0, &map.useWreckage, 2,
     2, 4, 0},
    {"lockotherteam", "", "Lock Other Team?", 0, YESNO, 0,
     &map.lockOtherTeam, 3, 2, 4, 0},
    {"allowviewing", "", "Allow Viewing?", 0, YESNO, 0, &map.allowViewing,
     4, 2, 4, 0},
    {"framespersecond", "FPS", "Frames/Second:", 6, POSINT,
     map.framesPerSecond, 0, 6, 2, 4, 0},

    {"initialfuel", "", "Initial Fuel:", 19, POSFLOAT, map.initialFuel, 0,
     0, 0, 5, 0},
    {"initialtanks", "", "Initial Tanks:", 19, POSFLOAT, map.initialTanks,
     0, 1, 0, 5, 0},
    {"initialecms", "", "Initial ECMs:", 19, POSFLOAT, map.initialECMs, 0,
     2, 0, 5, 0},
    {"initialmines", "", "Initial Mines:", 19, POSFLOAT, map.initialMines,
     0, 3, 0, 5, 0},
    {"initialmissiles", "", "Initial Missiles:", 19, POSFLOAT,
     map.initialMissiles, 0, 4, 0, 5, 0},
    {"initialcloaks", "", "Initial Cloaks:", 19, POSFLOAT,
     map.initialCloaks, 0, 5, 0, 5, 0},
    {"initialsensors", "", "Initial Sensors:", 19, POSFLOAT,
     map.initialSensors, 0, 6, 0, 5, 0},
    {"initialwideangles", "", "Initial Wideangles:", 19, POSFLOAT,
     map.initialWideangles, 0, 7, 0, 5, 0},
    {"initialrearshots", "", "Initial Rearshots:", 19, POSFLOAT,
     map.initialRearshots, 0, 8, 0, 5, 0},
    {"initialafterburners", "", "Initial Aftrbrnrs:", 19, POSFLOAT,
     map.initialAfterburners, 0, 9, 0, 5, 0},
    {"initialtransporters", "", "Initial Trnsprtrs:", 19, POSFLOAT,
     map.initialTransporters, 0, 0, 1, 5, 0},
    {"initialdeflectors", "", "Initial Deflectors:", 19, POSINT,
     map.initialDeflectors, 0, 1, 1, 5, 0},
    {"initialphasings", "", "Initial Phasings:", 19, POSINT,
     map.initialPhasings, 0, 2, 1, 5, 0},
    {"initialhyperjumps", "", "Init. HyperJumps:", 19, POSINT,
     map.initialHyperJumps, 0, 3, 1, 5, 0},
    {"initialemergencythrusts", "", "Init. EmerThrust:", 19, POSFLOAT,
     map.initialEmergencyThrusts, 0, 4, 1, 5, 0},
    {"initiallasers", "", "Initial Lasers:", 19, POSFLOAT,
     map.initialLasers, 0, 5, 1, 5, 0},
    {"initialtractorbeams", "", "Initial TractorBms:", 19, POSFLOAT,
     map.initialTractorBeams, 0, 6, 1, 5, 0},
    {"initialautopilots", "", "Initial Autopilot:", 19, POSFLOAT,
     map.initialAutopilots, 0, 7, 1, 5, 0},
    {"initialemergencyshields", "", "Init. EmerShields:", 19, POSFLOAT,
     map.initialEmergencyShields, 0, 8, 1, 5, 0},
    {"initialmirrors", "", "Initial Mirrors:", 19, POSINT,
     map.initialMirrors, 0, 9, 1, 5, 0},
    {"initialarmor", "initialarmors", "Initial Armor:", 19, POSINT,
     map.initialArmor, 0, 0, 2, 5, 0},

    {"maxfuel", "", "Max Fuel:", 19, POSINT, map.maxFuel, 0, 0, 0, 6, 0},
    {"maxtanks", "", "Max Tanks:", 19, POSINT, map.maxTanks, 0, 1, 0, 6,
     0},
    {"maxecms", "", "Max ECMS:", 19, POSINT, map.maxECMs, 0, 2, 0, 6, 0},
    {"maxmines", "", "Max Mines:", 19, POSINT, map.maxMines, 0, 3, 0, 6,
     0},
    {"maxmissiles", "", "Max Missiles:", 19, POSINT, map.maxMissiles, 0, 4,
     0, 6, 0},
    {"maxcloaks", "", "Max Cloaks:", 19, POSINT, map.maxCloaks, 0, 5, 0, 6,
     0},
    {"maxsensors", "", "Max Sensors:", 19, POSINT, map.maxSensors, 0, 6, 0,
     6, 0},
    {"maxwideangles", "", "Max Widesangles:", 19, POSINT,
     map.maxWideangles, 0, 7, 0, 6, 0},
    {"maxrearshots", "", "Max Rearshots:", 19, POSINT, map.maxRearshots, 0,
     8, 0, 6, 0},
    {"maxafterburners", "", "Max Afterburners:", 19, POSINT,
     map.maxAfterburners, 0, 9, 0, 6, 0},
    {"maxtransporters", "", "Max Transporters:", 19, POSINT,
     map.maxTransporters, 0, 0, 1, 6, 0},
    {"maxdeflectors", "", "Max Deflectors:", 19, POSINT, map.maxDeflectors,
     0, 1, 1, 6, 0},
    {"maxphasings", "", "Max Phasings:", 19, POSINT, map.maxPhasings, 0, 2,
     1, 6, 0},
    {"maxhyperjumps", "", "Max HyperJumps:", 19, POSINT, map.maxDeflectors,
     0, 3, 1, 6, 0},
    {"maxemergencythrusts", "", "Max Emrg.Thrsts:", 19, POSINT,
     map.maxEmergencyThrusts, 0, 4, 1, 6, 0},
    {"maxlasers", "", "Max Lasers:", 19, POSINT, map.maxLasers, 0, 5, 1, 6,
     0},
    {"maxtractorbeams", "", "Max TractorBms:", 19, POSINT,
     map.maxTractorBeams, 0, 6, 1, 6, 0},
    {"maxautopilots", "", "Max AutoPilots:", 19, POSINT, map.maxAutopilots,
     0, 7, 1, 6, 0},
    {"maxemergencyshields", "", "Max Emrg.Shields:", 19, POSINT,
     map.maxEmergencyShields, 0, 8, 1, 6, 0},
    {"maxmirrors", "", "Max Mirrors:", 19, POSINT, map.maxMirrors, 0, 9, 1,
     6, 0},
    {"maxarmor", "maxarmors", "Max Armor:", 19, POSINT, map.maxArmor, 0, 0,
     2, 6, 0},
    {"maxoffensiveitems", "", "MaxOffenseItems:", 6, POSINT,
     map.maxOffensiveItems, 0, 1, 2, 6, 0},
    {"maxdefensiveitems", "", "MaxDefenseItems:", 6, POSINT,
     map.maxDefensiveItems, 0, 2, 2, 6, 0},
    {"maxitemdensity", "", "MaxItemDensity:", 19, POSFLOAT,
     map.maxItemDensity, 0, 4, 2, 6, 0},


    {"itemenergypackprob", "", "Fuel Pack Prob:", 19, POSFLOAT,
     map.itemEnergyPackProb, 0, 0, 0, 7, 0},
    {"itemtankprob", "", "Tank Prob:", 19, POSFLOAT, map.itemTankProb, 0,
     1, 0, 7, 0},
    {"itemecmprob", "", "ECM Prob:", 19, POSFLOAT, map.itemECMProb, 0, 2,
     0, 7, 0},
    {"itemmineprob", "", "Mine Prob:", 19, POSFLOAT, map.itemMineProb, 0,
     3, 0, 7, 0},
    {"itemmissileprob", "", "Missile Prob:", 19, POSFLOAT,
     map.itemMissileProb, 0, 4, 0, 7, 0},
    {"itemcloakprob", "", "Cloak Prob:", 19, POSFLOAT, map.itemCloakProb,
     0, 5, 0, 7, 0},
    {"itemsensorprob", "", "Sensor Prob:", 19, POSFLOAT,
     map.itemSensorProb, 0, 6, 0, 7, 0},
    {"itemwideangleprob", "", "Wideangle Prob:", 19, POSFLOAT,
     map.itemWideangleProb, 0, 7, 0, 7, 0},
    {"itemrearshotprob", "", "Rearshot Prob:", 19, POSFLOAT,
     map.itemRearshotProb, 0, 8, 0, 7, 0},
    {"itemafterburnerprob", "", "Afterburner Prob:", 19, POSFLOAT,
     map.itemAfterburnerProb, 0, 9, 0, 7, 0},
    {"itemtransporterprob", "", "Transporter Prob:", 19, POSFLOAT,
     map.itemTransporterProb, 0, 0, 1, 7, 0},
    {"itemlaserprob", "", "Laser Prob:", 19, POSFLOAT, map.itemLaserProb,
     0, 1, 1, 7, 0},
    {"itememergencythrustprob", "", "EmerThrst Prob:", 19, POSFLOAT,
     map.itemEmergencyThrustProb, 0, 2, 1, 7, 0},
    {"itemtractorbeamprob", "", "Tractor Prob:", 19, POSFLOAT,
     map.itemTractorBeamProb, 0, 3, 1, 7, 0},
    {"itemautopilotprob", "", "Autopilot Prob:", 19, POSFLOAT,
     map.itemAutopilotProb, 0, 4, 1, 7, 0},
    {"itememergencyshieldprob", "", "EmerShield Prob:", 19, POSFLOAT,
     map.itemEmergencyShieldProb, 0, 5, 1, 7, 0},
    {"itemdeflectorprob", "", "Deflector Prob:", 19, POSFLOAT,
     map.itemDeflectorProb, 0, 6, 1, 7, 0},
    {"itemhyperjumpprob", "", "HyperJump Prob:", 19, POSFLOAT,
     map.itemHyperJumpProb, 0, 7, 1, 7, 0},
    {"itemphasingprob", "", "Phasing Prob:", 19, POSFLOAT,
     map.itemPhasingProb, 0, 8, 1, 7, 0},
    {"itemmirrorprob", "", "Mirror Prob:", 19, POSFLOAT,
     map.itemMirrorProb, 0, 9, 1, 7, 0},
    {"itemarmorprob", "", "Armor Prob:", 19, POSFLOAT, map.itemArmorProb,
     0, 0, 2, 7, 0},
    {"movingitemprob", "", "Moving Item Prob:", 19, POSFLOAT,
     map.movingItemProb, 0, 1, 2, 7, 0},
    {"dropitemonkillprob", "", "DropItemKillProb:", 19, POSFLOAT,
     map.dropItemOnKillProb, 0, 2, 2, 7, 0},
    {"detonateitemonkillprob", "", "DestItemKillProb:", 19, POSFLOAT,
     map.detonateItemOnKillProb, 0, 3, 2, 7, 0},
    {"destroyitemincollisionprob", "", "DestItemCollProb:", 19, POSFLOAT,
     map.destroyItemInCollisionProb, 0, 4, 2, 7, 0},
    {"itemconcentratorprob", "", "Item Conc. Prob:", 19, POSFLOAT,
     map.itemConcentratorProb, 0, 5, 2, 7, 0},
    {"itemprobmult", "itemprobfact", "Item Prob Mult:", 19, POSFLOAT,
     map.itemProbMult, 0, 7, 2, 7, 0},
    {"cannonitemprobmult", "", "Can.ItemProbMlt:", 19, POSFLOAT,
     map.cannonItemProbMult, 0, 8, 2, 7, 0},
    {"randomitemprob", "", "RandomItemProb:", 19, POSFLOAT,
     map.randomItemProb, 0, 10, 2, 7, 0},
    {"asteroidprob", "", "AsteroidProb:", 19, POSFLOAT, map.asteroidProb,
     0, 12, 2, 7, 0},
    {"asteroidconcentratorprob", "", "AstrdConcProb:", 19, POSFLOAT,
     map.asteroidConcentratorProb, 0, 13, 2, 7, 0},
    {"asteroiditemprob", "", "AstrdItemProb:", 19, POSFLOAT,
     map.asteroidItemProb, 0, 14, 2, 7, 0},

    {"shotkillscoremult", "", "ShotKillScoreMlt:", 19, POSFLOAT,
     map.shotKillScoreMult, 0, 0, 0, 8, 0},
    {"torpedokillscoremult", "", "TorpKillScoreMlt:", 19, POSFLOAT,
     map.torpedoKillScoreMult, 0, 1, 0, 8, 0},
    {"smartkillscoremult", "", "SmrtKillScoroMlt:", 19, POSFLOAT,
     map.smartKillScoreMult, 0, 2, 0, 8, 0},
    {"heatkillscoremult", "", "HeatKillScoreMlt:", 19, POSFLOAT,
     map.heatKillScoreMult, 0, 3, 0, 8, 0},
    {"clusterkillscoremult", "", "ClstrKillScoreMlt:", 19, POSFLOAT,
     map.clusterKillScoreMult, 0, 4, 0, 8, 0},
    {"laserkillscoremult", "", "LasrKillScoreMlt:", 19, POSFLOAT,
     map.laserKillScoreMult, 0, 5, 0, 8, 0},
    {"tankkillscoremult", "", "TankKillScoreMlt:", 19, POSFLOAT,
     map.tankKillScoreMult, 0, 6, 0, 8, 0},
    {"runoverkillscoremult", "", "RnvrKillScoreMlt:", 19, POSFLOAT,
     map.runoverKillScoreMult, 0, 7, 0, 8, 0},
    {"ballkillscoremult", "", "BallKillScoreMlt:", 19, POSFLOAT,
     map.ballKillScoreMult, 0, 8, 0, 8, 0},
    {"explosionkillscoremult", "", "ExplKillScoreMlt:", 19, POSFLOAT,
     map.explosionKillScoreMult, 0, 9, 0, 8, 0},
    {"shovekillscoremult", "", "ShveKillScoreMlt:", 19, POSFLOAT,
     map.shoveKillScoreMult, 0, 10, 0, 8, 0},
    {"crashscoremult", "", "CrashScoreMlt:", 19, POSFLOAT,
     map.crashScoreMult, 0, 11, 0, 8, 0},
    {"minescoremult", "", "MineScoreMlt:", 19, POSFLOAT, map.mineScoreMult,
     0, 12, 0, 8, 0},
    {"tankscoredecrement", "", "Tank Decrement:", 6, INT,
     map.tankScoreDecrement, 0, 0, 1, 8, 0},
    {"asteroidpoints", "", "Asteroid Pts:", 19, FLOAT, map.asteroidPoints,
     0, 1, 1, 8, 0},
    {"asteroidmaxscore", "", "AsteroidMaxScore:", 19, FLOAT,
     map.asteroidMaxScore, 0, 2, 1, 8, 0},
    {"cannonpoints", "", "Cannon Pts:", 19, FLOAT, map.cannonPoints, 0, 3,
     1, 8, 0},
    {"cannonmaxscore", "", "CannonMaxScore:", 19, FLOAT,
     map.cannonMaxScore, 0, 4, 1, 8, 0},
    {"teamsharescore", "", "TeamShareScore?", 0, YESNO, 0,
     &map.teamShareScore, 6, 1, 8, 0},

    {"mapdata", "", NULL, 0, MAPDATA, NULL, 0, 0, 0, 0, 0}
};

/***************************************************************************/
/* int main                                                                */
/* Arguments :                                                             */
/*   argc                                                                  */
/*   argv                                                                  */
/* Purpose :                                                               */
/***************************************************************************/

int main(int argc, char *argv[])
{
    SetDefaults(argc, argv);

    Setup_default_server_options();

    T_ConnectToServer(display_name);

    LoadMap(map.mapFileName);

    SizeMapwin();

    T_SetToolkitFont(fontname);

    T_GetGC(&White_GC, "white");
    T_GetGC(&Black_GC, "black");
    T_GetGC(&xorgc, "white");
    XSetFunction(display, xorgc, GXxor);
    T_GetGC(&Wall_GC, COLOR_WALL);
    T_GetGC(&Decor_GC, COLOR_DECOR);
    T_GetGC(&Fuel_GC, COLOR_FUEL);
    T_GetGC(&Treasure_GC, COLOR_TREASURE);
    T_GetGC(&Target_GC, COLOR_TARGET);
    T_GetGC(&Item_Conc_GC, COLOR_ITEM_CONC);
    T_GetGC(&Gravity_GC, COLOR_GRAVITY);
    T_GetGC(&Current_GC, COLOR_CURRENT);
    T_GetGC(&Wormhole_GC, COLOR_WORMHOLE);
    T_GetGC(&Base_GC, COLOR_BASE);
    T_GetGC(&Cannon_GC, COLOR_CANNON);
    T_GetGC(&Friction_GC, COLOR_FRICTION);

    mapwin = T_MakeWindow(50, (int) ((root_height - mapwin_height) / 2),
			  mapwin_width, mapwin_height, "white", "black");
    T_SetWindowName(mapwin,
		    PACKAGE_NAME " XP-MapEdit",
		    PACKAGE_NAME " XP-MapEdit");
    XSelectInput(display, mapwin, ExposureMask | ButtonPressMask |
		 KeyPressMask | StructureNotifyMask | ButtonReleaseMask |
		 PointerMotionMask);
    BuildMapwinForm();

    smlmap_pixmap = XCreatePixmap(display, mapwin, TOOLSWIDTH, TOOLSWIDTH,
				  DefaultDepth(display, screennum));

    prefwin = T_PopupCreate(PREF_X, PREF_Y, PREFSEL_WIDTH, PREFSEL_HEIGHT,
			    "Preferences");
    BuildPrefsForm();

    helpwin =
	T_PopupCreate(HELP_X, HELP_Y, HELP_WIDTH, HELP_HEIGHT, "Help");

    ResetMap();

    XMapWindow(display, mapwin);

    MainEventLoop();

    return 0;
}




/* RTT allow setting of server options from the default_settings array above */
void Setup_default_server_options(void)
{
    int i;

    for (i = 0; i < num_default_settings; i++)
	AddOption(default_settings[i].name, default_settings[i].value);
    return;
}




/***************************************************************************/
/* SetDefaults                                                             */
/* Arguments :                                                             */
/*   argc                                                                  */
/*   argv                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
void SetDefaults(int argc, char *argv[])
{
    int i, j;

    /* Set "hardcoded" defaults from defaults file */
    progname = argv[0];
    drawicon = 3;
    drawmode = 1;
    geometry_width = 800;
    geometry_height = 600;

    if (map.comments)
	free(map.comments);
    map.comments = (char *) NULL;
    map.mapName[0] = map.mapFileName[0] = '\0';
    /*   strcpy(map.author,"Captain America (mbcaprt@mphhpd.ph.man.ac.uk)\0"); */
    map.width = DEFAULT_WIDTH;
    sprintf(map.width_str, "%d", map.width);
    map.height = DEFAULT_HEIGHT;
    sprintf(map.height_str, "%d", map.height);
    for (i = 0; i < MAX_MAP_SIZE; i++)
	for (j = 0; j < MAX_MAP_SIZE; j++) {
	    map.data[i][j] = ' ';
	    clipdata[i][j] = ' ';
	}
    map.view_x = map.view_y = 0;
    map.view_zoom = DEFAULT_MAP_ZOOM;
    map.changed = 0;
    ParseArgs(argc, argv);
}

/***************************************************************************/
/* ParseArgs                                                               */
/* Arguments :                                                             */
/*   argc                                                                  */
/*   argv                                                                  */
/* Purpose :                                                               */
/***************************************************************************/
void ParseArgs(int argc, char *argv[])
{
    static char *options[] = {
	"-font",
	"-display",
	"-help",
	"--help",
	"--version"
    };
    int NUMOPTIONS = 5;
    int index, option;

    for (index = 1; index < argc; index++) {
	for (option = 0; option < NUMOPTIONS; option++)
	    if (!strcmp(argv[index], options[option]))
		break;

	switch (option) {

	case 0:
	    free(fontname);
	    fontname = (char *) malloc(strlen(argv[++index]) + 1);
	    strcpy(fontname, argv[index]);
	    break;

	case 1:
	    free(display_name);
	    display_name = malloc(strlen(argv[++index]) + 1);
	    strcpy(display_name, argv[index]);
	    break;

	case 2:
	case 3:

	    fprintf(stdout,
		    "Usage :\n\n	%s [options] [mapfile]\n\n",
		    argv[0]);
	    fprintf(stdout,
		    "[mapfile] is the map you wish to edit. Leaving it blank will start a new map\n");
	    fprintf(stdout, "[options] can be\n\n");
	    fprintf(stdout, "  -font <string>\n");
	    fprintf(stdout,
		    "    Use this font instead of the default\n\n");
	    fprintf(stdout, "  -display <string>\n");
	    fprintf(stdout,
		    "    Display mapeditor on this display instead of the current\n\n");
	    fprintf(stdout, "  -help\n");
	    fprintf(stdout, "    Output this help\n\n");
	    fprintf(stdout, "  --help\n");
	    fprintf(stdout, "    Output this help\n\n");
	    fprintf(stdout, "  --version\n");
	    fprintf(stdout, "    Output version\n\n");
	    exit(0);

	case 4:
	    fprintf(stdout, "xpilot-ng-xp-mapedit " VERSION "\n");
	    exit(0);

	default:
	    if (argv[index][0] != '-')
		strcpy(map.mapFileName, argv[index]);
	    else
		fprintf(stderr, "%s illegal option, ignoring.\n",
			argv[index]);
	    break;

	}
    }
}

/***************************************************************************/
/* ResetMap                                                                */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void ResetMap(void)
{
    map.view_x = map.view_y = 0;
    while (((mapwin_width - TOOLSWIDTH) > (map.width * map.view_zoom)) ||
	   (mapwin_height > (map.height * map.view_zoom)))
	map.view_zoom++;
    T_SetWindowSizeLimits(mapwin, TOOLSWIDTH + 50, TOOLSHEIGHT,
			  TOOLSWIDTH + map.width * map.view_zoom,
			  map.height * map.view_zoom, 0, 0);
    SizeSmallMap();
    DrawTools();
    XFillRectangle(display, mapwin, Black_GC, TOOLSWIDTH, 0,
		   mapwin_width - TOOLSWIDTH, mapwin_height);
    DrawMap(TOOLSWIDTH, 0, mapwin_width - TOOLSWIDTH, mapwin_height);
    T_FormScrollArea(mapwin, "draw_map_icon", T_SCROLL_UNBOUND, TOOLSWIDTH,
		     0, mapwin_width - TOOLSWIDTH, mapwin_height,
		     DrawMapIcon);
}

/***************************************************************************/
/* SizeMapwin                                                              */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void SizeMapwin(void)
{
    /* try for geometry settings */
    mapwin_width = geometry_width;
    mapwin_height = geometry_height;
    /* increase to size of tools if it's too small */
    if (mapwin_width < (TOOLSWIDTH + map.view_zoom))
	mapwin_width = TOOLSWIDTH + map.view_zoom;
    if (mapwin_height < TOOLSHEIGHT)
	mapwin_height = TOOLSHEIGHT;
    /* if it's too big for map, zoom in */
    while (((mapwin_width - TOOLSWIDTH) > (map.width * map.view_zoom)) ||
	   (mapwin_height > (map.height * map.view_zoom)))
	map.view_zoom++;
}

/***************************************************************************/
/* SizeSmallMap                                                            */
/* Arguments :                                                             */
/* Purpose :                                                               */
/***************************************************************************/
void SizeSmallMap(void)
{
    smlmap_width = smlmap_height = TOOLSWIDTH - 20;
    smlmap_xscale = (float) (map.width) / (float) (smlmap_width);
    smlmap_yscale = (float) (map.height) / (float) (smlmap_height);
    if (map.width > map.height) {
	smlmap_height = (int) (map.height / smlmap_xscale);
	smlmap_yscale = (float) (map.height) / (float) (smlmap_height);
    } else if (map.height > map.width) {
	smlmap_width = (int) (map.width / smlmap_yscale);
	smlmap_xscale = (float) (map.width) / (float) (smlmap_width);
    }
    smlmap_x = TOOLSWIDTH / 2 - smlmap_width / 2;
    smlmap_y = TOOLSWIDTH / 2 - smlmap_height / 2;
    T_FormScrollArea(mapwin, "move_view", T_SCROLL_UNBOUND, smlmap_x,
		     smlmap_y + TOOLSHEIGHT - TOOLSWIDTH, smlmap_width,
		     smlmap_height, MoveMapView);
}
