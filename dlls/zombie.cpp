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
#include "weapons.h"
#include "explode.h"
#include "decals.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_BOTH 0x03

#define ZOMBIE_FLINCH_DELAY 2 // at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;

	float m_flNextFlinch;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();
	void DeathSound() override;

	bool IsRusher() override { return true; }

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType); //Это и будет рисовать кровь

#ifdef GATESEQ
	// ku2zoff
	CUSTOM_SCHEDULES;
	Schedule_t *GetScheduleOfType(int Type);
	bool FHaveGait(void) { return true; }
	void MaintainGait(void);
#endif // GATESEQ
};

LINK_ENTITY_TO_CLASS(monster_zombie, CZombie);

const char* CZombie::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CZombie::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* CZombie::pAttackSounds[] =
	{
		"zombie/zo_attack1.wav",
		"zombie/zo_attack2.wav",
};

const char* CZombie::pIdleSounds[] =
	{
		"zombie/zo_idle1.wav",
		"zombie/zo_idle2.wav",
		"zombie/zo_idle3.wav",
		"zombie/zo_idle4.wav",
};

const char* CZombie::pAlertSounds[] =
	{
		"zombie/zo_alert10.wav",
		"zombie/zo_alert20.wav",
		"zombie/zo_alert30.wav",
};

const char* CZombie::pPainSounds[] =
	{
		"zombie/zo_pain1.wav",
		"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CZombie::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed()
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

bool CZombie::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CZombie::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CZombie::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CZombie::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CZombie::AttackSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, pitch);
}

//=========================================================
// DeathSound
//=========================================================
void CZombie::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "zombie/zo_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "zombie/zo_die2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "zombie/zo_die3.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
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

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CZombie::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{

	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:											 //Грудь
	case HITGROUP_STOMACH:											 //Живот
	case HITGROUP_LEFTLEG:											 //Левая нога
	case HITGROUP_RIGHTLEG:											 //Правая нога
	case HITGROUP_LEFTARM:											 //Левая рука
	case HITGROUP_RIGHTARM:											 //Правая рука
	case HITGROUP_GENERIC:											 //Прочие хитгруппы
		SpawnBlood(ptr->vecEndPos, BLOOD_COLOR_RED, flDamage * 5.0); // Спавнится красная кровь
		break;
	case HITGROUP_HEAD:													//Голова(Хедкраб)
		SpawnBlood(ptr->vecEndPos, BLOOD_COLOR_YELLOW, flDamage * 5.0); // Спавнится жёлтая кровь
		break;
	}

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// Spawn
//=========================================================
void CZombie::Spawn()
{
	Precache();
		
	SET_MODEL(ENT(pev), "models/zombie.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	PRECACHE_MODEL("models/zombie.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND("zombie/zo_die1.wav");
	PRECACHE_SOUND("zombie/zo_die2.wav");
	PRECACHE_SOUND("zombie/zo_die3.wav");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

#ifdef GATESEQ
// ku2zoff
Task_t	tlZombieMeleeAttack1[] =
{
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_MELEE_ATTACK1, (float)0 },
};

Schedule_t	slZombieMeleeAttack1[] =
{
	{
		tlZombieMeleeAttack1,
		ARRAYSIZE(tlZombieMeleeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Zombie Melee Attack 1"
	},
};

DEFINE_CUSTOM_SCHEDULES(CZombie)
{
	slZombieMeleeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CZombie, CBaseMonster);

Schedule_t* CZombie::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_MELEE_ATTACK1:
		return slZombieMeleeAttack1;
	default:
		return CBaseMonster::GetScheduleOfType(Type);
	}
}

// ku2zoff
void CZombie::MaintainGait(void)
{
	if (m_hEnemy != NULL && (m_hEnemy->pev->origin - pev->origin).Length() <= 128)
	{
		if (m_IdealGait != m_IdealActivity)
			RouteClear();
	}

	CBaseMonster::MaintainGait();
}
#endif // GATESEQ

int CZombie::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

///////////////////////////////////////////////////
/// Zombie Barney
//////////////////////////////////////////////////

class CZombieBarney : public CZombie
{
public:
	void Spawn() override;
	void Precache() override;
};

LINK_ENTITY_TO_CLASS (monster_zombie_barney, CZombieBarney);

//=========================================================
// Spawn
//=========================================================
void CZombieBarney::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zombie_barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombieBarney::Precache()
{
	PRECACHE_MODEL("models/zombie_barney.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

///////////////////////////////////////////////////
/// Zombie Soldier
//////////////////////////////////////////////////

class CZombieSoldier : public CZombie
{
public:
	void Spawn() override;
	void Precache() override;
};

LINK_ENTITY_TO_CLASS(monster_zombie_soldier, CZombieSoldier);

//=========================================================
// Spawn
//=========================================================
void CZombieSoldier::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zombie_soldier.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombieSoldier::Precache()
{
	PRECACHE_MODEL("models/zombie_soldier.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}


///////////////////////////////////////////////////
/// Zombie Grunt
//////////////////////////////////////////////////

class CZombieGrunt : public CZombie
{
public:
	void Spawn() override;
	void Precache() override;
};

LINK_ENTITY_TO_CLASS(monster_zombie_grunt, CZombieGrunt);

//=========================================================
// Spawn
//=========================================================
void CZombieGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zombie_grunt.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombieGrunt::Precache()
{
	PRECACHE_MODEL("models/zombie_grunt.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

///////////////////////////////////////////////////
/// Zombie ZSLAVE
//////////////////////////////////////////////////

#include "soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ZSLAVE_AE_CLAW (1)
#define ZSLAVE_AE_CLAWRAKE (2)
#define ZSLAVE_AE_ZAP_POWERUP (3)
#define ZSLAVE_AE_ZAP_SHOOT (4)
#define ZSLAVE_AE_ZAP_DONE (5)

#define ZSLAVE_MAX_BEAMS 16

class CZSlave : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	void DeathSound() override;
	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;

	void Killed(entvars_t* pevAttacker, int iGib) override;
	void Blow(void);

	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void PrescheduleThink(void);
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	CUSTOM_SCHEDULES;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams();
	void ArmBeam(int side);
	void ZapBeam(int side);
	void BeamGlow();

	CBeam* m_pBeam[ZSLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int m_voicePitch;

	int m_iBlow;

	bool IsRusher() override { return true; }

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pPainSounds[];
	static const char* pDeathSounds[];
};
LINK_ENTITY_TO_CLASS(monster_zombie_slave, CZSlave);
LINK_ENTITY_TO_CLASS(monster_zombiegaunt, CZSlave);


TYPEDESCRIPTION CZSlave::m_SaveData[] =
{
	DEFINE_ARRAY(CZSlave, m_pBeam, FIELD_CLASSPTR, ZSLAVE_MAX_BEAMS),
	DEFINE_FIELD(CZSlave, m_iBlow, FIELD_INTEGER),
	DEFINE_FIELD(CZSlave, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CZSlave, m_flNextAttack, FIELD_TIME),

	DEFINE_FIELD(CZSlave, m_voicePitch, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CZSlave, CBaseMonster);




const char* CZSlave::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CZSlave::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CZSlave::pPainSounds[] =
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char* CZSlave::pDeathSounds[] =
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CZSlave::Classify()
{
	return CLASS_ALIEN_MONSTER;
}


//=========================================================
// ALertSound - scream
//=========================================================
void CZSlave::AlertSound()
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CZSlave::IdleSound()
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
	}

	int side = RANDOM_LONG(0, 1) * 2 - 1;

	ClearBeams();
	ArmBeam(side);

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecSrc.x);	// X
	WRITE_COORD(vecSrc.y);	// Y
	WRITE_COORD(vecSrc.z);	// Z
	WRITE_BYTE(8);		// radius * 0.1
	WRITE_BYTE(255);		// r
	WRITE_BYTE(180);		// g
	WRITE_BYTE(96);		// b
	WRITE_BYTE(10);		// time * 10
	WRITE_BYTE(0);		// decay * 0.1
	MESSAGE_END();

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap1.wav", 0.3, ATTN_IDLE, 0, 100);
}

//=========================================================
// PainSound
//=========================================================
void CZSlave::PainSound()
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// DieSound
//=========================================================

void CZSlave::DeathSound()
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CZSlave::ISoundMask()
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}


void CZSlave::Killed(entvars_t* pevAttacker, int iGib)
{
	ClearBeams();
	CBaseMonster::Killed(pevAttacker, iGib);
}

void CZSlave::Blow(void)
{
	TraceResult tr;
	float flDist = 1.0;

	int side = RANDOM_LONG(0, 1) * 2 - 1;

	if (m_iBeams >= ZSLAVE_MAX_BEAMS)
		return;
	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 5) + gpGlobals->v_up * RANDOM_FLOAT(-5, 5);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	// m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetColor(96, 128, 16);
	m_pBeam[m_iBeams]->SetBrightness(64);
	m_pBeam[m_iBeams]->SetNoise(140);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	vecSrc = pev->origin + gpGlobals->v_forward * 32 + side * gpGlobals->v_right * 16 + gpGlobals->v_up * 36;

	float closest = 1;

	//Do 3 ray traces and use the closest one to make a beam
	for (auto ray = 0; ray < 3; ++ray)
	{
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + (side * gpGlobals->v_right * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1)) * 512, dont_ignore_monsters, edict(), &tr1);

		if (tr1.flFraction < closest)
		{
			tr = tr1;
			closest = tr1.flFraction;
		}
	}

	//No nearby objects found
	if (closest == 1)
	{
		return;
	}

	auto pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 30);

	if (!pBeam)
		return;

	pBeam->PointEntInit(tr.vecEndPos, entindex());
	pBeam->SetEndAttachment(3);
	pBeam->SetColor(96, 128, 16);
	pBeam->SetBrightness(64);
	pBeam->SetNoise(192);

	pBeam->SetThink(&CBaseEntity::SUB_Remove);
	pBeam->pev->nextthink = gpGlobals->time + 0.6;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZSlave::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_WALK:
		ys = 50;
		break;
	case ACT_RUN:
		ys = 70;
		break;
	case ACT_IDLE:
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CZSlave::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch (pEvent->event)
	{
	case ZSLAVE_AE_CLAW:
	{
		// SOUND HERE!
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zslaveDmgClaw, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
		}
		ClearBeams();
		ClearMultiDamage();

		UTIL_MakeAimVectors(pev->angles);

		ZapBeam(-1);
		ZapBeam(1);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
		// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
		ApplyMultiDamage(pev, pev);

		m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(3.5, 7.0);
	}
	break;

	// Why?
	case 2004:
	{
		//SpawnBlood(Center(), BLOOD_COLOR_YELLOW, 1);
		break;
	}

	case ZSLAVE_AE_CLAWRAKE:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zslaveDmgClawrake, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
		}
		else
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
		}
		ClearBeams();
		ClearMultiDamage();

		UTIL_MakeAimVectors(pev->angles);

		ZapBeam(-1);
		ZapBeam(1);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
		// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
		ApplyMultiDamage(pev, pev);

		m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(3.5, 7.0);
	}
	break;

	case ZSLAVE_AE_ZAP_POWERUP:
	{
		// speed up attack when on hard
		if (g_iSkillLevel == SKILL_HARD)
			pev->framerate = 1.5;
		else if (g_iSkillLevel == SKILL_MEDIUM)
			pev->framerate = 1.3;
		else
			pev->framerate = 1.1;

		UTIL_MakeAimVectors(pev->angles);

		if (m_iBeams == 0)
		{
			Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecSrc.x);			 // X
			WRITE_COORD(vecSrc.y);			 // Y
			WRITE_COORD(vecSrc.z);			 // Z
			WRITE_BYTE(12);					 // radius * 0.1
			WRITE_BYTE(255);				 // r
			WRITE_BYTE(180);				 // g
			WRITE_BYTE(96);					 // b
			WRITE_BYTE(20 / pev->framerate); // time * 10
			WRITE_BYTE(0);					 // decay * 0.1
			MESSAGE_END();
		}
		ArmBeam(-1);
		ArmBeam(1);
		BeamGlow();

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10);
		pev->skin = m_iBeams / 2;
	}
	break;

	case ZSLAVE_AE_ZAP_SHOOT:
	{
		ClearBeams();
		ClearMultiDamage();

		UTIL_MakeAimVectors(pev->angles);

		ZapBeam(-1);
		ZapBeam(1);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
		// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
		ApplyMultiDamage(pev, pev);

		m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(3.5, 7.0);
	}
	break;

	case ZSLAVE_AE_ZAP_DONE:
	{
		ClearBeams();
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack
//=========================================================
bool CZSlave::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return false;
	}

	return CBaseMonster::CheckRangeAttack1(flDot, flDist);
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
bool CZSlave::CheckRangeAttack2(float flDot, float flDist)
{
	return false;
}


//=========================================================
// StartTask
//=========================================================
void CZSlave::StartTask(Task_t* pTask)
{
	ClearBeams();
	switch (pTask->iTask)
	{
	case TASK_DIE:
		{
			//SetThink(&CZSlave::Blow);
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100);
			CBaseMonster::StartTask(pTask);
		}
		break;
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void CZSlave::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_DIE:
	{
		for (auto i = 0; i < 2; ++i)
		{
			Blow();
		}
		if (m_fSequenceFinished)
		{
			if (pev->frame >= 255)
			{
				pev->deadflag = DEAD_DEAD;

				pev->framerate = 0;

				//Flatten the bounding box so players can step on it
				if (BBoxFlat())
				{
					const auto maxs = Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 1);
					UTIL_SetSize(pev, pev->mins, maxs);
				}
				else
				{
					UTIL_SetSize(pev, { -4, -4, 0 }, { 4, 4, 1 });
				}
				pev->solid = SOLID_NOT;
				if (ShouldFadeOnDeath())
				{
					SUB_StartFadeOut();
				}
				else
				{
					CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 384, 30.0);
				}
				ClearBeams();
				ClearMultiDamage();

				MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
				WRITE_BYTE(TE_SPRITE);
				WRITE_COORD(pev->origin.x);
				WRITE_COORD(pev->origin.y);
				WRITE_COORD(pev->origin.z);
				WRITE_SHORT(m_iBlow);
				WRITE_BYTE(20);
				WRITE_BYTE(128);
				MESSAGE_END();
				::RadiusDamage(pev->origin, pev, pev, gSkillData.voltigoreDmgBeam, 160.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_SHOCK);
				TraceResult tr;
				UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
				UTIL_DecalTrace(&tr, DECAL_SPR_SPLT1 + RANDOM_LONG(0, 2));
				GibMonster();
			}
		}
	}
	break;

	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}


void CZSlave::PrescheduleThink(void)
{
}

//=========================================================
// Spawn
//=========================================================
void CZSlave::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zslave.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.zslaveHealth;
	pev->view_ofs = Vector(0, 0, 64);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch = RANDOM_LONG(70, 90);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZSlave::Precache()
{
	PRECACHE_MODEL("models/zslave.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	m_iBlow = PRECACHE_MODEL("sprites/spore_exp_01.spr");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	UTIL_PrecacheOther("test_effect");
}


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

bool CZSlave::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if ((bitsDamageType & DMG_SLASH) != 0 && pevAttacker && IRelationship(Instance(pevAttacker)) < R_DL)
		return false;

	m_afMemory |= bits_MEMORY_PROVOKED;
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CZSlave::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if ((bitsDamageType & DMG_SHOCK) != 0)
		return;

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t tlZSlaveAttack1[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slZSlaveAttack1[] =
{
	{tlZSlaveAttack1,
		ARRAYSIZE(tlZSlaveAttack1),
		bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_HEAR_SOUND |
			bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"Slave Range Attack1"},
};


DEFINE_CUSTOM_SCHEDULES(CZSlave) {
	slZSlaveAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CZSlave, CBaseMonster);


//=========================================================
//=========================================================
Schedule_t* CZSlave::GetSchedule()
{
	ClearBeams();

	/*
	if (pev->spawnflags)
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/

	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if ((pSound->m_iType & bits_SOUND_COMBAT) != 0)
			m_afMemory |= bits_MEMORY_PROVOKED;
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if (!HasConditions(bits_COND_SEE_ENEMY))
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		break;
	}
	return CBaseMonster::GetSchedule();
}


Schedule_t* CZSlave::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_FAIL:
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return CBaseMonster::GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slZSlaveAttack1;
	case SCHED_RANGE_ATTACK2:
		return slZSlaveAttack1;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CZSlave::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= ZSLAVE_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	// m_pBeam[m_iBeams]->SetColor(96, 128, 16);
	m_pBeam[m_iBeams]->SetBrightness(64);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CZSlave::BeamGlow()
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (!m_pBeam[i])
		{
			// Beams not restored and this may cause an error
			// So we will deleta all beams to avoid errors
			ClearBeams();
			//ALERT(at_error, "slave frame = %f, seq = %d\n", pev->framerate, pev->sequence);
			return;
		}
		if (m_pBeam[i]->GetBrightness() != 255)
		{
			m_pBeam[i]->SetBrightness(b);
		}
	}
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CZSlave::ZapBeam(int side)
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity* pEntity;

	if (m_iBeams >= ZSLAVE_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy(vecSrc);
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT(0, deflection) + gpGlobals->v_up * RANDOM_FLOAT(-deflection, deflection);
	UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(20);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && 0 != pEntity->pev->takedamage)
	{
		pEntity->TraceAttack(pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK);
	}
	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CZSlave::ClearBeams()
{
	for (int i = 0; i < ZSLAVE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND(ENT(pev), CHAN_WEAPON, "debris/zap4.wav");
}

#include "talkmonster.h"

///////////////////////////////////////////////////
/// Showel Grunt
//////////////////////////////////////////////////

class CShowelGrunt : public CZombie
{
public:
	void Spawn() override;
	void Precache() override;

	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();
	void DeathSound() override;

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
};

const char* CShowelGrunt::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CShowelGrunt::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CShowelGrunt::pAttackSounds[] =
{
	"hgrunt/shit!.wav",
	"hgrunt/shit.wav",
};

const char* CShowelGrunt::pIdleSounds[] =
{
	"hgrunt/gr_idle1.wav",
	"hgrunt/gr_idle2.wav",
	"hgrunt/gr_idle3.wav",
};

const char* CShowelGrunt::pAlertSounds[] =
{
	"hgrunt/hostiles!.wav",
	"hgrunt/ass!.wav",
	"hgrunt/fight!.wav",
};

const char* CShowelGrunt::pPainSounds[] =
{
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
};

LINK_ENTITY_TO_CLASS(monster_human_grunt_melee, CShowelGrunt);

//=========================================================
// Spawn
//=========================================================
void CShowelGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hgrunt_melee.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.hgruntHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShowelGrunt::Precache()
{
	PRECACHE_MODEL("models/hgrunt_melee.mdl");

	PRECACHE_SOUND("hgrunt/gr_step1.wav");
	PRECACHE_SOUND("hgrunt/gr_step2.wav");
	PRECACHE_SOUND("hgrunt/gr_step3.wav");
	PRECACHE_SOUND("hgrunt/gr_step4.wav");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

int CShowelGrunt::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

void CShowelGrunt::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, pitch);
}

void CShowelGrunt::AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);
	SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", 0.35, ATTN_NORM, 0, pitch);
}

void CShowelGrunt::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", 0.35, ATTN_NORM, 0, pitch);
}

void CShowelGrunt::AttackSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random attack sound
	SENTENCEG_PlayRndSz(ENT(pev), "HG_TAUNT", 0.35, ATTN_NORM, 0, pitch);
}

//=========================================================
// DeathSound
//=========================================================
void CShowelGrunt::DeathSound()
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CShowelGrunt::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
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
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
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

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CShowelGrunt::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}