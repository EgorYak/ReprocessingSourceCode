//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "Exports.h"

//
// Override the StudioModelRender virtual member functions here to implement custom bone
// setup, blending, etc.
//

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

// The renderer object, created on the stack.
CGameStudioModelRenderer g_StudioRenderer;
/*
====================
CGameStudioModelRenderer

====================
*/
CGameStudioModelRenderer::CGameStudioModelRenderer()
{
}

////////////////////////////////////
// Hooks to class implementation
////////////////////////////////////

/*
====================
R_StudioDrawPlayer

====================
*/
int R_StudioDrawPlayer(int flags, entity_state_t* pplayer)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawPlayer(flags, pplayer));
}

/*
====================
R_StudioDrawModel

====================
*/
int R_StudioDrawModel(int flags)
{
	return static_cast<int>(g_StudioRenderer.StudioDrawModel(flags));
}

/*
====================
R_StudioInit

====================
*/
void R_StudioInit()
{
	g_StudioRenderer.Init();
}

// The simple drawing interface we'll pass back to the engine
r_studio_interface_t studio =
	{
		STUDIO_INTERFACE_VERSION,
		R_StudioDrawModel,
		R_StudioDrawPlayer,
};

/*
====================
HUD_GetStudioModelInterface

Export this function for the engine to use the studio renderer class to render objects.
====================
*/
int DLLEXPORT HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	//	RecClStudioInterface(version, ppinterface, pstudio);

	if (version != STUDIO_INTERFACE_VERSION)
		return 0;

	// Point the engine to our callbacks
	*ppinterface = &studio;

	// Copy in engine helper functions
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));

	// Initialize local variables, etc.
	R_StudioInit();

	// Success
	return 1;
}

#define BURN_SPRITE_PATH       "sprites/flames.spr"
#define BURN_SPRITE_FRAMES     23
// Animation speed for separate sprite frames.
#define BURN_SPRITE_FPS        15.0f

// World-space size of each flame.
#define BURN_SPRITE_WIDTH      20.0f
#define BURN_SPRITE_HEIGHT     32.0f

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void CGameStudioModelRenderer::Init()
{
	CStudioModelRenderer::Init();

	m_pBurnSprite = IEngineStudio.Mod_ForName(
		BURN_SPRITE_PATH,
		0
	);

	if (m_pBurnSprite && m_pBurnSprite->type != mod_sprite)
	{
		m_pBurnSprite = NULL;
	}
}


// ---------------------------------------------------------------------------
// Find bone by name
// ---------------------------------------------------------------------------

int CGameStudioModelRenderer::StudioFindBone(const char* name)
{
	int i;
	mstudiobone_t* bones;

	if (!m_pStudioHeader || !name)
		return -1;

	bones = (mstudiobone_t*)((byte*)m_pStudioHeader +
		m_pStudioHeader->boneindex);

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		if (stricmp(bones[i].name, name) == 0)
			return i;
	}

	return -1;
}


// ---------------------------------------------------------------------------
// Draw burn sprites
// ---------------------------------------------------------------------------
#include "event_api.h"
void CGameStudioModelRenderer::StudioDrawBurnSprites()
{
	static const char* boneNames[] =
	{
		"Bip01 Head",
		"Bip01 Spine2",
		"Bip01 Pelvis",

		"Bip01 L Arm1",
		"Bip01 R Arm1",

		"Bip01 L Leg1",
		"Bip01 R Leg1"
	};

	const int boneCount =
		sizeof(boneNames) / sizeof(boneNames[0]);

	int i;
	int frame;

	float halfWidth;
	float halfHeight;

	float center[3];

	float right[3];
	float up[3];

	float v0[3];
	float v1[3];
	float v2[3];
	float v3[3];

	int bone;

	// Check current entity.
	if (!m_pCurrentEntity)
		return;

	// Check effect flag.
	if ((m_pCurrentEntity->curstate.effects & EF_SNIPERLASER) == 0)
		return;

	// Check sprite.
	if (!m_pBurnSprite)
		return;

	if (m_pBurnSprite->type != mod_sprite)
		return;

	if (m_pBurnSprite->numframes <= 0)
		return;

	// Check bone transforms.
	if (!m_plighttransform || !m_pStudioHeader)
		return;

	// Calculate animation frame.
	frame = (int)(m_clTime * BURN_SPRITE_FPS);

	frame %= m_pBurnSprite->numframes;

	if (frame < 0)
		frame = 0;

	halfWidth = BURN_SPRITE_WIDTH * 0.5f;
	halfHeight = BURN_SPRITE_HEIGHT * 0.5f;

	// Billboard orientation: camera right and up vectors.
	VectorCopy(m_vRight, right);
	VectorCopy(m_vUp, up);

	// Enable additive sprite rendering.
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);

	for (i = 0; i < boneCount; i++)
	{
		bone = StudioFindBone(boneNames[i]);

		if (bone < 0)
			continue;

		// Bone position in world coordinates.
		center[0] = (*m_plighttransform)[bone][0][3];
		center[1] = (*m_plighttransform)[bone][1][3];
		center[2] = (*m_plighttransform)[bone][2][3];

		// Top-left vertex.
		v0[0] = center[0] - right[0] * halfWidth + up[0] * halfHeight;
		v0[1] = center[1] - right[1] * halfWidth + up[1] * halfHeight;
		v0[2] = center[2] - right[2] * halfWidth + up[2] * halfHeight;

		// Top-right vertex.
		v1[0] = center[0] + right[0] * halfWidth + up[0] * halfHeight;
		v1[1] = center[1] + right[1] * halfWidth + up[1] * halfHeight;
		v1[2] = center[2] + right[2] * halfWidth + up[2] * halfHeight;

		// Bottom-right vertex.
		v2[0] = center[0] + right[0] * halfWidth - up[0] * halfHeight;
		v2[1] = center[1] + right[1] * halfWidth - up[1] * halfHeight;
		v2[2] = center[2] + right[2] * halfWidth - up[2] * halfHeight;

		// Bottom-left vertex.
		v3[0] = center[0] - right[0] * halfWidth - up[0] * halfHeight;
		v3[1] = center[1] - right[1] * halfWidth - up[1] * halfHeight;
		v3[2] = center[2] - right[2] * halfWidth - up[2] * halfHeight;

		// Select sprite texture/frame.
		gEngfuncs.pTriAPI->SpriteTexture(m_pBurnSprite, frame);

		// Draw billboard quad.
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3f(v0[0], v0[1], v0[2]);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3f(v1[0], v1[1], v1[2]);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3f(v2[0], v2[1], v2[2]);

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3f(v3[0], v3[1], v3[2]);

		gEngfuncs.pTriAPI->End();
	}

	// Restore the default render mode.
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	// Dynamic lighting
	float t = m_clTime * 3.0f;

	int r = 255;
	int g = 100 + (int)(70.0f * (0.5f + 0.5f * sin(t)));
	int b = 10 + (int)(15.0f * (0.5f + 0.5f * sin(t * 1.7f)));

	bone = StudioFindBone(boneNames[2]);
	center[0] = (*m_plighttransform)[bone][0][3];
	center[1] = (*m_plighttransform)[bone][1][3];
	center[2] = (*m_plighttransform)[bone][2][3];
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(m_pCurrentEntity->index);
	dl->origin = center;
	dl->die = gEngfuncs.GetClientTime() + 0.05f;
	dl->color.r = r;
	dl->color.g = g;
	dl->color.b = b;
	dl->radius = 128;
}


// ---------------------------------------------------------------------------
// StudioRenderModel
// ---------------------------------------------------------------------------

void CGameStudioModelRenderer::StudioRenderModel()
{
	// Draw the original studio model first.
	CStudioModelRenderer::StudioRenderModel();

	// Draw additional burn sprites over the model.
	StudioDrawBurnSprites();
}