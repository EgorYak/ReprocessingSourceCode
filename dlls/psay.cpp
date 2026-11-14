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
#define SF_FORCE_SUBT 0x0004

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
	if (pPlayer && pPlayer->IsAlive())
	{
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, szSoundFile, VOL_NORM, ATTN_NORM);
		UTIL_ShowMessagePVS(STRING(m_iszSentence), pPlayer->pev->origin);
	}
	/*
		if ((pev->spawnflags & SF_SAY_SUIT) == 0)
			EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, szSoundFile, VOL_NORM, ATTN_NORM);
		else
			EMIT_SOUND_SUIT(ENT(pPlayer->pev), szSoundFile);
	*/

	SUB_UseTargets(NULL, USE_TOGGLE, 0);
}

class CMonSay : public CBaseToggle
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
	int m_iszEntity; // entity that is wanted for this script
	int m_iszSentence;	// string index for idle animation
};

TYPEDESCRIPTION CMonSay::m_SaveData[] =
{
	DEFINE_FIELD(CMonSay, m_iszSentence, FIELD_STRING),
	DEFINE_FIELD(CMonSay, m_iszEntity, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CMonSay, CBaseToggle);

LINK_ENTITY_TO_CLASS(scripted_monster_say, CMonSay);

bool CMonSay::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sentence"))
	{
		m_iszSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEntity"))
	{
		m_iszEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

//
// ambient_generic - general-purpose user-defined static sound
//
void CMonSay::Spawn()
{
	Precache();
}


void CMonSay::Precache()
{
	char* szSoundFile = (char*)STRING(pev->message);

	if (!FStringNull(pev->message) && strlen(szSoundFile) > 1)
	{
		if (*szSoundFile != '!')
			PRECACHE_SOUND(szSoundFile);
	}
}

void CMonSay::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	char* szSoundFile = (char*)STRING(pev->message);

	edict_t* pentTarget;
	CBaseMonster* pMonster;


	pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszEntity));
	pMonster = NULL;

	while (!FNullEnt(pentTarget))
	{
		pMonster = GetMonsterPointer(pentTarget);
		if (pMonster != NULL)
		{
			EMIT_SOUND(ENT(pMonster->pev), CHAN_VOICE, szSoundFile, VOL_NORM, ATTN_NORM);
			if ((pev->spawnflags & SF_FORCE_SUBT) == 0)
				UTIL_ShowMessagePVS(STRING(m_iszSentence), pMonster->pev->origin);
			else
			{
				CBaseEntity* pPlayer = NULL;
				pPlayer = UTIL_GetLocalPlayer();
				if (pPlayer && pPlayer->IsAlive())
				{
					UTIL_ShowMessagePVS(STRING(m_iszSentence), pPlayer->pev->origin);
				}
			}
		}
		pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(m_iszEntity));
	}

	SUB_UseTargets(NULL, USE_TOGGLE, 0);
}