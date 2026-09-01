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
// bg_camera.c -- third person camera geometry shared by the client and the
// server, so both derive the same aim ray from the same player state.

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*
=================
BG_CameraViewSource

The point the aim ray starts from: the idealised third person camera position.

Deliberately has no smoothing and no world collision. The camera the player
actually sees lags behind the character and gets pushed around by walls, but
aiming has to be reproducible on the server from the player state alone, so
both sides derive the aim ray from this ideal position instead. The caller is
expected to keep the result out of the world with its own trace.
=================
*/
void BG_CameraViewSource( const playerState_t *ps, const vec3_t viewangles,
						  float dist, float height, float side, vec3_t source ) {
	vec3_t	forward, right;

	AngleVectors( viewangles, forward, right, NULL );

	VectorCopy( ps->origin, source );
	source[2] += ps->viewheight + height;
	VectorMA( source, side, right, source );
	VectorMA( source, -dist, forward, source );
}
