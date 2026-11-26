//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"

#include "r_studioint.h"

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

struct model_s	*m_pShells[4];

extern Vector v_sim_vel, v_angles;


void Game_HookEvents();

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo(float *Angles, int IsLocal, float *ShellVelocity, float *ShellOrientation)
{
	int i;
	float fR, fU;
	Vector vecViewAngles, vecVel, vecForward, vecRight, vecUp;

	// HACK:: Only add velocity for the local player
	vecVel = IsLocal ? v_sim_vel : vec3_origin;

	vecViewAngles = IsLocal ? v_angles : Angles;

	AngleVectors(vecViewAngles, vecForward, vecRight, vecUp);

	fR = gEngfuncs.pfnRandomFloat(50, 70);
	fU = gEngfuncs.pfnRandomFloat(100, 150);

	for (i = 0; i < 3; i++)
	{
		ShellVelocity[i] = vecVel[i] + vecRight[i] * fR + vecUp[i] * fU + vecForward[i] * 25;
	}
}

//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass(Vector vecOrigin, const Vector &angles, const Vector &gunAngles, int type, int IsLocal)
{
	if (!m_pShells[0])
	{
		m_pShells[0] = IEngineStudio.Mod_ForName("models/shell.mdl", true);

		m_pShells[1] = IEngineStudio.Mod_ForName("models/shotgunshell.mdl", true);

		m_pShells[2] = IEngineStudio.Mod_ForName("models/762shell.mdl", true);
		m_pShells[3] = IEngineStudio.Mod_ForName("models/556shell.mdl", true);
	}

	const model_s *pModel = m_pShells[type];

	if (pModel == NULL)
		return;

	TEMPENTITY	*pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc(vecOrigin, (model_s *)pModel);

	if (pTemp == NULL)
		return;

	//Keep track of shell type
	if (type == 1)
	{
		pTemp->hitSound = BOUNCE_SHOTSHELL;
	}
	else
	{
		pTemp->hitSound = BOUNCE_SHELL;
	}

	pTemp->entity.curstate.body = 0;

	pTemp->flags |= (FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE);

	pTemp->entity.baseline.angles[0] = gEngfuncs.pfnRandomFloat(-256, 255);
	pTemp->entity.baseline.angles[1] = gEngfuncs.pfnRandomFloat(-256, 255);
	pTemp->entity.baseline.angles[2] = gEngfuncs.pfnRandomFloat(-256, 255);

	pTemp->entity.curstate.rendermode = kRenderNormal;
	pTemp->entity.baseline.renderamt = 255;		// Set this for fadeout

	Vector	vecShellVelocity, vecShellOrientation;

	EV_GetDefaultShellInfo(Vector(angles.x, angles.y, angles.z), IsLocal, vecShellVelocity, vecShellOrientation);

	// Face forward
	pTemp->entity.angles = gunAngles;

	pTemp->entity.baseline.origin = vecShellVelocity;

	pTemp->die = gEngfuncs.GetClientTime() + 2.5;
}


/*
===================
EV_HookEvents

See if game specific code wants to hook any events.
===================
*/
void EV_HookEvents()
{
	Game_HookEvents();
}
