/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// psay.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_SAY_SUIT 0x0002

class CPlayerSay : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];


private:
	int m_iszSentence;	// string index for idle animation
};

TYPEDESCRIPTION CPlayerSay::m_SaveData[] =
{
	DEFINE_FIELD(CPlayerSay, m_iszSentence, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CPlayerSay, CBaseToggle);

LINK_ENTITY_TO_CLASS(scripted_player_say, CPlayerSay);

bool CPlayerSay::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sentence"))
	{
		m_iszSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

//
// ambient_generic - general-purpose user-defined static sound
//
void CPlayerSay::Spawn()
{
	Precache();
}


void CPlayerSay::Precache()
{
	char* szSoundFile = (char*)STRING(pev->message);

	if (!FStringNull(pev->message) && strlen(szSoundFile) > 1)
	{
		if (*szSoundFile != '!')
			PRECACHE_SOUND(szSoundFile);
	}
}

void CPlayerSay::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	char* szSoundFile = (char*)STRING(pev->message);

	//UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 1.0, ATTN_NONE, 0, PITCH_NORM);
	CBaseEntity* pPlayer = NULL;
	//pPlayer = UTIL_FindEntityGeneric(, pev->origin, 4096);
	pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer)
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, szSoundFile, VOL_NORM, ATTN_NORM);
	UTIL_ShowMessagePVS(STRING(m_iszSentence), pPlayer->pev->origin);
	/*
		if ((pev->spawnflags & SF_SAY_SUIT) == 0)
			EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, szSoundFile, VOL_NORM, ATTN_NORM);
		else
			EMIT_SOUND_SUIT(ENT(pPlayer->pev), szSoundFile);
	*/

	SUB_UseTargets(NULL, USE_TOGGLE, 0);
}