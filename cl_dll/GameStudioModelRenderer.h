//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

/*
====================
CGameStudioModelRenderer

====================
*/
class CGameStudioModelRenderer : public CStudioModelRenderer
{
public:
	CGameStudioModelRenderer();

	//virtual void Init();
	virtual void StudioRenderModel();

protected:
	int StudioFindBone(const char* name);
	void StudioDrawBurnSprites();
};
