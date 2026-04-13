#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "nodes.h"
#include "explode.h"
#include "decals.h"

#define N_SCALE 1
#define N_SPHERES 20

#define	PINGUIN_MAX_ATTACK_RADIUS		256

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PING_AE_STARTATTACK		1
#define PING_AE_THUMP			2
#define PING_AE_STARTATTACK2	3
#define PING_AE_LASER_START		4
#define PING_AE_LASER_END		5
#define PING_AE_TELEPORT		6
#define PING_AE_HEAL			7

class CPinguin : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void SetYawSpeed(void);
	void WarmUpSound(void);
	void WarmUpSound2(void);
	void AlertSound(void);
	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);
	void Killed(entvars_t* pevAttacker, int iGib);
	void StartTask(Task_t* pTask);
	void RunTask(Task_t* pTask);
	void SonicAttack(void);
	void TeleportAttack(int attachment);
	void ZapAttack(int attachment);
	void MakeFriend(Vector vecPos);
	void MakeFriends(void);
	void GoCower(void);
	void WriteBeamColor(void);
	bool EmitSphere();
	bool CheckRangeAttack1(float flDot, float flDist);
	bool CheckRangeAttack2(float flDot, float flDist);
	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule(void);
	void RunAI(void);

	void DrawChaoticBeams(int beamspr, float r, float g, float b, Vector vecOrigin, edict_t* pentIgnore, int iBeams);
	void PortalEffect(Vector vecOrigin, edict_t* pentIgnore, int sprite, float r, float g, float b, float a, int sprite2, float r2, float g2, float b2, float a2, int beamspr);

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	bool	Save(CSave& save);
	bool Restore(CRestore& restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iSpriteTexture, m_iBlow, m_ibeamTexture, m_iball, m_iballFlare;

	bool m_bCanShoot;

	int m_iTeleport;
	float m_flNextSonic;
	
private:
	static const char* pAttackSounds[];	  // vocalization: play sometimes when he launches an attack
	static const char* pBallSounds[];	  // the sound of the lightening ball launch
	static const char* pShootSounds[];	  // grunting vocalization: play sometimes when he launches an attack
	static const char* pRechargeSounds[]; // vocalization: play when he recharges
	static const char* pLaughSounds[];	  // vocalization: play sometimes when hit and still has lots of health
	static const char* pPainSounds[];	  // vocalization: play sometimes when hit and has much less health and no more chargers
	static const char* pDeathSounds[];	  // vocalization: play as he dies

	Vector m_vecDesired;
	Vector m_posDesired;

	int m_iLevel;
	float m_flShootEnd;
	float m_flShootTime;
	char m_szTeleportUse[64];
	char m_szTeleportTouch[64];
	char m_szRechargerTarget[64];
	EHANDLE m_hFriend[3];
	EHANDLE m_hRecharger;
	bool m_bFirstShoot;
	int m_iFrustration;
	
};
LINK_ENTITY_TO_CLASS(monster_kingpin, CPinguin);

TYPEDESCRIPTION	CPinguin::m_SaveData[] =
{
	DEFINE_FIELD(CPinguin, m_iSpriteTexture, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_iBlow, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_ibeamTexture, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_iball, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_iballFlare, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_flShootEnd, FIELD_TIME),
	DEFINE_FIELD(CPinguin, m_flShootTime, FIELD_TIME),
	DEFINE_FIELD(CPinguin, m_flNextSonic, FIELD_TIME),
	DEFINE_FIELD(CPinguin, m_vecDesired, FIELD_VECTOR),
	DEFINE_FIELD(CPinguin, m_iLevel, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_posDesired, FIELD_POSITION_VECTOR),
	DEFINE_ARRAY(CPinguin, m_szTeleportUse, FIELD_CHARACTER, 64),
	DEFINE_ARRAY(CPinguin, m_szTeleportTouch, FIELD_CHARACTER, 64),
	DEFINE_ARRAY(CPinguin, m_szRechargerTarget, FIELD_CHARACTER, 64),
	DEFINE_FIELD(CPinguin, m_hRecharger, FIELD_EHANDLE),
	DEFINE_ARRAY(CPinguin, m_hFriend, FIELD_EHANDLE, 3),
	DEFINE_FIELD(CPinguin, m_iTeleport, FIELD_INTEGER),
	DEFINE_FIELD(CPinguin, m_bFirstShoot, FIELD_BOOLEAN),
	DEFINE_FIELD(CPinguin, m_bCanShoot, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CPinguin, CBaseMonster);

class CKinghHVR : public CBaseMonster
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;

	void CircleInit(CBaseEntity* pTarget);
	void AbsorbInit();
	void TeleportInit(CPinguin* pOwner, CBaseEntity* pEnemy, CBaseEntity* pTarget, CBaseEntity* pTouch);
	void GreenBallInit();
	void ZapInit(CBaseEntity* pEnemy);

	void EXPORT HoverThink();
	bool CircleTarget(Vector vecTarget);
	void EXPORT DissipateThink();

	void EXPORT ZapThink();
	void EXPORT TeleportThink();
	void EXPORT TeleportTouch(CBaseEntity* pOther);

	void EXPORT RemoveTouch(CBaseEntity* pOther);
	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT ZapTouch(CBaseEntity* pOther);

	CBaseEntity* RandomClassname(const char* szName);

	// void EXPORT SphereUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void MovetoTarget(Vector vecTarget);
	virtual void Crawl();

	void Zap();
	void Teleport();

	float m_flIdealVel;
	Vector m_vecIdeal;
	CPinguin* m_pNihilanth;
	EHANDLE m_hTouch;
	int m_nFrames;
};

LINK_ENTITY_TO_CLASS(kingpin_energy_ball, CKinghHVR);


TYPEDESCRIPTION CKinghHVR::m_SaveData[] =
{
	DEFINE_FIELD(CKinghHVR, m_flIdealVel, FIELD_FLOAT),
	DEFINE_FIELD(CKinghHVR, m_vecIdeal, FIELD_VECTOR),
	DEFINE_FIELD(CKinghHVR, m_pNihilanth, FIELD_CLASSPTR),
	DEFINE_FIELD(CKinghHVR, m_hTouch, FIELD_EHANDLE),
	DEFINE_FIELD(CKinghHVR, m_nFrames, FIELD_INTEGER),
};


IMPLEMENT_SAVERESTORE(CKinghHVR, CBaseMonster);

const char* CPinguin::pAttackSounds[] =
{
	"kingpin/attack1.wav",
	"kingpin/attack2.wav",
	"kingpin/attack3.wav",
	"kingpin/attack4.wav",
};

const char* CPinguin::pBallSounds[] =
{
	"X/x_ballattack1.wav",
};

const char* CPinguin::pShootSounds[] =
{
	"X/x_shoot1.wav",
};

const char* CPinguin::pRechargeSounds[] =
{
	"kingpin/misc1.wav",
};

const char* CPinguin::pLaughSounds[] =
{
	"kingpin/pain1.wav",
	"kingpin/pain2.wav",
};

const char* CPinguin::pPainSounds[] =
{
	"kingpin/pain1.wav",
	"kingpin/pain2.wav",
};

const char* CPinguin::pDeathSounds[] =
{
	"kingpin/die.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPinguin::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// CheckRangeAttack1 - overridden for pinguin so that they
// try to get within half of their max attack radius before
// attacking, so as to increase their chances of doing damage.
//=========================================================
bool CPinguin::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > (PINGUIN_MAX_ATTACK_RADIUS) && flDot >= 0.3f && gpGlobals->time >= m_flShootTime && m_bCanShoot)
	{
		if (m_iTeleport < 4)
		{
			m_flShootTime = gpGlobals->time + 5;
			return true;
		}
		if (m_bFirstShoot)
		{
			m_flShootEnd = gpGlobals->time + 3;
			m_bFirstShoot = false;
		}
		if (gpGlobals->time > m_flShootEnd)
		{
			m_bFirstShoot = true;
			m_flShootTime = gpGlobals->time + 3;
		}
		else
		{
			m_flShootTime = gpGlobals->time + 0.3;
		}
		return true;
	}
	return false;
}

bool CPinguin::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist <= PINGUIN_MAX_ATTACK_RADIUS && flDot >= 0.3f && m_flNextSonic < gpGlobals->time)
	{
		return true;
	}
	return false;
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

bool CPinguin::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	m_iFrustration += flDamage;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPinguin::SetYawSpeed(void)
{
	pev->yaw_speed = 120;
}

void CPinguin::Killed(entvars_t* pevAttacker, int iGib)
{
	iGib = GIB_NEVER;
	pev->health = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	
	CBaseMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPinguin::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case PING_AE_STARTATTACK:
		WarmUpSound();
		break;
		
	case PING_AE_STARTATTACK2:
		WarmUpSound2();
		break;

	case PING_AE_THUMP:
		SonicAttack();
		MakeFriends();
		break;
		
	case PING_AE_LASER_START:
		TeleportAttack(2);
		break;
		
	case PING_AE_LASER_END:
		MakeFriends();
		break;

	case PING_AE_TELEPORT:
		TeleportAttack(3);
		break;

	case PING_AE_HEAL:
		/*
		if (m_hRecharger != NULL)
		{
			if (!EmitSphere())
			{
				m_hRecharger = NULL;
			}
		}
		*/
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CPinguin::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/kingpin.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;//MOVETYPE_TOSS;
	//pev->takedamage = DAMAGE_NO;
	m_bloodColor = BLOOD_COLOR_YELLOW;	//BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = gSkillData.nihilanthHealth;
	pev->yaw_speed = 120;
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	//m_flDistTooFar = 2048;
	MonsterInit();

	m_bCanShoot = true;

	m_vecDesired = Vector(1, 0, 0);
	m_posDesired = Vector(pev->origin.x, pev->origin.y, 512);

	m_iLevel = 1;
	m_iTeleport = 1;
	m_bFirstShoot = true;
	m_flShootTime = gpGlobals->time;
	m_flNextSonic = gpGlobals->time;

	m_iFrustration = 0;

	if (m_szRechargerTarget[0] == '\0')
		strcpy(m_szRechargerTarget, "n_recharger");
	if (m_szTeleportUse[0] == '\0')
		strcpy(m_szTeleportUse, "n_leaving");
	if (m_szTeleportTouch[0] == '\0')
		strcpy(m_szTeleportTouch, "n_teleport");
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPinguin::Precache()
{
	PRECACHE_MODEL("models/kingpin.mdl");	
	PRECACHE_SOUND("kingpin/alert.wav");	
	PRECACHE_SOUND("kingpin/die.wav");		
	PRECACHE_SOUND("kingpin/idle1.wav");	// клк
	PRECACHE_SOUND("kingpin/idle2.wav");	// клк
	PRECACHE_SOUND("kingpin/attack1.wav");	// клк
	PRECACHE_SOUND("kingpin/attack2.wav");	// клк
	PRECACHE_SOUND("kingpin/attack3.wav");	// клк
	PRECACHE_SOUND("kingpin/attack4.wav");	// клк
	PRECACHE_SOUND("kingpin/blast1.wav");	// клк
	PRECACHE_SOUND("kingpin/blast2.wav");	// клк
	PRECACHE_SOUND("kingpin/blast3.wav");	// клк
	PRECACHE_SOUND("kingpin/laser.wav");	// клк

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pBallSounds);
	PRECACHE_SOUND_ARRAY(pShootSounds);
	PRECACHE_SOUND_ARRAY(pRechargeSounds);
	PRECACHE_SOUND_ARRAY(pLaughSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND("debris/beamstart7.wav");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");

	m_ibeamTexture = PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/fexplo1.spr");
	PRECACHE_MODEL("sprites/xflare1.spr");

	ALLOC_STRING("sprites/lgtning.spr");
	m_iball = ALLOC_STRING("sprites/fexplo1.spr");
	m_iballFlare = ALLOC_STRING("sprites/xflare1.spr");

	m_iBlow = PRECACHE_MODEL("sprites/spore_exp_01.spr");

	UTIL_PrecacheOther("kingpin_energy_ball");
	UTIL_PrecacheOther("monster_alien_controller");
	UTIL_PrecacheOther("monster_alien_slave");
}

//=========================================================
// IdleSound
//=========================================================
void CPinguin::IdleSound(void)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "kingpin/idle1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "kingpin/idle2.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// IdleSound
//=========================================================
void CPinguin::WarmUpSound(void)
{
	if (RANDOM_LONG(0, 4) == 0)
		EMIT_SOUND(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, 0.2);

	EMIT_SOUND(edict(), CHAN_BODY, RANDOM_SOUND_ARRAY(pBallSounds), 1.0, 0.2);
}

void CPinguin::WarmUpSound2(void)
{
	if (RANDOM_LONG(0, 4) == 0)
		EMIT_SOUND(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, 0.2);

	EMIT_SOUND(edict(), CHAN_BODY, RANDOM_SOUND_ARRAY(pBallSounds), 1.0, 0.2);
}

//=========================================================
// AlertSound 
//=========================================================
void CPinguin::AlertSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "kingpin/alert.wav", 1, ATTN_NORM);
}

//=========================================================
// DeathSound 
//=========================================================
void CPinguin::DeathSound(void)
{
	EMIT_SOUND(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, 0.1);
}

//=========================================================
// PainSound 
//=========================================================
void CPinguin::PainSound(void)
{
	if (pev->health > gSkillData.nihilanthHealth / 2)
	{
		EMIT_SOUND(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pLaughSounds), 1.0, 0.2);
	}
	else
	{
		EMIT_SOUND(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, 0.2);
	}
}

//=========================================================
// WriteBeamColor - writes a color vector to the network 
// based on the size of the group. 
//=========================================================
void CPinguin::WriteBeamColor(void)
{
	WRITE_BYTE(255);
	WRITE_BYTE(128);
	WRITE_BYTE(64);
}

//=========================================================
// SonicAttack
//=========================================================
void CPinguin::SonicAttack(void)
{
	float		flAdjustedDamage;
	float		flDist;

	switch (RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "kingpin/blast1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "kingpin/blast2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "kingpin/blast3.wav", 1, ATTN_NORM);	break;
	}

	// blast circle
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 64);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 64 + (PINGUIN_MAX_ATTACK_RADIUS / 4) / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(8); // life
	WRITE_BYTE(64);  // width
	WRITE_BYTE(0);   // noise

	WriteBeamColor();

	WRITE_BYTE(255); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();

	CBaseEntity* pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, PINGUIN_MAX_ATTACK_RADIUS)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (!FClassnameIs(pEntity->pev, "monster_kingpin") && !FClassnameIs(pEntity->pev, "monster_alien_slave") && !FClassnameIs(pEntity->pev, "monster_alien_controller"))
			{
				flAdjustedDamage = gSkillData.nihilanthZap * 2.75f;
				flDist = (pEntity->Center() - pev->origin).Length();
				flAdjustedDamage -= (flDist / PINGUIN_MAX_ATTACK_RADIUS) * flAdjustedDamage;

				if (flAdjustedDamage > 0)
				{
					pEntity->TakeDamage(pev, pev, flAdjustedDamage, DMG_NERVEGAS | DMG_PARALYZE | DMG_NEVERGIB);
					if (pEntity->IsPlayer())
					{
						//((CBasePlayer*)pEntity)->ConcDamage(CONCUSS_WAVE, CONCUSS_WAVE_VAL);

						Vector org1 = pev->origin;
						Vector org2 = pEntity->pev->origin;

						org1.z = 0;
						org2.z = 0;

						Vector aim = (org1 - org2).Normalize();
						pEntity->pev->velocity = aim * -768;

						//dont jump
						pEntity->pev->flags &= ~FL_ONGROUND;
						pEntity->pev->velocity.z = 350;
					}
				}
			}
		}
	}
	m_flNextSonic = gpGlobals->time + 1.5;
}

void CPinguin::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	for (int  i = 0; i < 3; i++)
	{
		if (m_hFriend[i] != NULL && !m_hFriend[i]->IsAlive())
		{
			m_hFriend[i]->MyMonsterPointer()->pev->renderfx = kRenderFxDistort;
			m_hFriend[i]->MyMonsterPointer()->pev->renderamt = 100;
			m_hFriend[i]->MyMonsterPointer()->Killed(pev, GIB_NEVER);
			if (m_hFriend[i]->MyMonsterPointer()->m_fSequenceFinished)
				m_hFriend[i]->MyMonsterPointer()->GibMonster();
		}
	}

	if (pev->health < gSkillData.nihilanthHealth / 2)
	{
		char szName[128];

		CBaseEntity* pEnt = NULL;
		CBaseEntity* pRecharger = NULL;
		float flDist = 8192;

		sprintf(szName, "%s%d", m_szRechargerTarget, m_iLevel);

		while ((pEnt = UTIL_FindEntityByTargetname(pEnt, szName)) != NULL)
		{
			float flLocal = (pEnt->pev->origin - pev->origin).Length();
			if (flLocal < flDist)
			{
				flDist = flLocal;
				pRecharger = pEnt;
			}
		}

		if (pRecharger)
		{
			m_hRecharger = pRecharger;
			if (EmitSphere())
			{
				pev->sequence = LookupSequence("recharge");
				pev->health = gSkillData.nihilanthHealth;
				m_hRecharger = NULL;
			}
		}
		else
		{
			m_hRecharger = NULL;
			ALERT(at_aiconsole, "kingpin can't find %s\n", szName);
			m_iLevel++;
		}
	}

	if (m_iFrustration > gSkillData.nihilanthHealth / 4)
	{
		MakeFriends();
		//GoCower();
		m_iFrustration = 0;
	}

	//float flDist = (m_posDesired - pev->origin).Length();
	//float flDot = DotProduct(m_vecDesired, gpGlobals->v_forward);
}

void CPinguin::TeleportAttack(int attachment)
{
	if (m_iTeleport > 4)
	{
		ZapAttack(attachment);
		MakeFriend(pev->origin);
	}
	if (m_hEnemy != NULL)
	{
		char szText[128];

		sprintf(szText, "%s%d", m_szTeleportTouch, m_iTeleport);
		CBaseEntity* pTouch = UTIL_FindEntityByTargetname(NULL, szText);

		sprintf(szText, "%s%d", m_szTeleportUse, m_iTeleport);
		CBaseEntity* pTrigger = UTIL_FindEntityByTargetname(NULL, szText);

		if (pTrigger != NULL || pTouch != NULL)
		{
			if (m_hEnemy->IsPlayer())
				m_bCanShoot = false;
			WarmUpSound();

			Vector vecSrc, vecAngles;
			GetAttachment(attachment, vecSrc, vecAngles);
			CKinghHVR* pEntity = (CKinghHVR*)Create("kingpin_energy_ball", vecSrc, pev->angles, edict());
			pEntity->pev->velocity = pev->origin - vecSrc;
			pEntity->TeleportInit(this, m_hEnemy, pTrigger, pTouch);
		}
		else
		{
			m_iTeleport++; // unexpected failure

			WarmUpSound2();

			ALERT(at_aiconsole, "nihilanth can't target %s\n", szText);

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_ELIGHT);
			WRITE_SHORT(entindex() + 0x3000); // entity, attachment
			WRITE_COORD(pev->origin.x);		  // origin
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(256); // radius
			WRITE_BYTE(128);  // R
			WRITE_BYTE(128);  // G
			WRITE_BYTE(255);  // B
			WRITE_BYTE(10);	  // life * 10
			WRITE_COORD(128); // decay
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_ELIGHT);
			WRITE_SHORT(entindex() + 0x4000); // entity, attachment
			WRITE_COORD(pev->origin.x);		  // origin
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(256); // radius
			WRITE_BYTE(128);  // R
			WRITE_BYTE(128);  // G
			WRITE_BYTE(255);  // B
			WRITE_BYTE(10);	  // life * 10
			WRITE_COORD(128); // decay
			MESSAGE_END();
		}
	}
}

void CPinguin::ZapAttack(int attachment)
{
	if (m_hEnemy != NULL)
	{
		Vector vecSrc, vecAngles;
		GetAttachment(attachment, vecSrc, vecAngles);
		CKinghHVR* pEntity = (CKinghHVR*)Create("kingpin_energy_ball", vecSrc, pev->angles, edict());
		pEntity->pev->velocity = pev->origin - vecSrc;
		pEntity->ZapInit(m_hEnemy);
	}
}

void CPinguin::DrawChaoticBeams(int beamspr, float r, float g, float b, Vector vecOrigin, edict_t* pentIgnore, int iBeams)
{
	int iTimes = 0;
	int iDrawn = 0;
	while (iDrawn < iBeams && iTimes < (iBeams * 2))
	{
		TraceResult tr;
		const Vector vecDest = vecOrigin + ((int)(1 * 0.1f)) * (Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1)).Normalize());
		UTIL_TraceLine(vecOrigin, vecDest, ignore_monsters, pentIgnore, &tr);
		if (tr.flFraction != 1.0)
		{
			// we hit something.
			iDrawn++;
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(vecOrigin.x);
			WRITE_COORD(vecOrigin.y);
			WRITE_COORD(vecOrigin.z);
			WRITE_COORD(vecDest.x);
			WRITE_COORD(vecDest.y);
			WRITE_COORD(vecDest.z);
			WRITE_SHORT(beamspr);
			WRITE_BYTE(0); // framestart
			WRITE_BYTE(10); // framerate
			WRITE_BYTE(RANDOM_LONG(1.5f * 10, 1.5f * 10/2)); // life
			WRITE_BYTE(30);  // width
			WRITE_BYTE(65);   // noise
			WRITE_BYTE(r);   // r, g, b
			WRITE_BYTE(g);   // r, g, b
			WRITE_BYTE(b);   // r, g, b
			WRITE_BYTE(220);	// brightness
			WRITE_BYTE(35);		// speed
			MESSAGE_END();
		}
		iTimes++;
	}

	// If drew less than half of requested beams, just draw beams without respect to the walls, but with smaller radius.
	if (iDrawn < iBeams / 2)
	{
		iBeams = V_min(iBeams - iDrawn, iBeams / 2);
		for (int i = 0; i < iBeams; ++i)
		{
			Vector vecDest = vecOrigin + ((int)(1 * 0.1f)) * 0.5f * Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1));
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(vecOrigin.x);
			WRITE_COORD(vecOrigin.y);
			WRITE_COORD(vecOrigin.z);
			WRITE_COORD(vecDest.x);
			WRITE_COORD(vecDest.y);
			WRITE_COORD(vecDest.z);
			WRITE_SHORT(beamspr);
			WRITE_BYTE(0); // framestart
			WRITE_BYTE(10); // framerate
			WRITE_BYTE(RANDOM_LONG(1.5f * 10, 1.5f * 10 / 2)); // life
			WRITE_BYTE(30);  // width
			WRITE_BYTE(65);   // noise
			WRITE_BYTE(r);   // r, g, b
			WRITE_BYTE(g);   // r, g, b
			WRITE_BYTE(b);   // r, g, b
			WRITE_BYTE(220);	// brightness
			WRITE_BYTE(35);		// speed
			MESSAGE_END();
		}
	}
}

void CPinguin::PortalEffect(Vector vecOrigin, edict_t* pentIgnore, int sprite, float r, float g, float b, float a, int sprite2, float r2, float g2, float b2, float a2, int beamspr)
{
	//ALERT(at_console, "%s ,%s ,%s\n", STRING(sprite), STRING(sprite2), STRING(beamspr));
	/*
	UTIL_ScreenShake(vecOrigin, 6, 160, 1, 48);
	CSprite *pSpr = CSprite::SpriteCreate(STRING(sprite), vecOrigin, true);
	pSpr->AnimateAndDie(12);
	pSpr->SetTransparency(kRenderGlow, r, g, b, a, kRenderFxNoDissipation);
	pSpr->SetScale(1);

	CSprite *pSpr2 = CSprite::SpriteCreate(STRING(sprite2), vecOrigin, true);
	pSpr2->AnimateAndDie(12);
	pSpr2->SetTransparency(kRenderGlow, r2, g2, b2, a2, kRenderFxNoDissipation);
	pSpr2->SetScale(1);
	*/

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
	WRITE_BYTE(TE_TELEPORT);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	MESSAGE_END();

	/*
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_BYTE(0.1f);		// radius * 0.1
	WRITE_BYTE(r);
	WRITE_BYTE(g);
	WRITE_BYTE(b);
	WRITE_BYTE(a);
	WRITE_BYTE(15);		// time * 10
	WRITE_BYTE(7.5f);		// decay * 0.1
	MESSAGE_END();
	*/
	//DrawChaoticBeams(beamspr, r, g, b, vecOrigin, pentIgnore, RANDOM_LONG(2, 5));
}

void CPinguin::MakeFriend(Vector vecStart)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (m_hFriend[i] != NULL && !m_hFriend[i]->IsAlive())
		{
			if (pev->rendermode == kRenderNormal) // don't do it if they are already fading
				m_hFriend[i]->MyMonsterPointer()->FadeMonster();
			m_hFriend[i] = NULL;
		}

		if (m_hFriend[i] == NULL)
		{
			if (RANDOM_LONG(0, 1) == 0)
			{
				int iNode = WorldGraph.FindNearestNode(vecStart, bits_NODE_AIR);
				if (iNode != NO_NODE)
				{
					CNode& node = WorldGraph.Node(iNode);
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 32), node.m_vecOrigin + Vector(0, 0, 32), dont_ignore_monsters, large_hull, NULL, &tr);
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_controller", node.m_vecOrigin, pev->angles);
				}
			}
			else
			{
				int iNode = WorldGraph.FindNearestNode(vecStart, bits_NODE_LAND | bits_NODE_WATER);
				if (iNode != NO_NODE)
				{
					CNode& node = WorldGraph.Node(iNode);
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 36), node.m_vecOrigin + Vector(0, 0, 36), dont_ignore_monsters, human_hull, NULL, &tr);
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_slave", node.m_vecOrigin, pev->angles);
					PortalEffect(node.m_vecOrigin, edict(), m_iball, 200, 150, 200, 200, m_iballFlare, 250, 200, 250, 220, m_ibeamTexture);
				}
			}
			if (m_hFriend[i] != NULL)
			{
				(m_hFriend[i]->MyMonsterPointer()->pev->spawnflags &= SF_MONSTER_FADECORPSE);
				EMIT_SOUND(m_hFriend[i]->edict(), CHAN_WEAPON, "debris/beamstart7.wav", 1.0, ATTN_NORM);
			}

			return;
		}
	}
}

void CPinguin::MakeFriends(void)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (m_hFriend[i] != NULL && !m_hFriend[i]->IsAlive())
		{
			if (pev->rendermode == kRenderNormal) // don't do it if they are already fading
				m_hFriend[i]->MyMonsterPointer()->FadeMonster();
			m_hFriend[i] = NULL;
		}

		if (m_hFriend[i] == NULL)
		{
			if (RANDOM_LONG(0, 1) == 0)
			{
				int iNode = WorldGraph.FindNearestNode(pev->origin, bits_NODE_AIR);
				if (iNode != NO_NODE)
				{
					CNode& node = WorldGraph.Node(iNode);
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 32), node.m_vecOrigin + Vector(0, 0, 32), dont_ignore_monsters, large_hull, NULL, &tr);
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_controller", node.m_vecOrigin, pev->angles);
				}
			}
			else
			{
				int iNode = WorldGraph.FindNearestNode(pev->origin, bits_NODE_LAND | bits_NODE_WATER);
				if (iNode != NO_NODE)
				{
					CNode& node = WorldGraph.Node(iNode);
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 36), node.m_vecOrigin + Vector(0, 0, 36), dont_ignore_monsters, human_hull, NULL, &tr);
					if (tr.fStartSolid == 0)
						m_hFriend[i] = Create("monster_alien_slave", node.m_vecOrigin, pev->angles);
					PortalEffect(node.m_vecOrigin, edict(), m_iball, 200, 150, 200, 200, m_iballFlare, 250, 200, 250, 220, m_ibeamTexture);
				}
			}
			if (m_hFriend[i] != NULL)
			{
				EMIT_SOUND(m_hFriend[i]->edict(), CHAN_WEAPON, "debris/beamstart7.wav", 1.0, ATTN_NORM);
			}

			return;
		}
	}
}

void CPinguin::GoCower(void)
{
	if (!m_hEnemy)
		return;
	EMIT_SOUND(this->edict(), CHAN_WEAPON, "debris/beamstart7.wav", 1.0, ATTN_NORM);
	PortalEffect(pev->origin, edict(), m_iball, 200, 150, 200, 200, m_iballFlare, 250, 200, 250, 220, m_ibeamTexture);
	int iNode = WorldGraph.FindNearestNode(m_hEnemy->pev->origin, bits_NODE_LAND | bits_NODE_WATER);
	if (iNode != NO_NODE)
	{
		CNode& node = WorldGraph.Node(iNode);
		TraceResult tr;
		UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 36), node.m_vecOrigin + Vector(0, 0, 36), dont_ignore_monsters, human_hull, NULL, &tr);
		if (tr.fStartSolid == 0)
		{
			pev->origin = node.m_vecOrigin;
			PortalEffect(node.m_vecOrigin, edict(), m_iball, 200, 150, 200, 200, m_iballFlare, 250, 200, 250, 220, m_ibeamTexture);
			EMIT_SOUND(this->edict(), CHAN_WEAPON, "debris/beamstart7.wav", 1.0, ATTN_NORM);
		}
	}
}

bool CPinguin::EmitSphere()
{
	ALERT(at_console, "%d = level\n", m_iLevel);
	Vector vecSrc = m_hRecharger->pev->origin;
	CKinghHVR* pEntity = (CKinghHVR*)Create("kingpin_energy_ball", vecSrc, pev->angles, edict());
	pEntity->pev->velocity = pev->origin - vecSrc;
	pEntity->CircleInit(this);

	return true;
}

//=========================================================
// start task
//=========================================================
void CPinguin::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	
	switch (pTask->iTask)
	{
		case TASK_RANGE_ATTACK1_NOTURN:
		case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		case TASK_RANGE_ATTACK2_NOTURN:
		case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			break;
		}
		default:
		{
			ALERT(at_aiconsole, "No StartTask entry for %d\n", (SHARED_TASKS)pTask->iTask);
			break;
		}
}
	CBaseMonster::StartTask(pTask);
}

//=========================================================
// RunTask 
//=========================================================
void CPinguin::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
		case TASK_DIE:
		{
			pev->renderfx = kRenderFxDistort;
			pev->renderamt = 100;
			int i;

			for (i = 0; i < 3; i++)
			{
				if (m_hFriend[i] != NULL && m_hFriend[i]->IsAlive())
				{
					m_hFriend[i]->MyMonsterPointer()->pev->renderfx = kRenderFxDistort;
					m_hFriend[i]->MyMonsterPointer()->pev->renderamt = 100;
					m_hFriend[i]->MyMonsterPointer()->Killed(pev, GIB_NEVER);
					if (m_hFriend[i]->MyMonsterPointer()->m_fSequenceFinished)
						m_hFriend[i]->MyMonsterPointer()->GibMonster();
				}
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
					ClearMultiDamage();

					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
					WRITE_BYTE(TE_SPRITE);
					WRITE_COORD(pev->origin.x);
					WRITE_COORD(pev->origin.y);
					WRITE_COORD(pev->origin.z);
					WRITE_SHORT(m_iBlow);
					WRITE_BYTE(50);
					WRITE_BYTE(128);
					MESSAGE_END();
					::RadiusDamage(pev->origin, pev, pev, gSkillData.voltigoreDmgBeam, 160.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_SHOCK);
					TraceResult tr;
					UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
					UTIL_DecalTrace(&tr, DECAL_SPR_SPLT1 + RANDOM_LONG(0, 2));
					GibMonster();
				}
			}
			break;
		}
		case TASK_RELOAD:
		{
			if (m_fSequenceFinished)
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
		default:
		{
			CBaseMonster::RunTask(pTask);
			break;
		}
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlPingYell[] =
{
		{TASK_STOP_MOVING, 0},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Task_t	tlPingYell2[] =
{
		{TASK_STOP_MOVING, 0},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_SPECIAL_ATTACK1,		(float)0					},
};

Schedule_t slPingRangeAttack[] =
{
	{
		tlPingYell,
		ARRAYSIZE(tlPingYell),
		0,
		0,
		"PingRangeAttack1"
	},
	{
		tlPingYell2,
		ARRAYSIZE(tlPingYell2),
		0,
		0,
		"PingRangeAttack1a"
	},
};

Task_t	tlPingLaser[] =
{
		{TASK_STOP_MOVING, 0},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_RANGE_ATTACK2,		(float)0					},
};

Task_t	tlPingLaser2[] =
{
		{TASK_STOP_MOVING, 0},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_SPECIAL_ATTACK2,		(float)0					},
};

Schedule_t slPingRangeAttack2[] =
{
	{
		tlPingLaser,
		ARRAYSIZE(tlPingLaser),
		0,
		0,
		"PingRangeAttack2"
	},
	{
		tlPingLaser2,
		ARRAYSIZE(tlPingLaser2),
		0,
		0,
		"PingRangeAttack2a"
	},
};

Task_t	tlPingReload[] =
{
		{TASK_STOP_MOVING, 0},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_RELOAD,				(float)0					},
};

Schedule_t slPingReload[] =
{
	{
		tlPingReload,
		ARRAYSIZE(tlPingReload),
		0,
		0,
		"PingReload"
	},
};

DEFINE_CUSTOM_SCHEDULES(CPinguin)
{
	slPingRangeAttack,
	&slPingRangeAttack[1],
	slPingRangeAttack2,
	&slPingRangeAttack2[1],
	slPingReload,
};

IMPLEMENT_CUSTOM_SCHEDULES(CPinguin, CBaseMonster);

//=========================================================
// GetScheduleOfType 
//=========================================================
Schedule_t* CPinguin::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_IDLE_STAND:
		return CBaseMonster::GetScheduleOfType(Type);
	case SCHED_RANGE_ATTACK1:
		return &slPingRangeAttack[0];
	case SCHED_SPECIAL_ATTACK1:
		return &slPingRangeAttack[1];
	case SCHED_RANGE_ATTACK2:
		return &slPingRangeAttack2[0];
	case SCHED_SPECIAL_ATTACK2:
		return &slPingRangeAttack2[1];	
	case SCHED_RELOAD:
		return slPingReload;
	case SCHED_FAIL:
		return &slPingRangeAttack2[0];
	default:
		return CBaseMonster::GetScheduleOfType(Type);
	}
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t* CPinguin::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1) || HasConditions(bits_COND_CAN_RANGE_ATTACK2))
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return CBaseMonster::GetSchedule();
		}
		return GetScheduleOfType(SCHED_CHASE_ENEMY);
		break;
	}

	return CBaseMonster::GetSchedule();
}


//=========================================================
// Controller bouncy ball attack
//=========================================================



void CKinghHVR::Spawn()
{
	Precache();

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 3.0;
}


void CKinghHVR::Precache()
{
	PRECACHE_MODEL("sprites/flare6.spr");
	PRECACHE_MODEL("sprites/nhth1.spr");
	PRECACHE_MODEL("sprites/exit1.spr");
	PRECACHE_MODEL("sprites/tele1.spr");
	PRECACHE_MODEL("sprites/animglow01.spr");
	PRECACHE_MODEL("sprites/xspark4.spr");
	PRECACHE_MODEL("sprites/muzzleflash3.spr");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("x/x_teleattack1.wav");
}



void CKinghHVR::CircleInit(CBaseEntity* pTarget)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	// SET_MODEL(edict(), "sprites/flare6.spr");
	// pev->scale = 3.0;
	// SET_MODEL(edict(), "sprites/xspark4.spr");
	SET_MODEL(edict(), "sprites/muzzleflash3.spr");
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 128;
	pev->rendercolor.z = 64;
	pev->scale = 1.0;
	m_nFrames = 1;
	pev->renderamt = 255;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CKinghHVR::HoverThink);
	SetTouch(&CKinghHVR::BounceTouch);
	pev->nextthink = gpGlobals->time + 0.1;

	m_hTargetEnt = pTarget;
}


CBaseEntity* CKinghHVR::RandomClassname(const char* szName)
{
	int total = 0;

	CBaseEntity* pEntity = NULL;
	CBaseEntity* pNewEntity = NULL;
	while ((pNewEntity = UTIL_FindEntityByClassname(pNewEntity, szName)) != NULL)
	{
		total++;
		if (RANDOM_LONG(0, total - 1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}

void CKinghHVR::HoverThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_hTargetEnt != NULL && (m_hTargetEnt->Center() - pev->origin).Length() > 64)
	{
		MovetoTarget(m_hTargetEnt->Center());
		//CircleTarget(m_hTargetEnt->pev->origin + Vector(0, 0, 0 * N_SCALE));
	}
	else
	{
		UTIL_Remove(this);
	}


	if (RANDOM_LONG(0, 99) < 5)
	{
		/*
		CBaseEntity *pOther = RandomClassname( STRING(pev->classname) );

		if (pOther && pOther != this)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMENTS );
				WRITE_SHORT( this->entindex() );
				WRITE_SHORT( pOther->entindex() );
				WRITE_SHORT( g_sModelIndexLaser );
				WRITE_BYTE( 0 ); // framestart
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 10 ); // life
				WRITE_BYTE( 80 );  // width
				WRITE_BYTE( 80 );   // noise
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 128 );   // r, g, b
				WRITE_BYTE( 64 );   // r, g, b
				WRITE_BYTE( 255 );	// brightness
				WRITE_BYTE( 30 );		// speed
			MESSAGE_END();
		}
*/
MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMENTS );
	WRITE_SHORT( this->entindex() );
	WRITE_SHORT( m_hTargetEnt->entindex() );
	WRITE_SHORT( g_sModelIndexLaser );
	WRITE_BYTE( 0 ); // framestart
	WRITE_BYTE( 0 ); // framerate
	WRITE_BYTE( 10 ); // life
	WRITE_BYTE( 80 );  // width
	WRITE_BYTE( 80 );   // noise
	WRITE_BYTE( 255 );   // r, g, b
	WRITE_BYTE( 128 );   // r, g, b
	WRITE_BYTE( 64 );   // r, g, b
	WRITE_BYTE( 255 );	// brightness
	WRITE_BYTE( 30 );		// speed
MESSAGE_END();
	}

	pev->frame = ((int)pev->frame + 1) % m_nFrames;
}




void CKinghHVR::ZapInit(CBaseEntity* pEnemy)
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(edict(), "sprites/nhth1.spr");

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 128;
	pev->rendercolor.z = 64;
	pev->scale = 2.0;

	pev->velocity = (pEnemy->pev->origin - pev->origin).Normalize() * 200;

	m_hEnemy = pEnemy;
	SetThink(&CKinghHVR::ZapThink);
	//SetTouch(NULL);
	SetTouch(&CKinghHVR::ZapTouch);
	pev->nextthink = gpGlobals->time + 0.1;

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100);
}

void CKinghHVR::ZapThink()
{
	pev->nextthink = gpGlobals->time + 0.05;

	// check world boundaries
	if (m_hEnemy == NULL || pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 ||
		pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096 || 
		pev->velocity == g_vecZero)
	{
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	if (pev->velocity.Length() < 2000)
	{
		pev->velocity = pev->velocity * 1.2;
	}


	// MovetoTarget( m_hEnemy->Center( ) );

	if ((m_hEnemy->Center() - pev->origin).Length() < 256)
	{
		TraceResult tr;

		UTIL_TraceLine(pev->origin, m_hEnemy->Center(), dont_ignore_monsters, edict(), &tr);

		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity != NULL && 0 != pEntity->pev->takedamage)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(pev, gSkillData.nihilanthZap, pev->velocity, &tr, DMG_SHOCK | DMG_NERVEGAS);
			ApplyMultiDamage(pev, pev);
		}

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMENTPOINT);
		WRITE_SHORT(entindex());
		WRITE_COORD(tr.vecEndPos.x);
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_SHORT(g_sModelIndexLaser);
		WRITE_BYTE(0);	 // frame start
		WRITE_BYTE(10);	 // framerate
		WRITE_BYTE(3);	 // life
		WRITE_BYTE(20);	 // width
		WRITE_BYTE(20);	 // noise
		WRITE_BYTE(256);	  // R
		WRITE_BYTE(128);  // G
		WRITE_BYTE(64);	  // B
		WRITE_BYTE(255); // brightness
		WRITE_BYTE(10);	 // speed
		MESSAGE_END();

		UTIL_EmitAmbientSound(edict(), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));

		SetTouch(NULL);
		UTIL_Remove(this);
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}

	pev->frame = (int)(pev->frame + 1) % 11;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(entindex());	// entity, attachment
	WRITE_COORD(pev->origin.x); // origin
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(128); // radius
	WRITE_BYTE(256);	  // R
	WRITE_BYTE(128);  // G
	WRITE_BYTE(64);	  // B
	WRITE_BYTE(10);	  // life * 10
	WRITE_COORD(128); // decay
	MESSAGE_END();

	// Crawl( );
}


void CKinghHVR::ZapTouch(CBaseEntity* pOther)
{
	if (pOther == m_pNihilanth || FClassnameIs(pOther->pev, "monster_alien_slave") || FClassnameIs(pOther->pev, "monster_alien_controller"))
		return;
	UTIL_EmitAmbientSound(edict(), pev->origin, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(90, 95));

	RadiusDamage(pev, pev, 50, CLASS_NONE, DMG_NERVEGAS);
	pev->velocity = pev->velocity * 0;

	/*
	for (int i = 0; i < 10; i++)
	{
		Crawl( );
	}
	*/
	//m_pNihilanth->MakeFriend(pev->origin);
	SetTouch(NULL);
	UTIL_Remove(this);
	pev->nextthink = gpGlobals->time + 0.2;
}



void CKinghHVR::TeleportInit(CPinguin* pOwner, CBaseEntity* pEnemy, CBaseEntity* pTarget, CBaseEntity* pTouch)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 128;
	pev->rendercolor.z = 64;
	pev->velocity.z *= 0.2;

	SET_MODEL(edict(), "sprites/exit1.spr");

	m_pNihilanth = pOwner;
	m_hEnemy = pEnemy;
	m_hTargetEnt = pTarget;
	m_hTouch = pTouch;

	SetThink(&CKinghHVR::TeleportThink);
	SetTouch(NULL);
	//SetTouch(&CKinghHVR::TeleportTouch);
	pev->nextthink = gpGlobals->time + 0.1;

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "x/x_teleattack1.wav", 1, 0.2, 0, 100);
}


void CKinghHVR::GreenBallInit()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->scale = 1.0;

	SET_MODEL(edict(), "sprites/exit1.spr");

	SetTouch(&CKinghHVR::RemoveTouch);
}


void CKinghHVR::TeleportThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	// check world boundaries
	if (m_hEnemy == NULL || !m_hEnemy->IsAlive() || pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
		UTIL_Remove(this);
		m_pNihilanth->m_bCanShoot = true;
		return;
	}

	if ((m_hEnemy->Center() - pev->origin).Length() < 128)
	{
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
		UTIL_Remove(this);

		m_pNihilanth->m_bCanShoot = true;

		if (m_hEnemy->IsPlayer())
		{
			if (m_hTargetEnt != NULL)
			{
				m_hTargetEnt->Use(m_hEnemy, m_hEnemy, USE_ON, 1.0);
			}
			if (m_hTouch != NULL && m_hEnemy != NULL)
			{
				m_hTouch->Touch(m_hEnemy);
			}
			m_pNihilanth->m_iTeleport++;
		}
		else
		{
			m_hEnemy->pev->health = 0;
			m_hEnemy->TakeDamage(pev, pev, 1, DMG_NEVERGIB);
		}
	}
	else
	{
		MovetoTarget(m_hEnemy->Center());
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(entindex());	// entity, attachment
	WRITE_COORD(pev->origin.x); // origin
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(256); // radius
	WRITE_BYTE(256);	  // R
	WRITE_BYTE(128);  // G
	WRITE_BYTE(64);	  // B
	WRITE_BYTE(10);	  // life * 10
	WRITE_COORD(256); // decay
	MESSAGE_END();

	pev->frame = (int)(pev->frame + 1) % 20;
}


void CKinghHVR::AbsorbInit()
{
	SetThink(&CKinghHVR::DissipateThink);
	pev->renderamt = 255;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMENTS);
	WRITE_SHORT(this->entindex());
	WRITE_SHORT(m_hTargetEnt->entindex() + 0x1000);
	WRITE_SHORT(g_sModelIndexLaser);
	WRITE_BYTE(0);	 // framestart
	WRITE_BYTE(0);	 // framerate
	WRITE_BYTE(50);	 // life
	WRITE_BYTE(80);	 // width
	WRITE_BYTE(80);	 // noise
	WRITE_BYTE(255); // r, g, b
	WRITE_BYTE(128); // r, g, b
	WRITE_BYTE(64);	 // r, g, b
	WRITE_BYTE(255); // brightness
	WRITE_BYTE(30);	 // speed
	MESSAGE_END();
}

void CKinghHVR::TeleportTouch(CBaseEntity* pOther)
{
	/*
	if (pOther == m_pNihilanth)
		return;
	CBaseEntity* pEnemy = m_hEnemy;

	if (pOther == pEnemy)
	{
		if (m_hTargetEnt != NULL)
			m_hTargetEnt->Use(pEnemy, pEnemy, USE_ON, 1.0);

		if (m_hTouch != NULL && pEnemy != NULL)
			m_hTouch->Touch(pEnemy);
		m_pNihilanth->m_iTeleport++;
		m_pNihilanth->m_bIsShootTeleport = false;
		SetTouch(NULL);
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
		UTIL_Remove(this);
	}
	else
		m_pNihilanth->MakeFriend(pev->origin);
	*/

	//SetTouch(NULL);
	//STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
	//UTIL_Remove(this);
}


void CKinghHVR::DissipateThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->scale > 5.0)
		UTIL_Remove(this);

	pev->renderamt -= 2;
	pev->scale += 0.1;

	if (m_hTargetEnt != NULL)
	{
		CircleTarget(m_hTargetEnt->pev->origin + Vector(0, 0, 4096));
	}
	else
	{
		UTIL_Remove(this);
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(entindex());	// entity, attachment
	WRITE_COORD(pev->origin.x); // origin
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(pev->renderamt); // radius
	WRITE_BYTE(255);			 // R
	WRITE_BYTE(192);			 // G
	WRITE_BYTE(64);				 // B
	WRITE_BYTE(2);				 // life * 10
	WRITE_COORD(0);				 // decay
	MESSAGE_END();
}


bool CKinghHVR::CircleTarget(Vector vecTarget)
{
	bool fClose = false;

	Vector vecDest = vecTarget;
	Vector vecEst = pev->origin + pev->velocity * 0.5;
	Vector vecSrc = pev->origin;
	vecDest.z = 0;
	vecEst.z = 0;
	vecSrc.z = 0;
	float d1 = (vecDest - vecSrc).Length() - 1 * N_SCALE;
	float d2 = (vecDest - vecEst).Length() - 1 * N_SCALE;

	if (m_vecIdeal == Vector(0, 0, 0))
	{
		m_vecIdeal = pev->velocity;
	}

	if (d1 < 0 && d2 <= d1)
	{
		// ALERT( at_console, "too close\n");
		m_vecIdeal = m_vecIdeal - (vecDest - vecSrc).Normalize() * 50;
	}
	else if (d1 > 0 && d2 >= d1)
	{
		// ALERT( at_console, "too far\n");
		m_vecIdeal = m_vecIdeal + (vecDest - vecSrc).Normalize() * 50;
	}
	pev->avelocity.z = d1 * 20;

	if (d1 < 32)
	{
		fClose = true;
	}

	m_vecIdeal = m_vecIdeal + Vector(RANDOM_FLOAT(-2, 2), RANDOM_FLOAT(-2, 2), RANDOM_FLOAT(-2, 2));
	m_vecIdeal = Vector(m_vecIdeal.x, m_vecIdeal.y, 0).Normalize() * 200
		/* + Vector( -m_vecIdeal.y, m_vecIdeal.x, 0 ).Normalize( ) * 32 */
		+ Vector(0, 0, m_vecIdeal.z);
	// m_vecIdeal = m_vecIdeal + Vector( -m_vecIdeal.y, m_vecIdeal.x, 0 ).Normalize( ) * 2;

	// move up/down
	d1 = vecTarget.z - pev->origin.z;
	if (d1 > 0 && m_vecIdeal.z < 200)
		m_vecIdeal.z += 20;
	else if (d1 < 0 && m_vecIdeal.z > -200)
		m_vecIdeal.z -= 20;

	pev->velocity = m_vecIdeal;

	// ALERT( at_console, "%.0f %.0f %.0f\n", m_vecIdeal.x, m_vecIdeal.y, m_vecIdeal.z );
	return fClose;
}


void CKinghHVR::MovetoTarget(Vector vecTarget)
{
	if (m_vecIdeal == Vector(0, 0, 0))
	{
		m_vecIdeal = pev->velocity;
	}

	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed > 300)
	{
		m_vecIdeal = m_vecIdeal.Normalize() * 300;
	}
	m_vecIdeal = m_vecIdeal + (vecTarget - pev->origin).Normalize() * 300;
	pev->velocity = m_vecIdeal;
}




void CKinghHVR::Crawl()
{

	Vector vecAim = Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1)).Normalize();
	Vector vecPnt = pev->origin + pev->velocity * 0.2 + vecAim * 128;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMENTPOINT);
	WRITE_SHORT(entindex());
	WRITE_COORD(vecPnt.x);
	WRITE_COORD(vecPnt.y);
	WRITE_COORD(vecPnt.z);
	WRITE_SHORT(g_sModelIndexLaser);
	WRITE_BYTE(0);	 // frame start
	WRITE_BYTE(10);	 // framerate
	WRITE_BYTE(3);	 // life
	WRITE_BYTE(20);	 // width
	WRITE_BYTE(80);	 // noise
	WRITE_BYTE(64);	 // r, g, b
	WRITE_BYTE(128); // r, g, b
	WRITE_BYTE(255); // r, g, b
	WRITE_BYTE(255); // brightness
	WRITE_BYTE(10);	 // speed
	MESSAGE_END();
}


void CKinghHVR::RemoveTouch(CBaseEntity* pOther)
{
	STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
	UTIL_Remove(this);
}

void CKinghHVR::BounceTouch(CBaseEntity* pOther)
{
	Vector vecDir = m_vecIdeal.Normalize();

	TraceResult tr = UTIL_GetGlobalTrace();

	float n = -DotProduct(tr.vecPlaneNormal, vecDir);

	vecDir = 2.0 * tr.vecPlaneNormal * n + vecDir;

	m_vecIdeal = vecDir * m_vecIdeal.Length();

	if (pOther == m_pNihilanth)
	{
		SetTouch(NULL);
		STOP_SOUND(edict(), CHAN_WEAPON, "x/x_teleattack1.wav");
		UTIL_Remove(this);
	}
		//pOther->pev->health += gSkillData.nihilanthHealth / 4;
}