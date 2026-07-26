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


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include "talkmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"

#define HASSAULT_SENTENCE_VOLUME (float)0.35			// volume of Assault sentences
#define HASSAULT_VOL 0.35		 // volume of Assault sounds
#define HASSAULT_ATTN ATTN_NORM // attenutation of Assault sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HASSAULT_AE_FIRE		( 1 )
#define		HASSAULT_AE_GREN		( 2 )
#define		HASSAULT_AE_RELOAD		( 3 )
#define		HASSAULT_AE_KICK		( 4 )

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_Assault_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_Assault_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_Assault_COVER_AND_RELOAD,
	SCHED_Assault_SWEEP,
	SCHED_Assault_FOUND_ENEMY,
	SCHED_Assault_WAIT_FACE_ENEMY,
	SCHED_Assault_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_Assault_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_Assault_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_Assault_SPEAK_SENTENCE,
	TASK_Assault_CHECK_FIRE,
};

class CHassault : public CSquadMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);

	void DeathSound(void);
	void PainSound( void );

	bool CheckMeleeAttack1(float flDot, float flDist);
	bool CheckMeleeAttack2(float flDot, float flDist) { return false; }
	bool CheckRangeAttack1 ( float flDot, float flDist );
	bool CheckRangeAttack2(float flDot, float flDist);

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	bool	Save( CSave &save ); 
	bool Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	void Shoot(void);
	void GrenadeShoot(void);

	void CheckAmmo ( void );

	static const char *pDeathSounds[];
	static const char *pPainSounds[];
	static const char *pBurstSounds[];

	bool FOkToSpeak();
	void JustSpoke();
	void SpeakSentence();

	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;

	CBaseEntity* Kick();
	Schedule_t	*GetSchedule( void );
	Schedule_t* GetScheduleOfType(int Type) override;

	CUSTOM_SCHEDULES;

	bool	m_fThrowGrenade;
	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;

	int		m_iSentence, m_cClipSize;

	static const char* pAssaultSentences[];
private:
	int		m_iShell;
};

LINK_ENTITY_TO_CLASS(monster_human_assault, CHassault );

TYPEDESCRIPTION	CHassault::m_SaveData[] = 
{
	DEFINE_FIELD( CHassault, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHassault, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHassault, m_cClipSize, FIELD_INTEGER ),
		DEFINE_FIELD(CHassault, m_iSentence, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE( CHassault, CSquadMonster );

const char* CHassault::pAssaultSentences[] =
{
	"HG_GREN",	  // grenade scared Assault
	"HG_ALERT",	  // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER",	  // running to cover
	"HG_THROW",	  // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT",	  // say rude things
};

enum HASSAULT_SENTENCE_TYPES
{
	HASSAULT_SENT_NONE = -1,
	HASSAULT_SENT_GREN = 0,
	HASSAULT_SENT_ALERT,
	HASSAULT_SENT_MONSTER,
	HASSAULT_SENT_COVER,
	HASSAULT_SENT_THROW,
	HASSAULT_SENT_CHARGE,
	HASSAULT_SENT_TAUNT,
};

const char *CHassault::pPainSounds[] =
{
	"hgrunt/gr_pain1.wav",
	"hgrunt/gr_pain2.wav",
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
	"hgrunt/gr_pain5.wav",
};

const char *CHassault::pDeathSounds[] =
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

const char *CHassault::pBurstSounds[] =
{
	"hassault/hw_shoot1.wav",
	"hassault/hw_shoot2.wav",
	"hassault/hw_shoot3.wav",
};

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHassault::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
bool CHassault::FOkToSpeak()
{
	// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return false;

	if ((pev->spawnflags & SF_MONSTER_GAG) != 0)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// no talking outside of combat if gagged.
			return false;
		}
	}

	// if player is not in pvs, don't speak
	//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
	//		return false;

	return true;
}

//=========================================================
//=========================================================
void CHassault::JustSpoke()
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HASSAULT_SENT_NONE;
}

//=========================================================
// PainSound
//=========================================================
void CHassault :: PainSound ( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM);
}

//=========================================================
// CheckAmmo - overridden for the Assault because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHassault :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
bool CHassault::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget(bits_MEMORY_INCOVER);

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHassault::HandleAnimEvent(MonsterEvent_t *pEvent)
{	
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case HASSAULT_AE_RELOAD:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hAssault/gr_reload1.wav", 1, ATTN_NORM);
		if (FBitSet(pev->weapons, 1))
			m_cAmmoLoaded = 3;
		else
			m_cAmmoLoaded = 100;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		Forget(bits_MEMORY_INCOVER);
		break;

	case HASSAULT_AE_GREN:
	{
		GrenadeShoot();
	}
	break;

	case HASSAULT_AE_FIRE:
	{
		Forget(bits_MEMORY_INCOVER);
		if (FBitSet(pev->weapons, 1))
			GrenadeShoot();
		else
		{
			Shoot();
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBurstSounds), 1, ATTN_NORM);
		}
	}
	break;

	case HASSAULT_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB);
		}
	}
	break;
	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CHassault::Spawn()
{
	Precache();

	if (FBitSet(pev->weapons, 1))
		SET_MODEL(ENT(pev), "models/grenadier.mdl");
	else
		SET_MODEL(ENT(pev), "models/hassault.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	m_flNextGrenadeCheck = gpGlobals->time + 1;

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 100;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP;

	m_HackedGunPos = Vector(0, 0, 55);

	if (FBitSet(pev->weapons, 1))
		m_cAmmoLoaded = 3;
	else
		m_cAmmoLoaded = 100;
	ClearConditions(bits_COND_NO_AMMO_LOADED);

	MonsterInit();
	m_flDistTooFar = 99999999.0;
	m_flDistLook = 4000.0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHassault::Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");
	PRECACHE_MODEL("models/grenadier.mdl");

	PRECACHE_SOUND("hassault/hw_gun4.wav");

	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBurstSounds);

	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHassault::Classify(void)
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// CheckRangeAttack1 - overridden for HAssault, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HAssault can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
bool CHassault :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() && !FBitSet(pev->weapons, 1))
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 || m_cAmmoLoaded <= 0)
		{
			// kick nonclients, but don't shoot at them.
			return false;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return true;
		}
	}
	else if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire())
	{
		Vector vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		//if (HasConditions(bits_COND_SEE_ENEMY))
			//vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hAssaultGrenadeSpeed) * m_hEnemy->pev->velocity;

		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5);
		
		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;
			// don't check again for a while.
			if (m_flNextAttack < gpGlobals->time)
				return false;
			m_flNextAttack = gpGlobals->time + 5; // 1/3 second.
			return true;
		}
		else
		{
			// don't check again for a while.
			m_flNextAttack = gpGlobals->time + 8; // one full second.
			return false;
		}
	}

	return false;
}

//=========================================================
// CheckRangeAttack2 - this checks the Assault's grenade
// attack. 
//=========================================================
bool CHassault :: CheckRangeAttack2 ( float flDot, float flDist )
{
	// if the Assault isn't moving, it's ok to check.
	//if ( m_flGroundSpeed != 0 )
	//{
	//	m_fThrowGrenade = false;
	//	return m_fThrowGrenade;
	//}

	if (FBitSet(pev->weapons, 1))
		return false;

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	// find target
	// vecTarget = m_hEnemy->BodyTarget( pev->origin );
	vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
	// estimate position
	if (HasConditions( bits_COND_SEE_ENEMY))
		vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 3; // one full second.
			m_fThrowGrenade = false;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 6; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		// throw a hand grenade
		m_fThrowGrenade = true;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 3; // 1/3 second.
	}
	else
	{
		// don't throw
		m_fThrowGrenade = false;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 6; // one full second.
	}
	return m_fThrowGrenade;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
bool CHassault :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return false;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return true;
	}
	return false;
}

//=========================================================
// Shoot
//=========================================================
void CHassault::Shoot(void)
{
	if( m_hEnemy == 0 )
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_12MM); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// GrenadeShoot
//=========================================================
void CHassault::GrenadeShoot(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}

	m_fThrowGrenade = false;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
	CGrenade::ShootContact(pev, vecShootOrigin, m_vecTossVelocity);

	//m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	m_cAmmoLoaded--;// take away a bullet!
	SetBlending(0, angDir.x);
}

//=========================================================
// DeathSound 
//=========================================================
void CHassault::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_IDLE);
}

//=========================================================
//=========================================================
CBaseEntity* CHassault::Kick()
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// AssaultFail
//=========================================================
Task_t tlAssaultFail[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slAssaultFail[] =
{
	{tlAssaultFail,
		ARRAYSIZE(tlAssaultFail),
		bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Assault Fail"},
};

//=========================================================
// Assault Combat Fail
//=========================================================
Task_t tlAssaultCombatFail[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slAssaultCombatFail[] =
{
	{tlAssaultCombatFail,
		ARRAYSIZE(tlAssaultCombatFail),
		bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Assault Combat Fail"},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the Assault to attack.
//=========================================================
Task_t tlAssaultEstablishLineOfFire[] =
{
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_Assault_ELOF_FAIL},
	{TASK_GET_PATH_TO_ENEMY, (float)0},
	{TASK_Assault_SPEAK_SENTENCE, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slAssaultEstablishLineOfFire[] =
{
	{tlAssaultEstablishLineOfFire,
		ARRAYSIZE(tlAssaultEstablishLineOfFire),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK2 |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssaultEstablishLineOfFire"},
};

//=========================================================
// AssaultFoundEnemy - Assault established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlAssaultFoundEnemy[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_ENEMY, (float)0},
};

Schedule_t slAssaultFoundEnemy[] =
{
	{tlAssaultFoundEnemy,
		ARRAYSIZE(tlAssaultFoundEnemy),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssaultFoundEnemy"},
};

//=========================================================
// AssaultCombatFace Schedule
//=========================================================
Task_t tlAssaultCombatFace1[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_SET_SCHEDULE, (float)SCHED_Assault_SWEEP},
};

Schedule_t slAssaultCombatFace[] =
{
	{tlAssaultCombatFace1,
		ARRAYSIZE(tlAssaultCombatFace1),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"},
};

Task_t tlAssaultSuppress[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slAssaultSuppress[] =
{
	{tlAssaultSuppress,
		ARRAYSIZE(tlAssaultSuppress),
		bits_COND_ENEMY_DEAD |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND |
			bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"},
};


//=========================================================
// Assault wait in cover - we don't allow danger or the ability
// to attack to break a Assault's run to cover schedule, but
// when a Assault is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlAssaultWaitInCover[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slAssaultWaitInCover[] =
{
	{tlAssaultWaitInCover,
		ARRAYSIZE(tlAssaultWaitInCover),
		bits_COND_NEW_ENEMY |
			bits_COND_HEAR_SOUND |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"AssaultWaitInCover"},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlAssaultTakeCoverFromBestSound[] =
{
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER}, // duck and cover if cannot move from explosion
	{TASK_STOP_MOVING, (float)0},
	{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
	{TASK_TURN_LEFT, (float)179},
};

Schedule_t slAssaultTakeCoverFromBestSound[] =
{
	{tlAssaultTakeCoverFromBestSound,
		ARRAYSIZE(tlAssaultTakeCoverFromBestSound),
		0,
		0,
		"AssaultTakeCoverFromBestSound"},
};

//=========================================================
// Assault reload schedule
//=========================================================
Task_t tlAssaultHideReload[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD},
	{TASK_FIND_COVER_FROM_ENEMY, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slAssaultHideReload[] =
{
	{tlAssaultHideReload,
		ARRAYSIZE(tlAssaultHideReload),
		bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"AssaultHideReload"} };

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlAssaultSweep[] =
{
	{TASK_TURN_LEFT, (float)179},
	{TASK_WAIT, (float)1},
	{TASK_TURN_LEFT, (float)179},
	{TASK_WAIT, (float)1},
};

Schedule_t slAssaultSweep[] =
{
	{tlAssaultSweep,
		ARRAYSIZE(tlAssaultSweep),

		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD | // sound flags
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER,

		"Assault Sweep"},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// Assault's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlAssaultRangeAttack1[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slAssaultRangeAttack1[] =
{
	{tlAssaultRangeAttack1,
		ARRAYSIZE(tlAssaultRangeAttack1),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_ENEMY_OCCLUDED |
			bits_COND_HEAR_SOUND |
			bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// Assault's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlAssaultRangeAttack2[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2},
	{TASK_SET_SCHEDULE, (float)SCHED_Assault_WAIT_FACE_ENEMY}, // don't run immediately after throwing grenade.
};

Schedule_t slAssaultRangeAttack2[] =
{
	{tlAssaultRangeAttack2,
		ARRAYSIZE(tlAssaultRangeAttack2),
		0,
		0,
		"RangeAttack2"},
};


DEFINE_CUSTOM_SCHEDULES(CHassault) {
	slAssaultFail,
		slAssaultCombatFail,
		slAssaultEstablishLineOfFire,
		slAssaultFoundEnemy,
		slAssaultCombatFace,
		slAssaultSuppress,
		slAssaultWaitInCover,
		slAssaultTakeCoverFromBestSound,
		slAssaultHideReload,
		slAssaultSweep,
		slAssaultRangeAttack1,
		slAssaultRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHassault, CSquadMonster);

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CHassault::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if ((pSound->m_iType & bits_SOUND_DANGER) != 0)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "HG_GREN", HASSAULT_SENTENCE_VOLUME, HASSAULT_ATTN, 0, 100);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = false;

				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
				{
					if (FBitSet(pev->weapons, 1))
						return GetScheduleOfType(SCHED_RANGE_ATTACK1);
					else
						return GetScheduleOfType(SCHED_Assault_SUPPRESS);
				}
				else
				{
					return GetScheduleOfType(SCHED_Assault_ESTABLISH_LINE_OF_FIRE);
				}
			}
		}
		// no ammo
		else if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo.
			// He's going to try to find cover to run to and reload, but rarely, if
			// none is available, he'll drop and reload in the open here.
			return GetScheduleOfType(SCHED_Assault_COVER_AND_RELOAD);
		}

		// damaged just a little
		else if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}
		// can kick
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// can shoot
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}
		// can grenade launch
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			// shoot a grenade if you can
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
			{
				//!!!KELLY - this Assault is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (RANDOM_LONG(0, 1))
			{
				if (FBitSet(pev->weapons, 1))
					return GetScheduleOfType(SCHED_RANGE_ATTACK1);
				else
					return GetScheduleOfType(SCHED_Assault_SUPPRESS);
			}
			else
			{
				//!!!KELLY - Assault is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// Assault's covered position. Good place for a taunt, I guess?
				return GetScheduleOfType(SCHED_CHASE_ENEMY);
			}
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (RANDOM_LONG(0, 1) && FBitSet(pev->weapons, 1))
			{
				return GetScheduleOfType(SCHED_Assault_SUPPRESS);
			}
			return GetScheduleOfType(SCHED_Assault_ESTABLISH_LINE_OF_FIRE);
		}
		return GetScheduleOfType(SCHED_CHASE_ENEMY);
	}
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHassault::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slAssaultTakeCoverFromBestSound[0];
	}
	case SCHED_Assault_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_Assault_ELOF_FAIL:
	{
		// human Assault is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_Assault_ESTABLISH_LINE_OF_FIRE:
	{
		return &slAssaultEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		return &slAssaultRangeAttack1[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slAssaultRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slAssaultCombatFace[0];
	}
	case SCHED_Assault_WAIT_FACE_ENEMY:
	{
		return &slAssaultWaitInCover[0];
	}
	case SCHED_Assault_SWEEP:
	{
		return &slAssaultSweep[0];
	}
	case SCHED_Assault_COVER_AND_RELOAD:
	{
		return &slAssaultHideReload[0];
	}
	case SCHED_Assault_FOUND_ENEMY:
	{
		return &slAssaultFoundEnemy[0];
	}
	case SCHED_Assault_SUPPRESS:
	{
		return &slAssaultSuppress[0];
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// Assault has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slAssaultCombatFail[0];
		}

		return &slAssaultFail[0];
	}
	default:
	{
		return CSquadMonster::GetScheduleOfType(Type);
	}
	}
}

//=========================================================
// start task
//=========================================================
void CHassault::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_Assault_CHECK_FIRE:
		if (!NoFriendlyFire())
		{
			// need to add smth
		}
		TaskComplete();
		break;

	case TASK_Assault_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		CSquadMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_Assault_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster::StartTask(pTask);
		break;

	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHassault::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_Assault_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw(pev->origin + m_vecTossVelocity * 64);
		ChangeYaw(pev->yaw_speed);

		if (FacingIdeal())
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	}
	default:
	{
		CSquadMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has
// started moving.
//=========================================================
void CHassault::SpeakSentence()
{
	if (m_iSentence == HASSAULT_SENT_NONE)
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz(ENT(pev), pAssaultSentences[m_iSentence], HASSAULT_SENTENCE_VOLUME, HASSAULT_ATTN, 0, 100);
		JustSpoke();
	}
}