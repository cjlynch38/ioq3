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
// bg_melee.c -- melee attack definitions, shared by the client and the server.
//
// The timings live here rather than in the server alone because the swing is
// client predicted: bg_pmove.c needs the total swing length to set weaponTime,
// and the server needs the same numbers to know when the blow lands.

#include "../qcommon/q_shared.h"
#include "bg_public.h"

// One entry per melee weapon. giTag order is not significant; the lookup is by
// weapon number.
static const meleeAttack_t bg_meleeAttacks[] = {
	{
		WP_GAUNTLET,
		120,		// windup: the blow does not land the instant you click
		140,		// active: how long the arc can connect for
		220,		// recover
		56.0f,		// range, up from Quake 3's 32: a sword is not a fist
		70.0f,		// arc, degrees of horizontal sweep
		5,			// traces fanned across that arc
		5.0f,		// trace box half size
		50,			// damage
		90,			// knockback, deliberately not equal to damage
		MOD_GAUNTLET
	}
};

static const int bg_numMeleeAttacks = ARRAY_LEN( bg_meleeAttacks );

/*
=================
BG_MeleeAttackForWeapon

Returns the attack definition for a melee weapon, or NULL if the weapon is not
a melee weapon.
=================
*/
const meleeAttack_t *BG_MeleeAttackForWeapon( int weapon ) {
	int		i;

	for ( i = 0; i < bg_numMeleeAttacks; i++ ) {
		if ( bg_meleeAttacks[i].weapon == weapon ) {
			return &bg_meleeAttacks[i];
		}
	}
	return NULL;
}

/*
=================
BG_MeleeSwingTime

Total length of a swing, which is what weaponTime gets set to.
=================
*/
int BG_MeleeSwingTime( const meleeAttack_t *atk ) {
	return atk->windup + atk->active + atk->recover;
}
