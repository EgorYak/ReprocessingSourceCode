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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"
#include "UserMessages.h"

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};


//LINK_ENTITY_TO_CLASS(weapon_hornetgun, CHgun);
LINK_ENTITY_TO_CLASS(weapon_m60, CHgun);
LINK_ENTITY_TO_CLASS(weapon_nam_m60, CHgun);

bool CHgun::IsUseable()
{
	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
	{
		return true;
	}
	return false;
}

void CHgun::Spawn()
{
	Precache();
	m_iId = WEAPON_HORNETGUN;
	SET_MODEL(ENT(pev), "models/w_m60.mdl");

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit(); // get ready to fall down.
	if (g_iSkillLevel != SKILL_HARD && FClassnameIs(pev, "weapon_m60") && pev->movetype == MOVETYPE_TOSS)
		UTIL_Remove(this);
}


void CHgun::Precache()
{
	PRECACHE_MODEL("models/v_m60.mdl");
	PRECACHE_MODEL("models/w_m60.mdl");
	PRECACHE_MODEL("models/p_m60.mdl");

	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("hassault/hw_shoot2.wav");
	PRECACHE_SOUND("hassault/hw_shoot3.wav");

	m_iShell = PRECACHE_MODEL("models/762shell.mdl"); // brass shellTE_MODEL

	m_usHornetFire = PRECACHE_EVENT(1, "events/firehornet.sc");

	//UTIL_PrecacheOther("hornet");
}

void CHgun::AddToPlayer(CBasePlayer* pPlayer)
{
#ifndef CLIENT_DLL
	if (g_pGameRules->IsMultiplayer())
	{
		// in multiplayer, all hivehands come full.
		m_iDefaultAmmo = HORNET_MAX_CARRY;
	}
#endif

	if (FClassnameIs(pev, "weapon_nam_m60"))
		m_iFirePhase = 1;
	pev->classname = MAKE_STRING("weapon_m60");
	CBasePlayerWeapon::AddToPlayer(pPlayer);
}

bool CHgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HORNETGUN;
	p->iFlags = 0;
	//p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return true;
}


bool CHgun::Deploy()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 18)
		pev->body = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
	else
		pev->body = 18;
	return DefaultDeploy("models/v_m60.mdl", "models/p_m60.mdl", HGUN_UP, "hive", pev->body);
}

void CHgun::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(HGUN_DOWN);
}


void CHgun::PrimaryAttack()
{
	if (m_iFirePhase)
		Reload();
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

#ifndef CLIENT_DLL
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_762, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	m_flRechargeTime = gpGlobals->time + 0.12;
#endif

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	int recoil = (int)CVAR_GET_FLOAT("sk_adv_recoil");
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0, 1);

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (recoil)
	{
		if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
			KickBack(1.2, 0.65, 0.35, 0.125, 5.0, 3.5, 8);
		else if (m_pPlayer->pev->velocity.Length2D() > 0)
			KickBack(1.0, 0.5, 0.3, 0.06, 4.0, 3.0, 8);
		else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
			KickBack(0.35, 0.325, 0.25, 0.025, 3.5, 2.5, 9);
		else
			KickBack(0.5, 0.35, 0.3, 0.03, 3.75, 3.0, 9);
	}

	m_flNextPrimaryAttack = GetNextAttackDelay(0.12);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.12;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}



void CHgun::SecondaryAttack()
{
	return;
}


static float GetRechargeTime()
{
	if (gpGlobals->maxClients > 1)
	{
		return 0.3f;
	}
	return 0.5f;
}

void CHgun::Reload()
{
	if (!m_iFirePhase)
		return;
#ifndef CLIENT_DLL
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= HORNET_MAX_CARRY)
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < HORNET_MAX_CARRY && m_flRechargeTime < gpGlobals->time)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime += GetRechargeTime();
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 18)
		pev->body = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
	else
		pev->body = 18;

	SendWeaponAnim(HGUN_IDLE1);

	m_pPlayer->TabulateAmmo();
#endif
}


void CHgun::WeaponIdle()
{
	if (m_iFirePhase)
		Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 18)
		pev->body = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
	else
		pev->body = 18;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}
