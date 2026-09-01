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
// cg_monsters.c -- drawing AI actors.
//
// Monsters are not backed by a client slot, so they have no entry in
// cgs.clientinfo and no player configstring. They do use the same three part
// model format, so they borrow the player model pipeline wholesale: their
// models, skins and animations are loaded into a clientInfo_t of their own,
// one per monster type rather than one per client.

#include "cg_local.h"

/*
===============
CG_MonsterInfo

Model data for a monster type, loaded on first sight.
===============
*/
clientInfo_t *CG_MonsterInfo( int type ) {
	const monsterDef_t	*def;
	clientInfo_t		*ci;

	if ( type < 0 || type >= MONSTER_NUM_TYPES ) {
		return NULL;
	}

	ci = &cgs.monsterinfo[ type ];
	if ( ci->infoValid ) {
		return ci;
	}

	def = BG_MonsterDefByType( type );
	if ( !def ) {
		return NULL;
	}

	memset( ci, 0, sizeof( *ci ) );

	// head model and skin are the same as the body: OpenArena's player models
	// keep all three parts under one directory
	if ( !CG_RegisterClientModelname( ci, def->modelPath, def->skin,
			def->modelPath, def->skin, "" ) ) {
		CG_Printf( S_COLOR_YELLOW "WARNING: could not load monster model %s/%s\n",
			def->modelPath, def->skin );
		return NULL;
	}

	ci->infoValid = qtrue;
	return ci;
}

/*
===============
CG_ClientInfoForEntity

Model data for whatever kind of actor this entity is. Lets the player
animation and angle code serve monsters unchanged.
===============
*/
clientInfo_t *CG_ClientInfoForEntity( centity_t *cent ) {
	int		clientNum;

	if ( cent->currentState.eType == ET_MONSTER ) {
		return CG_MonsterInfo( cent->currentState.modelindex );
	}

	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return NULL;
	}
	return &cgs.clientinfo[ clientNum ];
}

/*
===============
CG_Monster

Assembles legs, torso and head the same way CG_Player does, minus everything
that only makes sense for a client: powerups, team colours, the held weapon,
name plates and the first person special cases.
===============
*/
void CG_Monster( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs, torso, head;
	int				legsOld, legsFrame, torsoOld, torsoFrame;
	float			legsBackLerp, torsoBackLerp;
	vec3_t			legsAngles, torsoAngles, headAngles;
	vec3_t			legsAxis[3], torsoAxis[3], headAxis[3];
	float			shadowPlane = 0;

	ci = CG_MonsterInfo( cent->currentState.modelindex );
	if ( !ci || !ci->infoValid ) {
		return;
	}

	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );

	CG_PlayerAnimation( cent, &legsOld, &legsFrame, &legsBackLerp,
		&torsoOld, &torsoFrame, &torsoBackLerp );

	// A monster faces where it is heading. The player's separate torso and legs
	// yaw exists so you can aim one way while running another; nothing here
	// needs that yet.
	VectorClear( legsAngles );
	legsAngles[YAW] = cent->lerpAngles[YAW];
	VectorCopy( legsAngles, torsoAngles );
	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[ROLL] = 0;

	AnglesToAxis( legsAngles, legsAxis );
	AnglesToAxis( torsoAngles, torsoAxis );
	AnglesToAxis( headAngles, headAxis );

	CG_PlayerShadow( cent, &shadowPlane );

	//
	// legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;
	VectorCopy( cent->lerpOrigin, legs.origin );
	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	AxisCopy( legsAxis, legs.axis );
	legs.oldframe = legsOld;
	legs.frame = legsFrame;
	legs.backlerp = legsBackLerp;
	legs.shadowPlane = shadowPlane;
	legs.renderfx = RF_LIGHTING_ORIGIN;
	VectorCopy( legs.origin, legs.oldorigin );
	if ( !legs.hModel ) {
		return;
	}
	trap_R_AddRefEntityToScene( &legs );

	//
	// torso
	//
	torso.hModel = ci->torsoModel;
	if ( !torso.hModel ) {
		return;
	}
	torso.customSkin = ci->torsoSkin;
	VectorCopy( cent->lerpOrigin, torso.lightingOrigin );
	AxisCopy( torsoAxis, torso.axis );
	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso" );
	torso.oldframe = torsoOld;
	torso.frame = torsoFrame;
	torso.backlerp = torsoBackLerp;
	torso.shadowPlane = shadowPlane;
	torso.renderfx = RF_LIGHTING_ORIGIN;
	trap_R_AddRefEntityToScene( &torso );

	//
	// head
	//
	head.hModel = ci->headModel;
	if ( !head.hModel ) {
		return;
	}
	head.customSkin = ci->headSkin;
	VectorCopy( cent->lerpOrigin, head.lightingOrigin );
	AxisCopy( headAxis, head.axis );
	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head" );
	head.shadowPlane = shadowPlane;
	head.renderfx = RF_LIGHTING_ORIGIN;
	trap_R_AddRefEntityToScene( &head );
}
