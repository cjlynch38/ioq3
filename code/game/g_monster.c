/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// g_monster.c -- AI actors.
//
// Monsters are ordinary gentities rather than bot clients. A bot would burn one
// of the 64 client slots and drag in Quake 3's item-and-weapon-shaped deathmatch
// AI, which actively fights a melee game: BotAggression is a hardcoded gun
// ladder that returns zero for a melee-only loadout, so bots armed with a sword
// simply run away.

#include "g_local.h"

static monsterAI_t	g_monsterAI[MAX_MONSTERS];

/*
===============
G_MonsterAllocAI
===============
*/
static monsterAI_t *G_MonsterAllocAI( void ) {
	int		i;

	for ( i = 0; i < MAX_MONSTERS; i++ ) {
		if ( !g_monsterAI[i].inuse ) {
			memset( &g_monsterAI[i], 0, sizeof( g_monsterAI[i] ) );
			g_monsterAI[i].inuse = qtrue;
			return &g_monsterAI[i];
		}
	}
	return NULL;
}

/*
===============
G_MonsterClearAI

Called from the level init so a map restart does not inherit stale slots.
===============
*/
void G_MonsterClearAI( void ) {
	memset( g_monsterAI, 0, sizeof( g_monsterAI ) );
}

/*
===============
G_MonsterFindEnemy

Nearest live player within range and in line of sight. Deliberately simple:
one player, no factions, no hearing.
===============
*/
static void G_MonsterFindEnemy( gentity_t *ent ) {
	monsterAI_t	*ai = ent->ai;
	gentity_t	*best = NULL;
	float		bestDist = 0;
	int			i;

	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t	*player = &g_entities[i];
		vec3_t		delta;
		float		dist;
		trace_t		tr;

		if ( !player->inuse || !player->client ) {
			continue;
		}
		if ( player->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( player->health <= 0 || player->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( player->flags & FL_NOTARGET ) {
			continue;
		}

		VectorSubtract( player->r.currentOrigin, ai->ps.origin, delta );
		dist = VectorLength( delta );
		if ( dist > g_monsterSightRange.value ) {
			continue;
		}

		trap_Trace( &tr, ai->ps.origin, NULL, NULL, player->r.currentOrigin,
			ent->s.number, MASK_SHOT );
		if ( tr.fraction < 1.0f && tr.entityNum != player->s.number ) {
			continue;		// something solid in the way
		}

		if ( !best || dist < bestDist ) {
			best = player;
			bestDist = dist;
		}
	}

	if ( best ) {
		ai->enemyNum = best->s.number;
		ai->lastEnemySeen = level.time;
	} else if ( level.time - ai->lastEnemySeen > 5000 ) {
		ai->enemyNum = ENTITYNUM_NONE;		// lost them, give up
	}
}

/*
===============
G_MonsterSteer

Turns the synthesised usercmd into "walk towards the enemy". Movement is
relative to the view angles, so facing the target and pushing forward is all
that a straight line chase needs.
===============
*/
static void G_MonsterSteer( gentity_t *ent, const monsterDef_t *def ) {
	monsterAI_t	*ai = ent->ai;
	gentity_t	*enemy;
	vec3_t		delta, angles;
	float		dist;
	int			i;

	ai->cmd.forwardmove = 0;
	ai->cmd.rightmove = 0;
	ai->cmd.upmove = 0;

	if ( ai->enemyNum == ENTITYNUM_NONE ) {
		ai->state = MSTATE_IDLE;
	} else {
		enemy = &g_entities[ ai->enemyNum ];

		VectorSubtract( enemy->r.currentOrigin, ai->ps.origin, delta );
		delta[2] = 0;			// steer in the horizontal plane only
		dist = VectorLength( delta );

		if ( dist > 1 ) {
			vectoangles( delta, angles );
			ai->ps.viewangles[YAW] = angles[YAW];
			ai->ps.viewangles[PITCH] = 0;
			ai->ps.viewangles[ROLL] = 0;
		}

		if ( dist > g_monsterStandoff.value ) {
			ai->state = MSTATE_CHASE;
			ai->cmd.forwardmove = 127;
			ai->ps.speed = def->runSpeed;
		} else {
			ai->state = MSTATE_ATTACK;		// in range: hold position
			ai->ps.speed = def->walkSpeed;
		}
	}

	// Pmove derives the view angles from cmd.angles plus delta_angles, so the
	// facing has to be written back through the command the same way a client's
	// mouse input would arrive.
	for ( i = 0; i < 3; i++ ) {
		ai->cmd.angles[i] = ANGLE2SHORT( ai->ps.viewangles[i] ) - ai->ps.delta_angles[i];
	}
}

/*
===============
G_RunMonster

One server frame of a monster: decide, then move with the real player physics.

Using Pmove rather than hand rolled movement means monsters inherit stair
stepping, gravity, slide-move, water, movers and jump pads for free, and it
fills in legsAnim and torsoAnim exactly as it does for a player - which is why
the client can draw them with the player animation code.
===============
*/
void G_RunMonster( gentity_t *ent ) {
	const monsterDef_t	*def;
	monsterAI_t			*ai;
	pmove_t				pm;

	ai = ent->ai;
	def = BG_MonsterDefByType( ent->monsterType );
	if ( !ai || !def ) {
		G_RunThink( ent );
		return;
	}

	G_MonsterFindEnemy( ent );
	G_MonsterSteer( ent, def );

	ai->cmd.serverTime = level.time;
	ai->ps.gravity = g_gravity.value;

	memset( &pm, 0, sizeof( pm ) );
	pm.ps = &ai->ps;
	pm.cmd = ai->cmd;
	pm.tracemask = MASK_PLAYERSOLID;
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;

	// Same movement feel the player gets. If these disagree with g_active.c the
	// monsters move differently from the player, which looks wrong long before
	// anyone works out why.
	pm.pm_accelerate = pm_accel.value;
	pm.pm_airaccelerate = pm_airaccel.value;
	pm.pm_friction = pm_frict.value;
	pm.pm_stopspeed = pm_stopspd.value;
	pm.pm_jumpvelocity = pm_jumpvel.value;

	Pmove( &pm );

	// Publish the results. BG_PlayerStateToEntityState is not used here: it
	// overwrites eType, number and clientNum, which would turn the monster into
	// a player entity belonging to client 0.
	VectorCopy( ai->ps.origin, ent->r.currentOrigin );
	VectorCopy( ai->ps.origin, ent->s.pos.trBase );
	VectorCopy( ai->ps.origin, ent->s.origin );
	ent->s.pos.trTime = level.time;

	VectorSet( ent->s.angles, 0, ai->ps.viewangles[YAW], 0 );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	VectorCopy( ent->s.angles, ent->r.currentAngles );
	ent->s.apos.trTime = level.time;

	if ( g_debugMonster.integer && ( level.time % 500 ) < 50 ) {
		G_Printf( "monster %i state %i enemy %i pos %.0f %.0f %.0f speed %.0f\n",
			ent->s.number, ai->state, ai->enemyNum,
			ai->ps.origin[0], ai->ps.origin[1], ai->ps.origin[2],
			VectorLength( ai->ps.velocity ) );
	}

	ent->s.legsAnim = ai->ps.legsAnim;
	ent->s.torsoAnim = ai->ps.torsoAnim;

	trap_LinkEntity( ent );

	G_RunThink( ent );
}

/*
===============
G_MonsterThink

Placeholder brain. Monsters stand their ground for now; movement comes next.
===============
*/
void G_MonsterThink( gentity_t *ent ) {
	ent->nextthink = level.time + 100;
}

/*
===============
G_MonsterDie

G_Damage calls targ->die unconditionally once health drops to zero, so every
damageable entity must have one or the server takes a null pointer.
===============
*/
static void G_MonsterDie( gentity_t *self, gentity_t *inflictor,
						  gentity_t *attacker, int damage, int mod ) {
	if ( self->ai ) {
		self->ai->inuse = qfalse;
		self->ai = NULL;
	}
	self->takedamage = qfalse;
	self->r.contents = 0;
	trap_LinkEntity( self );

	// No death animation yet: BOTH_DEATH1 needs the corpse to stay around and
	// stop being solid, which wants a proper corpse entity. Vanish for now.
	G_FreeEntity( self );
}

/*
===============
G_MonsterPain
===============
*/
static void G_MonsterPain( gentity_t *self, gentity_t *attacker, int damage ) {
	// Wakes the monster even if it was hit from behind and never saw it coming.
	self->enemy = attacker;
}

/*
===============
G_SpawnMonster

Creates a monster at a position. Returns NULL if the type is unknown.
===============
*/
gentity_t *G_SpawnMonster( const char *name, vec3_t origin, float yaw ) {
	const monsterDef_t	*def;
	gentity_t			*ent;
	vec3_t				angles;

	def = BG_MonsterDefByName( name );
	if ( !def ) {
		return NULL;
	}

	ent = G_Spawn();
	ent->classname = "monster";
	ent->s.eType = ET_MONSTER;

	// The type travels in modelindex. It is unused for anything that is not an
	// item, and it is what the client keys its model lookup off.
	ent->s.modelindex = def->type;

	ent->monsterType = def->type;
	ent->health = def->health;
	ent->takedamage = qtrue;
	ent->die = G_MonsterDie;
	ent->pain = G_MonsterPain;

	// Player sized on purpose. PM_CheckDuck hardcodes the player hull, so a
	// differently shaped monster needs that generalised first.
	VectorSet( ent->r.mins, -15, -15, -24 );
	VectorSet( ent->r.maxs, 15, 15, 32 );
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;

	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->r.currentOrigin );
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_INTERPOLATE;

	VectorSet( angles, 0, yaw, 0 );
	VectorCopy( angles, ent->s.angles );
	VectorCopy( angles, ent->r.currentAngles );
	ent->s.apos.trType = TR_INTERPOLATE;
	VectorCopy( angles, ent->s.apos.trBase );

	// Idle animations. These are the same enums the player models use, which is
	// the whole reason a monster can borrow the player rendering path.
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;

	ent->ai = G_MonsterAllocAI();
	if ( !ent->ai ) {
		G_Printf( "G_SpawnMonster: no free monster slots\n" );
		G_FreeEntity( ent );
		return NULL;
	}

	// The playerState the AI drives. clientNum must be the entity number:
	// Pmove passes it to the trace as the entity to skip, so a monster whose
	// clientNum is left at zero collides with client 0 instead of itself.
	ent->ai->ps.clientNum = ent->s.number;
	ent->ai->ps.pm_type = PM_NORMAL;
	ent->ai->ps.viewheight = DEFAULT_VIEWHEIGHT;
	ent->ai->ps.speed = def->runSpeed;
	ent->ai->ps.gravity = g_gravity.value;
	ent->ai->enemyNum = ENTITYNUM_NONE;
	ent->ai->state = MSTATE_IDLE;
	VectorCopy( origin, ent->ai->ps.origin );
	VectorCopy( angles, ent->ai->ps.viewangles );

	ent->think = G_MonsterThink;
	ent->nextthink = level.time + 100;

	trap_LinkEntity( ent );
	return ent;
}

/*
===============
Svcmd_Monster_f

Console: monster <name> [distance] - drops one in front of the first player.
===============
*/
void Svcmd_Monster_f( void ) {
	char		name[MAX_TOKEN_CHARS];
	char		distStr[MAX_TOKEN_CHARS];
	gentity_t	*player, *monster;
	vec3_t		forward, spot;
	float		dist;

	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		G_Printf( "usage: monster <name> [distance]\n" );
		return;
	}

	trap_Argv( 2, distStr, sizeof( distStr ) );
	dist = distStr[0] ? atof( distStr ) : 160.0f;

	player = &g_entities[0];
	if ( !player->inuse || !player->client ) {
		G_Printf( "no player to spawn in front of\n" );
		return;
	}

	AngleVectors( player->client->ps.viewangles, forward, NULL, NULL );
	forward[2] = 0;
	VectorNormalize( forward );
	VectorMA( player->client->ps.origin, dist, forward, spot );

	monster = G_SpawnMonster( name, spot, player->client->ps.viewangles[YAW] + 180 );
	if ( !monster ) {
		G_Printf( "unknown monster '%s'\n", name );
		return;
	}
	G_Printf( "spawned %s at %.0f %.0f %.0f\n", name, spot[0], spot[1], spot[2] );
}
