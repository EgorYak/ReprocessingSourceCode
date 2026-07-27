/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

//=========================================================
// hassassin - Human assassin, fast and stealthy
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"game.h"

extern DLL_GLOBAL int  g_iSkillLevel;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ASSASSIN_EXPOSED = LAST_COMMON_SCHEDULE + 1,// cover was blown.
	SCHED_ASSASSIN_JUMP,	// fly through the air
	SCHED_ASSASSIN_JUMP_ATTACK,	// fly through the air and shoot
	SCHED_ASSASSIN_JUMP_LAND, // hit and run away
	SCHED_ASSASSIN_JUMP_CHASE,
};

//=========================================================
// monster-specific tasks
//=========================================================

enum
{
	TASK_ASSASSIN_FALL_TO_GROUND = LAST_COMMON_TASK + 1, // falling and waiting to hit ground
};


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ASSASSIN_AE_SHOOT1	1
#define		ASSASSIN_AE_TOSS1	2
#define		ASSASSIN_AE_JUMP	3
#define		ASSASSIN_AE_SHOOT2	4
#define		ASSASSIN_AE_CROSSBOW	5

#define HEAD_GROUP 1
#define HEAD_NVG 0
#define HEAD_MASK 1
#define HEAD_SHOT 2
#define GUN_GROUP 2
#define GUN_DEFAULT 0

#define HASSASSIN_ONE (1 << 0)
#define HASSASSIN_DUAL (1 << 1)
#define HASSASSIN_CROSSBOW (1 << 2)

#define bits_MEMORY_BADJUMP		(bits_MEMORY_CUSTOM1)

#define HASSASSIN_JUMP_HEIGHT 360.0f

struct LaunchToHeightResult
{
	float height;
	float gravity;
	float time;
	float speed;
};

LaunchToHeightResult CalcLaunchToHeight(float height)
{
	LaunchToHeightResult result;
	result.height = height;
	result.gravity = g_psv_gravity->value;
	if (result.gravity <= 1)
		result.gravity = 1;
	result.time = sqrt(height * 2.0f / result.gravity);
	result.speed = result.gravity * result.time;
	return result;
}

class CHAssassin : public CSquadMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	int  ISoundMask(void);
	void Shoot(void);
	void Shoot2(void);
	void Crossbow(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void PrescheduleThink(void);
	Schedule_t* GetSchedule(void);
	Schedule_t* GetScheduleOfType(int Type);
	bool CheckMeleeAttack1(float flDot, float flDist);	// jump
	// BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	bool CheckRangeAttack1(float flDot, float flDist);	// shoot
	bool CheckRangeAttack2(float flDot, float flDist);	// throw grenade
	void StartTask(Task_t *pTask);
	void RunAI(void);
	void RunTask(Task_t *pTask);
	void DeathSound(void);
	void IdleSound(void);
	bool TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);

	void EXPORT DeadUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	float JumpSpeedFactor(float jumpHeight = HASSASSIN_JUMP_HEIGHT) const
	{
		auto result = CalcLaunchToHeight(jumpHeight);
		return result.speed / jumpHeight;
	}
	Vector CalcMonsterJump(const Vector& vecTarget, float maximumHeight, float maximumDistance, int &result);
	bool FindJumpToSpot(const Vector& vecTarget);
	bool FindJumpToEntity(CBaseEntity* pEntity);

	//virtual int	ObjectCaps(void) { return CSquadMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed(entvars_t *pevAttacker, int iGib);

	CUSTOM_SCHEDULES;

	bool	Save(CSave &save);
	bool Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastShot;
	float m_flDiviation;

	float m_flNextJump;
	Vector m_vecJumpVelocity;

	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;
	bool	m_fThrowGrenade;

	int		m_iTargetRanderamt;

	int		m_iFrustration;

	int		m_iShell;
};
LINK_ENTITY_TO_CLASS(monster_human_assassin, CHAssassin);


TYPEDESCRIPTION	CHAssassin::m_SaveData[] =
{
	DEFINE_FIELD(CHAssassin, m_flLastShot, FIELD_TIME),
	DEFINE_FIELD(CHAssassin, m_flDiviation, FIELD_FLOAT),

	DEFINE_FIELD(CHAssassin, m_flNextJump, FIELD_TIME),
	DEFINE_FIELD(CHAssassin, m_vecJumpVelocity, FIELD_VECTOR),

	DEFINE_FIELD(CHAssassin, m_flNextGrenadeCheck, FIELD_TIME),
	DEFINE_FIELD(CHAssassin, m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CHAssassin, m_fThrowGrenade, FIELD_BOOLEAN),

	DEFINE_FIELD(CHAssassin, m_iTargetRanderamt, FIELD_INTEGER),
	DEFINE_FIELD(CHAssassin, m_iFrustration, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CHAssassin, CSquadMonster);


//=========================================================
// DieSound
//=========================================================
void CHAssassin::DeathSound(void)
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "ops/fa_die1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "ops/fa_die2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "ops/fa_die3.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "ops/fa_die4.wav", 1, ATTN_IDLE);
		break;
	}
}

//=========================================================
// IdleSound
//=========================================================
void CHAssassin::IdleSound(void)
{
	float m_flLastLightLevel = GETENTITYILLUM(ENT(pev));
	if (m_flLastLightLevel > 100)
		return;
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
bool CHAssassin::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	Forget(bits_MEMORY_INCOVER);

	/*if (m_bHeadshot)
	{
		Vector	vecAng = Vector(0, 0, 68);

		CGib::SpawnStickyGibs(pev, vecAng, 6);
		SetBodygroup(2, 2);
	}*/
	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CHAssassin::Killed(entvars_t *pevAttacker, int iGib)
{
	//SetUse(&CHAssassin::DeadUse);
	if (InSquad())
	{
		CSquadMonster *pSquadLeader = MySquadLeader();
		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
		{
			CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
			if (pMember && pMember != this)
			{
				if (pMember->m_hEnemy == NULL)
					pMember->MoveToLocation(ACT_RUN, 0, pev->origin);
			}
		}
	}
	CSquadMonster::Killed(pevAttacker, iGib);
}

void CHAssassin::DeadUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;
	GetAttachment(0, vecGunPos, vecGunAngles);
	Vector	vecGunPos2;
	Vector	vecGunAngles2;
	GetAttachment(1, vecGunPos2, vecGunAngles2);
	SetBodygroup(1, 1);
	if (pCaller != NULL && pCaller->IsPlayer())
	{
		CBaseEntity *pGun;
		pGun = DropItem("weapon_glock", vecGunPos, vecGunAngles);
		pGun = DropItem("weapon_glock", vecGunPos2, vecGunAngles2);
		SetUse(NULL);
	}

}


//=========================================================
// TraceAttack
//=========================================================
void CHAssassin::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	//switch (ptr->iHitgroup)
	//{
	//case HITGROUP_HEAD:
	//	if (m_bHeadshot)
	//		return;
	//	break;
	//}
	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CHAssassin::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHAssassin::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHAssassin::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 360;
		break;
	default:
		ys = 360;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// Shoot
//=========================================================
void CHAssassin::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	if (m_flLastShot + 2 < gpGlobals->time)
	{
		m_flDiviation = 0.10;
	}
	else
	{
		m_flDiviation -= 0.01;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(m_flDiviation, m_flDiviation, m_flDiviation), 2048, BULLET_MONSTER_9MM); // shoot +-8 degrees

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	}

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	m_cAmmoLoaded--;
}

//=========================================================
// Shoot
//=========================================================
void CHAssassin::Shoot2(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = Vector(0, -24, 48);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	if (m_flLastShot + 2 < gpGlobals->time)
	{
		m_flDiviation = 0.10;
	}
	else
	{
		m_flDiviation -= 0.01;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(-40, -90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(m_flDiviation, m_flDiviation, m_flDiviation), 2048, BULLET_MONSTER_9MM); // shoot +-8 degrees

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	}

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	m_cAmmoLoaded--;
}

//=========================================================
// Crossbow
//=========================================================
void CHAssassin::Crossbow(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}
	if (gpGlobals->time - m_flLastShot <= 2.5)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	if (m_flLastShot + 6 < gpGlobals->time)
	{
		m_flDiviation = 0.10;
	}
	else
	{
		m_flDiviation -= 0.01;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors(pev->angles);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	CCrossbowBolt* pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecShootOrigin;
	pBolt->pev->angles = angDir;
	pBolt->pev->velocity = vecShootDir * 1000;
	pBolt->pev->speed = 1000;
	pBolt->pev->avelocity.z = 10;
	pBolt->pev->owner = ENT(pev);

	SetBlending(0, angDir.x);

	m_cAmmoLoaded--;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHAssassin::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ASSASSIN_AE_SHOOT1:
		Shoot();
		break;
	case ASSASSIN_AE_SHOOT2:
		Shoot2();
		break;
	case ASSASSIN_AE_CROSSBOW:
		Crossbow();
		break;
	case ASSASSIN_AE_TOSS1:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 34 + Vector(0, 0, 32), m_vecTossVelocity, 2.0);

		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		m_fThrowGrenade = false;
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;
	case ASSASSIN_AE_JUMP:
	{
		// ALERT( at_console, "jumping");
		UTIL_MakeAimVectors(pev->angles);
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;
		pev->velocity = m_vecJumpVelocity;
		m_flNextJump = gpGlobals->time + 3.0;
	}
	return;
	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHAssassin::Spawn()
{
	Precache();

	if (FBitSet(pev->weapons, HASSASSIN_CROSSBOW))
	{
		SET_MODEL(ENT(pev), "models/hassassin_bow.mdl");
	}
	else if (FBitSet(pev->weapons, HASSASSIN_DUAL))
		SET_MODEL(ENT(pev), "models/hassassin_dual.mdl");
	else
		SET_MODEL(ENT(pev), "models/hassassin.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hassassinHealth;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP;
	pev->friction = 1;

	m_HackedGunPos = Vector(0, 24, 48);

	m_fEnemyEluded = false;

	m_iTargetRanderamt = 20;
	pev->renderamt = 20;
	pev->rendermode = kRenderTransTexture;

	MonsterInit();
	if (FBitSet(pev->weapons, HASSASSIN_CROSSBOW))
	{
		m_flDistTooFar = 99999999.0;
		m_flDistLook = 4000.0;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHAssassin::Precache()
{
	PRECACHE_MODEL("models/hassassin.mdl");
	PRECACHE_MODEL("models/hassassin_bow.mdl");
	PRECACHE_MODEL("models/hassassin_dual.mdl");

	PRECACHE_SOUND("weapons/pl_gun1.wav");
	PRECACHE_SOUND("weapons/pl_gun2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");

	PRECACHE_SOUND("ops/fa_die1.wav");
	PRECACHE_SOUND("ops/fa_die2.wav");
	PRECACHE_SOUND("ops/fa_die3.wav");
	PRECACHE_SOUND("ops/fa_die4.wav");

	PRECACHE_SOUND("debris/beamstart1.wav");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}



//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t	tlAssassinFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)0		},
	// { TASK_WAIT_PVS,			(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slAssassinFail[] =
{
	{
		tlAssassinFail,
		ARRAYSIZE(tlAssassinFail),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_PLAYER,
		"AssassinFail"
	},
};


//=========================================================
// Enemy exposed Agrunt's cover
//=========================================================
Task_t	tlAssassinExposed[] =
{
	{ TASK_STOP_MOVING,			(float)0							},
	{ TASK_RANGE_ATTACK1,		(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ASSASSIN_JUMP			},
	{ TASK_SET_SCHEDULE,		(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slAssassinExposed[] =
{
	{
		tlAssassinExposed,
		ARRAYSIZE(tlAssassinExposed),
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AssassinExposed",
	},
};


//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAssassinTakeCoverFromEnemy[] =
{
	//{ TASK_STOP_MOVING,				(float)0					},
	//{ TASK_WAIT,					(float)0.1					},
	//{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MELEE_ATTACK1	},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAssassinTakeCoverFromEnemy[] =
{
	{
		tlAssassinTakeCoverFromEnemy,
		ARRAYSIZE(tlAssassinTakeCoverFromEnemy),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy"
	},
};


//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAssassinTakeCoverFromEnemy2[] =
{
	//{ TASK_STOP_MOVING,				(float)0					},
	//{ TASK_WAIT,					(float)0.1					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_RANGE_ATTACK1,			(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK2	},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384			},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAssassinTakeCoverFromEnemy2[] =
{
	{
		tlAssassinTakeCoverFromEnemy2,
		ARRAYSIZE(tlAssassinTakeCoverFromEnemy2),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy2"
	},
};


//=========================================================
// hide from the loudest sound source
//=========================================================
Task_t	tlAssassinTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_MELEE_ATTACK1	},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slAssassinTakeCoverFromBestSound[] =
{
	{
		tlAssassinTakeCoverFromBestSound,
		ARRAYSIZE(tlAssassinTakeCoverFromBestSound),
		bits_COND_NEW_ENEMY,
		0,
		"AssassinTakeCoverFromBestSound"
	},
};





//=========================================================
// AlertIdle Schedules
//=========================================================
Task_t	tlAssassinHide[] =
{
	{ TASK_STOP_MOVING,			0						 },
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE			 },
	{ TASK_WAIT,				(float)0				 },
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slAssassinHide[] =
{
	{
		tlAssassinHide,
		ARRAYSIZE(tlAssassinHide),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_FEAR |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_PROVOKED |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssassinHide"
	},
};



//=========================================================
// HUNT Schedules
//=========================================================
Task_t tlAssassinHunt[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slAssassinHunt[] =
{
	{
		tlAssassinHunt,
		ARRAYSIZE(tlAssassinHunt),
		bits_COND_NEW_ENEMY |
		// bits_COND_SEE_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssassinHunt"
	},
};


//=========================================================
// Jumping Schedules
//=========================================================
Task_t	tlAssassinJump[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_HOP	},
	{ TASK_SET_SCHEDULE,		(float)SCHED_ASSASSIN_JUMP_ATTACK },
};

Schedule_t	slAssassinJump[] =
{
	{
		tlAssassinJump,
		ARRAYSIZE(tlAssassinJump),
		0,
		0,
		"AssassinJump"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlAssassinJumpAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ASSASSIN_JUMP_LAND	},
	// { TASK_SET_ACTIVITY,		(float)ACT_FLY	},
	{ TASK_ASSASSIN_FALL_TO_GROUND, (float)0		},
};


Schedule_t	slAssassinJumpAttack[] =
{
	{
		tlAssassinJumpAttack,
		ARRAYSIZE(tlAssassinJumpAttack),
		0,
		0,
		"AssassinJumpAttack"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlAssassinJumpLand[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_ASSASSIN_EXPOSED	},
	// { TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MELEE_ATTACK1	},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_REMEMBER,				(float)bits_MEMORY_BADJUMP	},
	{ TASK_FIND_NODE_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_FORGET,					(float)bits_MEMORY_BADJUMP	},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
};

Schedule_t	slAssassinJumpLand[] =
{
	{
		tlAssassinJumpLand,
		ARRAYSIZE(tlAssassinJumpLand),
		0,
		0,
		"AssassinJumpLand"
	},
};

Task_t tlAssassinJumpChase[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_HOP },
	{ TASK_ASSASSIN_FALL_TO_GROUND, (float)0 },
};

Schedule_t slAssassinJumpChase[] =
{
	{
		tlAssassinJumpChase,
		ARRAYSIZE(tlAssassinJumpChase),
		0,
		0,
		"AssassinJumpChase"
	},
};

DEFINE_CUSTOM_SCHEDULES(CHAssassin)
{
	slAssassinFail,
		slAssassinExposed,
		slAssassinTakeCoverFromEnemy,
		slAssassinTakeCoverFromEnemy2,
		slAssassinTakeCoverFromBestSound,
		slAssassinHide,
		slAssassinHunt,
		slAssassinJump,
		slAssassinJumpAttack,
		slAssassinJumpLand,
		slAssassinJumpChase,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHAssassin, CSquadMonster);


//=========================================================
// CheckMeleeAttack1 - jump like crazy if the enemy gets too close. 
//=========================================================
bool CHAssassin::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_flNextJump < gpGlobals->time && (flDist <= 128 || HasMemory(bits_MEMORY_BADJUMP)) && m_hEnemy != NULL)
	{
		TraceResult	tr;

		Vector vecDest = pev->origin + Vector(RANDOM_FLOAT(-64, 64), RANDOM_FLOAT(-64, 64), 160);

		UTIL_TraceHull(pev->origin + Vector(0, 0, 36), vecDest + Vector(0, 0, 36), dont_ignore_monsters, human_hull, ENT(pev), &tr);

		if (tr.fStartSolid || tr.flFraction < 1.0)
		{
			return false;
		}

		float flGravity = g_psv_gravity->value;

		float time = sqrt(160 / (0.5 * flGravity));
		float speed = flGravity * time / 160;
		m_vecJumpVelocity = (vecDest - pev->origin) * speed;

		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack1  - drop a cap in their ass
//
//=========================================================
bool CHAssassin::CheckRangeAttack1(float flDot, float flDist)
{
	if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist > 64 && flDist <= 2048 /* && flDot >= 0.5 */ && NoFriendlyFire())
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine(vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction == 1 || tr.pHit == m_hEnemy->edict())
		{
			return true;
		}
	}
	return false;
}

//=========================================================
// CheckRangeAttack2 - toss grenade is enemy gets in the way and is too close. 
//=========================================================
bool CHAssassin::CheckRangeAttack2(float flDot, float flDist)
{
	m_fThrowGrenade = false;


	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
	{
		// don't throw grenades at anything that isn't on the ground!
		return false;
	}

	// don't get grenade happy unless the player starts to piss you off
	if (m_iFrustration <= 2)
		return false;

	Vector vecToss = VecCheckThrow(pev, GetGunPosition(), m_hEnemy->Center(), flDist, 0.5); // use dist as speed to get there in 1 second

	// are any of my squad members near the intended grenade impact area?
	if (InSquad())
	{
		if (SquadMemberInRange(vecToss, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = false;
			return m_fThrowGrenade;
		}
	}

	if ((vecToss - pev->origin).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	if (m_flNextGrenadeCheck < gpGlobals->time && !HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 512 /* && flDot >= 0.5 */ && NoFriendlyFire())
	{

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;

			return true;
		}
	}

	return false;
}


//=========================================================
// RunAI
//=========================================================
void CHAssassin::RunAI(void)
{
	CSquadMonster::RunAI();

	// always visible if moving
	// always visible is not on hard
	if (g_iSkillLevel < SKILL_MEDIUM || m_hEnemy == NULL || pev->deadflag != DEAD_NO || m_Activity == ACT_RUN || m_Activity == ACT_WALK || !(pev->flags & FL_ONGROUND))
		m_iTargetRanderamt = 255;
	else
		m_iTargetRanderamt = 20;

	if (pev->renderamt > m_iTargetRanderamt)
	{
		if (pev->renderamt == 255)
		{
			EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/beamstart1.wav", 0.2, ATTN_NORM);
		}

		pev->renderamt = V_max(pev->renderamt - 50, m_iTargetRanderamt);
		pev->rendermode = kRenderTransTexture;
	}
	else if (pev->renderamt < m_iTargetRanderamt)
	{
		pev->renderamt = V_min(pev->renderamt + 50, m_iTargetRanderamt);
		if (pev->renderamt == 255)
			pev->rendermode = kRenderNormal;
	}

	/*
	if (m_Activity == ACT_RUN || m_Activity == ACT_WALK)
	{
		static int iStep = 0;
		iStep = !iStep;
		if (iStep)
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_step1.wav", 0.5, ATTN_NORM);	break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_step3.wav", 0.5, ATTN_NORM);	break;
			case 2:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_step2.wav", 0.5, ATTN_NORM);	break;
			case 3:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_step4.wav", 0.5, ATTN_NORM);	break;
			}
		}
	}
	*/
}


//=========================================================
// StartTask
//=========================================================
void CHAssassin::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK2:
		if (!m_fThrowGrenade)
		{
			TaskComplete();
		}
		else
		{
			CSquadMonster::StartTask(pTask);
		}
		break;
	case TASK_ASSASSIN_FALL_TO_GROUND:
		break;
	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}


//=========================================================
// RunTask 
//=========================================================
void CHAssassin::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ASSASSIN_FALL_TO_GROUND:
		MakeIdealYaw(m_vecEnemyLKP);
		ChangeYaw(pev->yaw_speed);

		if (m_fSequenceFinished)
		{
			if (pev->velocity.z > 0)
			{
				pev->sequence = LookupSequence("fly_up");
			}
			else if (HasConditions(bits_COND_SEE_ENEMY))
			{
				pev->sequence = LookupSequence("fly_attack");
				pev->frame = 0;
			}
			else
			{
				pev->sequence = LookupSequence("fly_down");
				pev->frame = 0;
			}

			ResetSequenceInfo();
			SetYawSpeed();
		}
		if (pev->flags & FL_ONGROUND)
		{
			// ALERT( at_console, "on ground\n");
			pev->sequence = LookupSequence("landfromjump");
			pev->frame = 0;
			TaskComplete();
		}
		break;
	default:
		CSquadMonster::RunTask(pTask);
		break;
	}
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHAssassin::PrescheduleThink(void)
{
	if (InSquad() && m_hEnemy != NULL)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if (gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5)
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = true;
			}
		}
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHAssassin::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	{
		if (HasConditions(bits_COND_HEAR_SOUND))
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT(pSound != NULL);
			if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			{
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			if (pSound && (pSound->m_iType & bits_SOUND_COMBAT))
			{
				return GetScheduleOfType(SCHED_INVESTIGATE_SOUND);
			}
		}
	}
	break;

	case MONSTERSTATE_COMBAT:
	{
		if (!MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_SEE_ENEMY))
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = true;
			}
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// flying?
		if (pev->movetype == MOVETYPE_TOSS)
		{
			if (pev->flags & FL_ONGROUND)
			{
				// ALERT( at_console, "landed\n");
				// just landed
				pev->movetype = MOVETYPE_STEP;
				return GetScheduleOfType(SCHED_ASSASSIN_JUMP_LAND);
			}
			else
			{
				// ALERT( at_console, "jump\n");
				// jump or jump/shoot
				if (m_MonsterState != MONSTERSTATE_COMBAT)
					return GetScheduleOfType(SCHED_ASSASSIN_JUMP);
				else
					return GetScheduleOfType(SCHED_ASSASSIN_JUMP_ATTACK);
			}
		}

		if (HasConditions(bits_COND_HEAR_SOUND))
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT(pSound != NULL);
			if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			{
				//CBaseEntity *pOther = NULL;
				//pOther = UTIL_FindEntityByClassname(pOther, "grenade");
				//if (pOther != NULL)
				//{
				//	UTIL_Remove(pOther);
				//}

				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			else if (pSound && (pSound->m_iType & bits_SOUND_COMBAT))
			{
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			m_iFrustration++;
			return GetScheduleOfType(SCHED_ASSASSIN_EXPOSED);
		}
		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			m_iFrustration++;
			return GetScheduleOfType(SCHED_ASSASSIN_EXPOSED);
		}

		// jump player!
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			// ALERT( at_console, "melee attack 1\n");
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		// throw grenade
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			// ALERT( at_console, "range attack 2\n");
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}

		// spotted
		if (HasConditions(bits_COND_SEE_ENEMY) && HasConditions(bits_COND_ENEMY_FACING_ME))
		{
			// ALERT( at_console, "exposed\n");
			m_iFrustration++;
			return GetScheduleOfType(SCHED_ASSASSIN_EXPOSED);
		}

		// can attack
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			// ALERT( at_console, "range attack 1\n");
			m_iFrustration = 0;
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// ALERT( at_console, "face\n");
			return GetScheduleOfType(SCHED_COMBAT_FACE);
		}

		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (InSquad())
			{
				m_fEnemyEluded = false;
				SquadMakeEnemy(m_hEnemy);
			}
			// ALERT( at_console, "take cover\n");
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
		}

		// ALERT( at_console, "stand\n");
		return GetScheduleOfType(SCHED_ALERT_STAND);
	}
	break;
	}

	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHAssassin::GetScheduleOfType(int Type)
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if (pev->health > 30)
			return slAssassinTakeCoverFromEnemy;
		else
			return slAssassinTakeCoverFromEnemy2;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slAssassinTakeCoverFromBestSound;
	case SCHED_ASSASSIN_EXPOSED:
		return slAssassinExposed;
	case SCHED_FAIL:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
			return slAssassinFail;
		break;
	case SCHED_ALERT_STAND:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
			return slAssassinHide;
		break;
	case SCHED_CHASE_ENEMY:
		return slAssassinHunt;
	case SCHED_MELEE_ATTACK1:
		if (pev->flags & FL_ONGROUND)
		{
			if (m_flNextJump > gpGlobals->time)
			{
				// can't jump yet, go ahead and fail
				return slAssassinFail;
			}
			else
			{
				return slAssassinJump;
			}
		}
		else
		{
			return slAssassinJumpAttack;
		}
	case SCHED_ASSASSIN_JUMP:
	case SCHED_ASSASSIN_JUMP_ATTACK:
		return slAssassinJumpAttack;
	case SCHED_ASSASSIN_JUMP_LAND:
		return slAssassinJumpLand;
	case SCHED_ASSASSIN_JUMP_CHASE:
		return slAssassinJumpChase;
	case SCHED_CHASE_ENEMY_FAILED:
		{
			if (m_flNextJump <= gpGlobals->time && FindJumpToEntity(m_hEnemy))
			{
				m_flNextJump = gpGlobals->time + 1.0f;
				return GetScheduleOfType(SCHED_ASSASSIN_JUMP_CHASE);
			}
		}
	}

	return CSquadMonster::GetScheduleOfType(Type);
}

static Vector TraceBottom(const Vector& pos, edict_t* pIgnore, float deepness = 500)
{
	TraceResult tr;
	UTIL_TraceLine(pos, pos - Vector(0, 0, deepness), ignore_monsters, pIgnore, &tr);
	return tr.vecEndPos;
}

enum
{
	MONSTERJUMP_OK = 0,
	MONSTERJUMP_CLOSE_IN_2D,
	MONSTERJUMP_TOOFAR_SOLID,
	MONSTERJUMP_TOOFAR_TOODEEP,
	MONSTERJUMP_TOOHIGH,
	MONSTERJUMP_NO_TRACE_TO_APEX
};

Vector CHAssassin::CalcMonsterJump(const Vector &vecTarget, float maximumHeight, float maximumDistance, int &result)
{
	const float myHeight = pev->size.z;
	const float halfMyHeight = myHeight * 0.5f;
	const Vector halfMyHeightVec(0.0f, 0.0f, halfMyHeight);
	Vector vecChosenDest = vecTarget;

	const Vector2D vec2DDist = (vecChosenDest - pev->origin).Make2D();
	const float dist2DSqr = vec2DDist.Length() * vec2DDist.Length();
	const bool isTooFar = dist2DSqr > maximumDistance * maximumDistance;
	const bool isTooCloseIn2D = dist2DSqr < 48.0f * 48.0f;
	if (isTooCloseIn2D)
	{
		result = MONSTERJUMP_CLOSE_IN_2D;
		return g_vecZero;
	}

	// Can't jump that far. Just jump to get closer
	if (isTooFar)
	{
		const Vector2D horizontalShift = vec2DDist.Normalize() * maximumDistance;
		vecChosenDest.x = pev->origin.x + horizontalShift.x;
		vecChosenDest.y = pev->origin.y + horizontalShift.y;

		if (UTIL_PointContents(vecChosenDest) == CONTENTS_SOLID)
		{
			result = MONSTERJUMP_TOOFAR_SOLID;
			return g_vecZero;
		}

		Vector bottom = TraceBottom(vecChosenDest, edict());
		if (bottom.z + 8.0f < V_min(pev->origin.z, vecChosenDest.z))
		{
			//DrawBeamLine(vecChosenDest, bottom, Color3(255, 100, 50));
			const Vector2D halfHorizontalShift = horizontalShift * 0.5f;
			vecChosenDest.x = pev->origin.x + halfHorizontalShift.x;
			vecChosenDest.y = pev->origin.y + halfHorizontalShift.y;

			if (UTIL_PointContents(vecChosenDest) == CONTENTS_SOLID)
			{
				result = MONSTERJUMP_TOOFAR_TOODEEP;
				return g_vecZero;
			}
			else
			{
				bottom = TraceBottom(vecChosenDest, edict());
				if (bottom.z + 8.0f < V_min(pev->origin.z, vecChosenDest.z))
				{
					result = MONSTERJUMP_TOOFAR_TOODEEP;
					return g_vecZero;
				}
			}
		}
	}

	Vector midPoint = (vecChosenDest + pev->origin) * 0.5f;

	TraceResult mindpointTr;
	UTIL_TraceLine(midPoint, midPoint + Vector(0, 0, V_max(500.0f, maximumHeight + myHeight)), ignore_monsters, edict(), &mindpointTr);
	const float ceilingZ = mindpointTr.vecEndPos.z;

	const float jumpMinZ = V_max(vecChosenDest.z, pev->origin.z) + halfMyHeight * 0.5f;
	const float jumpMaxZ = V_max(ceilingZ - myHeight, jumpMinZ);

	if (jumpMinZ - pev->origin.z > maximumHeight)
	{
		result = MONSTERJUMP_TOOHIGH;
		return g_vecZero;
	}

	Vector vecApex = midPoint;
	vecApex.z = V_min(pev->origin.z + maximumHeight, jumpMaxZ);

	const Vector vecTraceHullStart = pev->origin + halfMyHeightVec;
	const Vector vectraceHullEnd = vecApex + halfMyHeightVec;

	TraceResult tr;
	UTIL_TraceHull(vecTraceHullStart, vectraceHullEnd, dont_ignore_monsters, human_hull, edict(), &tr);

	if (!tr.fStartSolid && tr.flFraction == 1.0f)
	{
		//DrawBeamLine(vecTraceHullStart, vectraceHullEnd, Color3(0, 255, 0));

		auto launchCalcResult = CalcLaunchToHeight(vecApex.z - pev->origin.z);

		Vector velocity;
		velocity.x = (vecApex.x - pev->origin.x) / launchCalcResult.time;
		velocity.y = (vecApex.y - pev->origin.y) / launchCalcResult.time;
		velocity.z = launchCalcResult.speed;
		result = MONSTERJUMP_OK;
		return velocity;
	}
	else
	{
		//DrawBeamLine(vecTraceHullStart, vectraceHullEnd, Color3(255, 0, 0));
		result = MONSTERJUMP_NO_TRACE_TO_APEX;
		return g_vecZero;
	}
}

bool CHAssassin::FindJumpToSpot(const Vector& vecTarget)
{
	const float horizontalJumpDistance = 360.0f;

	const Vector2D vec2DDist = (vecTarget - pev->origin).Make2D();
	const float dist2DSqr = vec2DDist.Length() * vec2DDist.Length();
	const bool isTooCloseIn2D = dist2DSqr < 48.0f * 48.0f;

	if (isTooCloseIn2D)
	{
		if (std::fabs(pev->origin.z - vecTarget.z) <= pev->size.z)
		{
			// Refuse early
			//ALERT(at_console, "%s: already is close to the target\n", STRING(pev->classname));
			return false;
		}
		if (pev->origin.z < vecTarget.z)
		{
			// The target is on top. Can't handle it for now
			return false;
		}

		// The target is underneath. Try jump off to it
		float directions[][2] = {
			{1, 0},
			{-1, 0},
			{0, 1},
			{0, -1}
		};

		const float spotOffset = V_min(128.0f, horizontalJumpDistance);

		const size_t indexShift = RANDOM_LONG(0, ARRAYSIZE(directions) - 1);

		for (size_t i = 0; i < ARRAYSIZE(directions); ++i)
		{
			size_t j = (i + indexShift) % ARRAYSIZE(directions);
			const auto& randomDir = directions[j];

			Vector vecSpot = pev->origin + Vector(randomDir[0] * spotOffset, randomDir[1] * spotOffset, 0.0f);
			vecSpot.z = vecTarget.z;

			int result;
			auto jumpResult = CalcMonsterJump(vecSpot, HASSASSIN_JUMP_HEIGHT, horizontalJumpDistance, result);
			if (jumpResult != g_vecZero)
			{
				m_vecJumpVelocity = jumpResult;
				return true;
			}
		}

		return false;
	}

	//TODO: calculate this depending on the monster and player geometry?
	//const FloatRange baseOffsetRange(45.0f, 50.0f); // don't jump right onto the target

	const Vector2D vecTarget2D = vecTarget.Make2D();
	const Vector2D vecToTarget = vecTarget2D - pev->origin.Make2D();
	const Vector2D dirToTarget = vecToTarget.Normalize();
	const Vector2D inFrontOfTarget = vecTarget2D - dirToTarget;//* RandomizeNumberFromRange(baseOffsetRange);
	const Vector2D toRightFromTarget = vecTarget2D + Vector2D(dirToTarget.y, -dirToTarget.x);// *RandomizeNumberFromRange(baseOffsetRange);
	const Vector2D toLeftFromTarget = vecTarget2D + Vector2D(-dirToTarget.y, dirToTarget.x);// *RandomizeNumberFromRange(baseOffsetRange);

	const Vector possibleTargets[] = {
		Vector(inFrontOfTarget, vecTarget.z),
		Vector(toRightFromTarget, vecTarget.z),
		Vector(toLeftFromTarget, vecTarget.z)
	};

	const size_t indexShift = RANDOM_LONG(0, ARRAYSIZE(possibleTargets) - 1);

	for (size_t i = 0; i < ARRAYSIZE(possibleTargets); ++i)
	{
		size_t j = (i + indexShift) % ARRAYSIZE(possibleTargets);

		const Vector& vecPossibleTarget = possibleTargets[j];
		const Vector bottom = TraceBottom(vecPossibleTarget, edict());

		if (bottom.z + 8.0f < V_min(pev->origin.z, vecPossibleTarget.z))
		{
			continue;
		}

		int result;
		auto jumpResult = CalcMonsterJump(vecPossibleTarget, HASSASSIN_JUMP_HEIGHT, horizontalJumpDistance, result);
		if (jumpResult != g_vecZero)
		{
			m_vecJumpVelocity = jumpResult;
			//ALERT(at_console, "%s: vertical jump velocity: %g; horizontal jump velocity: %g\n", STRING(pev->classname), m_vecJumpVelocity.z, m_vecJumpVelocity.Make2D().Length());
			return true;
		}
		else
		{
			/*switch(jumpResult.second)
			{
			case MONSTERJUMP_CLOSE_IN_2D:
				ALERT(at_console, "%s can't make jump: is already too close\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOFAR_SOLID:
				ALERT(at_console, "%s can't make jump: the target is to far and the new selected point is in solid volume\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOFAR_TOODEEP:
				ALERT(at_console, "%s can't make jump: new selected point is too deep\n", STRING(pev->classname));
				break;
			case MONSTERJUMP_TOOHIGH:
				ALERT(at_console, "%s can't make jump: too high!\n", STRING(pev->classname));
				break;
			default:
				ALERT(at_console, "%s can't make jump: can't calculate jump\n", STRING(pev->classname));
				break;
			}*/
		}
	}

	return false;
}

bool CHAssassin::FindJumpToEntity(CBaseEntity* pEntity)
{
	if (!pEntity)
		return false;

	const Vector vecTarget(pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->absmin.z + 1.0f);
	return FindJumpToSpot(vecTarget);
}

#endif