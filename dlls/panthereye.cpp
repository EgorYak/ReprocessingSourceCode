/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_BOTH 0x03
#define HC_AE_JUMPATTACK 0x04

Task_t tlPTRangeAttack1[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_FACE_IDEAL, (float)0},
};

Schedule_t slPTRangeAttack1[] =
{
	{tlPTRangeAttack1,
		ARRAYSIZE(tlPTRangeAttack1),
		bits_COND_ENEMY_OCCLUDED |
			bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"},
};

Task_t tlPTRangeAttack1Fast[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slPTRangeAttack1Fast[] =
{
	{tlPTRangeAttack1Fast,
		ARRAYSIZE(tlPTRangeAttack1Fast),
		bits_COND_ENEMY_OCCLUDED |
			bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"},
};

#define ZOMBIE_FLINCH_DELAY 2 // at most one flinch every n secs

class CPanthereye : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	int IRelationship(CBaseEntity* pTarget) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;
	void RunTask(Task_t* pTask) override;
	void EXPORT LeapTouch(CBaseEntity* pOther);
	void StartTask(Task_t* pTask) override;
	Schedule_t* GetScheduleOfType(int Type) override;

	float m_flNextJump;
	Vector m_vJumpOrigin;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void DeathSound() override;
	void AttackSound();

	bool IsRusher() override { return true; }

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pDeathSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
};

LINK_ENTITY_TO_CLASS(monster_panther, CPanthereye);

const char* CPanthereye::pIdleSounds[] =
	{
		"panthereye/pa_idle1.wav",
		"panthereye/pa_idle2.wav",
		"panthereye/pa_idle3.wav",
		"panthereye/pa_idle4.wav",
};
const char* CPanthereye::pAlertSounds[] =
	{
		"panthereye/pa_alert1.wav",
};
const char* CPanthereye::pPainSounds[] =
	{
		"panthereye/pa_pain1.wav",
};
const char* CPanthereye::pAttackSounds[] =
	{
		"panthereye/pa_attack1.wav",
		"panthereye/pa_attack2.wav",
};

const char* CPanthereye::pDeathSounds[] =
	{
		"panthereye/pa_die1.wav",
		"panthereye/pa_die2.wav",
};
const char* CPanthereye::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CPanthereye::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CPanthereye::Classify()
{
	return CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPanthereye::SetYawSpeed()
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

bool CPanthereye::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster* pEnemy = nullptr;

	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();
	}

	if (!pEnemy)
	{
		return false;
	}

	if (flDist <= 64 && flDot >= 0.7 &&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CPanthereye::CheckRangeAttack1(float flDot, float flDist)
{
	if (FBitSet(pev->flags, FL_ONGROUND) && m_flNextAttack <= gpGlobals->time && flDist <= 256 && flDot >= 0.5)
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
bool CPanthereye::CheckRangeAttack2(float flDot, float flDist)
{
	return false;
	// BUGBUG: Why is this code here?  There is no ACT_RANGE_ATTACK2 animation.  I've disabled it for now.
#if 0
	if (FBitSet(pev->flags, FL_ONGROUND) && flDist > 64 && flDist <= 256 && flDot >= 0.5)
	{
		return true;
	}
	return false;
#endif
}

bool CPanthereye::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ((bitsDamageType & DMG_ACID) != 0)
		flDamage = 0;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CPanthereye::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CPanthereye::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CPanthereye::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CPanthereye::AttackSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CPanthereye::DeathSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, pitch);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPanthereye::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.bigmommaDmgSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.bigmommaDmgSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = 18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZOMBIE_AE_ATTACK_BOTH:
	{
		// do stuff for this event.
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.bigmommaDmgBlast, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case HC_AE_JUMPATTACK:
	{
		ClearBits(pev->flags, FL_ONGROUND);

		UTIL_SetOrigin(pev, pev->origin + Vector(0, 0, 1)); // take him off ground so engine doesn't instantly reset onground
		UTIL_MakeVectors(pev->angles);

		Vector vecJumpDir;
		if (m_hEnemy != NULL)
		{
			float gravity = g_psv_gravity->value;
			if (gravity <= 1)
				gravity = 1;

			// How fast does the headcrab need to travel to reach that height given gravity?
			float height = (m_vJumpOrigin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
			if (height < 16)
				height = 16;
			float speed = sqrt(2 * gravity * height);
			float time = speed / gravity;

			// Scale the sideways velocity to get there at the right time
			vecJumpDir = (m_vJumpOrigin + m_hEnemy->pev->view_ofs - pev->origin);
			vecJumpDir = vecJumpDir * (1.0 / time);

			// Speed to offset gravity at the desired height
			vecJumpDir.z = speed;

			// Don't jump too far/fast
			float distance = vecJumpDir.Length();

			if (distance > 650)
			{
				vecJumpDir = vecJumpDir * (650.0 / distance);
			}
		}
		else
		{
			// jump hop, don't care where
			vecJumpDir = Vector(gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z) * 350;
		}

		AttackSound();

		pev->velocity = vecJumpDir;
		m_flNextAttack = gpGlobals->time + 3;
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CPanthereye::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/panthereye.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.pantherHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// RunTask
//=========================================================
void CPanthereye::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
	{
		if (m_fSequenceFinished)
		{
			TaskComplete();
			SetTouch(NULL);
			m_IdealActivity = ACT_IDLE;
		}
		break;
	}
	default:
	{
		CBaseMonster::RunTask(pTask);
	}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CPanthereye::LeapTouch(CBaseEntity* pOther)
{
	if (0 == pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		pOther->TakeDamage(pev, pev, gSkillData.bigmommaDmgSlash, DMG_SLASH);
	}

	SetTouch(NULL);
}

void CPanthereye::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		AttackSound();
		if (m_hEnemy != NULL)
			m_vJumpOrigin = m_hEnemy->pev->origin;
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CPanthereye::LeapTouch);
		break;
	}
	default:
	{
		CBaseMonster::StartTask(pTask);
	}
	}
}

Schedule_t* CPanthereye::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
	{
		return &slPTRangeAttack1[0];
	}
	break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPanthereye::Precache()
{
	PRECACHE_MODEL("models/panthereye.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CPanthereye::IgnoreConditions()
{
	return 0;
}


int CPanthereye::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_panther"))
	{
		return R_NO;
	}

	return CBaseMonster::IRelationship(pTarget);
}