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
// bg_monsters.c -- monster definitions shared by the client and the server.
//
// The server needs these to spawn and drive a monster; the client needs the
// model and skin names to draw one. Keeping the table in one place means the
// two can never disagree about what a "gargoyle" is.

#include "../qcommon/q_shared.h"
#include "bg_public.h"

// Run speeds are all below the player's g_speed on purpose: a monster that
// matches the player exactly can never be shaken off, which removes retreating
// as an option entirely. The gargoyle is the slow bruiser, the skeleton the
// fast one, and both can be outrun in a straight line.
// modelPath/skin point at OpenArena's existing player models, which are GPL and
// already dark-fantasy: a winged stone gargoyle and a skeletal frame. Custom
// art replaces these later without touching any code.
static const monsterDef_t bg_monsterDefs[] = {
	{
		MONSTER_GARGOYLE,
		"gargoyle",
		"gargoyle",		// models/players/gargoyle
		"stone",		// its stone skin variant
		100,			// health
		175.0f,			// run speed
		110.0f,			// walk speed
		WP_GAUNTLET		// what it attacks with
	},
	{
		MONSTER_SKELETON,
		"skeleton",
		"skelebot",
		"default",
		60,
		200.0f,			// the fast one, but still losable
		125.0f,
		WP_GAUNTLET
	}
};

static const int bg_numMonsterDefs = ARRAY_LEN( bg_monsterDefs );

/*
=================
BG_MonsterDefByType
=================
*/
const monsterDef_t *BG_MonsterDefByType( int type ) {
	int		i;

	for ( i = 0; i < bg_numMonsterDefs; i++ ) {
		if ( bg_monsterDefs[i].type == type ) {
			return &bg_monsterDefs[i];
		}
	}
	return NULL;
}

/*
=================
BG_MonsterDefByName

Used by the map entity spawner and the console spawn command.
=================
*/
const monsterDef_t *BG_MonsterDefByName( const char *name ) {
	int		i;

	for ( i = 0; i < bg_numMonsterDefs; i++ ) {
		if ( !Q_stricmp( bg_monsterDefs[i].name, name ) ) {
			return &bg_monsterDefs[i];
		}
	}
	return NULL;
}
