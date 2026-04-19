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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include "decals.h"
#include "soundent.h"
#include "player.h"
#include "animation.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	VC_AE_ATTACK_RIGHT			0x01
#define	VC_AE_ATTACK_LEFT			0x02
#define	ZOMBIE_AE_ATTACK_SKS_SHOOT		0x03
#define GONOME_AE_ATTACK_SLASH_FIRST		19
#define GONOME_AE_ATTACK_SLASH_SECOND	20
#define GONOME_AE_ATTACK_SLASH_THIRD		21
#define GONOME_AE_ATTACK_SLASH_FINISH	22

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CNamBullet : public CBaseEntity
{
public:
	using BaseClass = CBaseEntity;

	bool Save( CSave &save ) override;
	bool Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;

	void Touch( CBaseEntity* pOther ) override;

	void EXPORT Animate();

	static void Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );

	static CNamBullet* GonomeGutsCreate( const Vector& origin );

	void Launch( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );

	int m_maxFrame;
};

TYPEDESCRIPTION	CNamBullet::m_SaveData[] =
{
	DEFINE_FIELD( CNamBullet, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CNamBullet, CNamBullet::BaseClass );

LINK_ENTITY_TO_CLASS( nambullet, CNamBullet );

void CNamBullet::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "gonomeguts" );

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL( edict(), "models/bullet.mdl" );

	pev->frame = 0;
	//pev->scale = 0.5;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	m_maxFrame = static_cast<int>( MODEL_FRAMES( pev->modelindex ) - 1 );
}

void CNamBullet::Touch( CBaseEntity *pOther )
{
	// splat sound
	const auto iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/ric1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/bullet_hit1.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/bullet_hit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}

	if( !pOther->pev->takedamage )
	{
		TraceResult tr;
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		UTIL_BloodDrips( tr.vecEndPos, tr.vecPlaneNormal, BLOOD_COLOR_RED, 35 );
	}
	else
	{
		pOther->TakeDamage( pev, pev, gSkillData.gonomeDmgGuts, DMG_GENERIC );
	}

	SetThink( &CNamBullet::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CNamBullet::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->frame++ )
	{
		if( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void CNamBullet::Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	auto pGuts = GetClassPtr<CNamBullet>( nullptr );
	pGuts->Spawn();

	UTIL_SetOrigin( pGuts->pev, vecStart );
	pGuts->pev->velocity = vecVelocity;
	pGuts->pev->owner = ENT( pevOwner );

	if( pGuts->m_maxFrame > 0 )
	{
		pGuts->SetThink( &CNamBullet::Animate );
		pGuts->pev->nextthink = gpGlobals->time + 0.1;
	}
}

CNamBullet* CNamBullet::GonomeGutsCreate( const Vector& origin )
{
	auto pGuts = GetClassPtr<CNamBullet>( nullptr );
	pGuts->Spawn();

	pGuts->pev->origin = origin;

	return pGuts;
}

void CNamBullet::Launch( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	UTIL_SetOrigin( pev, vecStart );
	pev->velocity = vecVelocity;

	SetThink( &CNamBullet::Animate );
	pev->nextthink = gpGlobals->time + 0.1;
}

enum
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};


class CNamCharlie : public CBaseMonster
{
public:
	bool Save( CSave &save ) override;
	bool Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int IgnoreConditions () override;

	void Shoot();

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void DeathSound() override;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	bool CheckRangeAttack1 ( float flDot, float flDist ) override;
	bool CheckRangeAttack2 ( float flDot, float flDist ) override { return false; }
	bool TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) override;

	bool CheckMeleeAttack1( float flDot, float flDist ) override;

	Schedule_t* GetScheduleOfType( int Type ) override;

	//void SetActivity(Activity NewActivity);

	CUSTOM_SCHEDULES;

	float m_flNextFlinch;
	float m_flNextThrowTime;

	int m_iBrassShell;
};

TYPEDESCRIPTION	CNamCharlie::m_SaveData[] =
{
	DEFINE_FIELD( CNamCharlie, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( CNamCharlie, m_flNextThrowTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CNamCharlie, CBaseMonster );

LINK_ENTITY_TO_CLASS( monster_nam_charlie, CNamCharlie );

const char *CNamCharlie::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CNamCharlie::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

Task_t tlCharlieVictoryDance[] = 
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_WAIT, 0.2 },
	{ TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
};

Schedule_t slCharlieVictoryDance[] =
{
	{
		tlCharlieVictoryDance,
		ARRAYSIZE( tlCharlieVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_NONE,
		"CharlieVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( CNamCharlie )
{
	slCharlieVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CNamCharlie, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CNamCharlie :: Classify ()
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNamCharlie :: SetYawSpeed ()
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

bool CNamCharlie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CNamCharlie :: PainSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/alert1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/alert2.wav", 1, ATTN_IDLE);
		break;
	}
}

void CNamCharlie :: AlertSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/alert1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/alert2.wav", 1, ATTN_IDLE);
		break;
	}
}

void CNamCharlie :: IdleSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/idle1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/idle2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/idle3.wav", 1, ATTN_IDLE);
		break;
	}
}

void CNamCharlie::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/die1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/die2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "vc/die3.wav", 1, ATTN_IDLE);
		break;
	}
}

void CNamCharlie::Shoot()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sks-1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sks-2.wav", 1, ATTN_NORM);
		break;
	}

	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	CNamBullet* pBullet = CNamBullet::GonomeGutsCreate(vecShootOrigin);
	pBullet->Launch(pev, vecShootOrigin, vecShootDir * 900);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNamCharlie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case VC_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gonomeDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -9;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 25;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case VC_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gonomeDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 9;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 25;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case ZOMBIE_AE_ATTACK_SKS_SHOOT:
		case 4:
			{
				Shoot();
			}
			break;

		case GONOME_AE_ATTACK_SLASH_FIRST:
		case GONOME_AE_ATTACK_SLASH_SECOND:
		case GONOME_AE_ATTACK_SLASH_THIRD:
			{
				// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gonomeDmgOneBite, DMG_SLASH );
				if( pHurt )
				{
					if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
					{
						pHurt->pev->punchangle.x = 9;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 25;
					}
					EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pAttackHitSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 ) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
				}
				else
					EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pAttackMissSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 ) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			break;

		case GONOME_AE_ATTACK_SLASH_FINISH:
			{
				// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gonomeDmgOneBite, DMG_SLASH );
				if( pHurt )
				{
					if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
					{
						pHurt->pev->punchangle.x = 9;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 25;
					}
					EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pAttackHitSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 ) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
				}
				else
					EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pAttackMissSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 ) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CNamCharlie :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/charlie.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.pitdroneHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_flNextThrowTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNamCharlie :: Precache()
{
	int i;

	PRECACHE_MODEL("models/charlie.mdl");
	PRECACHE_MODEL( "models/bullet.mdl" );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	PRECACHE_SOUND( "vc/die1.wav" );
	PRECACHE_SOUND( "vc/die2.wav" );
	PRECACHE_SOUND( "vc/die3.wav" );

	PRECACHE_SOUND("vc/alert1.wav");
	PRECACHE_SOUND("vc/alert2.wav");

	PRECACHE_SOUND("vc/idle1.wav");
	PRECACHE_SOUND("vc/idle2.wav");
	PRECACHE_SOUND("vc/idle3.wav");

	PRECACHE_SOUND("gonome/gonome_step1.wav");
	PRECACHE_SOUND("gonome/gonome_step2.wav");
	PRECACHE_SOUND("weapons/sks-1.wav");
	PRECACHE_SOUND("weapons/sks-2.wav");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CNamCharlie::IgnoreConditions ()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( m_Activity == ACT_RANGE_ATTACK1 )
	{
		iIgnore |= bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE;
	}
	else if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
	
}

bool CNamCharlie::CheckMeleeAttack1( float flDot, float flDist )
{
	if( flDist <= 64.0 && flDot >= 0.7 && m_hEnemy )
	{
		return ( m_hEnemy->pev->flags & FL_ONGROUND ) != 0;
	}

	return false;
}

bool CNamCharlie::CheckRangeAttack1( float flDot, float flDist )
{
	return false;
	if( flDist > 64.0 && flDist <= 784.0 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime )
	{
		if( m_hEnemy )
		{
			if( IsMoving() )
			{
				m_flNextThrowTime = gpGlobals->time + 5.0;
			}
			else
			{
				m_flNextThrowTime = gpGlobals->time + 3.5;
			}
			return true;
		}
	}

	return false;
}

Schedule_t* CNamCharlie::GetScheduleOfType( int Type )
{
	if( Type == SCHED_VICTORY_DANCE )
		return slCharlieVictoryDance;
	else
		return CBaseMonster::GetScheduleOfType( Type );
}

/*
void CNamCharlie::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
			pev->frame = 0;

		pev->sequence = iSequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;
	}

	m_Activity = NewActivity;
	m_IdealActivity = NewActivity;
}
*/