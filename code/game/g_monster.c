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
