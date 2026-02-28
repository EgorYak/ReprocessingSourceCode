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
/*

===== h_export.cpp ========================================================

  Entity classes exported by Halflife.

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"

#undef DLLEXPORT
#ifdef WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT __attribute__((visibility("default")))
#endif

// Holds engine functionality callbacks
//enginefuncs_t g_engfuncs;
//globalvars_t  *gpGlobals;



// Half-Life: Update MOD [START]

// Reset engine function
/*
typedef void(__cdecl *CLSNDRES)(void);
CLSNDRES ClientSoundReset;

// Precache sound function - sample
typedef void(__cdecl *CLSNDPRE)(void *);
CLSNDPRE ClientSoundPrecache;

// Emit sound function - sample, edict, entindex, volume, atten, flags, pitch
typedef void(__cdecl *CLSNDEMIT)(void *, void *, int, float, float, int, int, int);
CLSNDEMIT ClientSoundEmit;

// Emit ambient function - sample, edict, entindex, volume, atten, position, flags, pitch
typedef void(__cdecl *CLSNDAMB)(void *, void *, int, float, float, float *, int, int);
CLSNDAMB ClientSoundAmbient;

// Half-Life: Update MOD [END]
*/

extern "C" void DLLEXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
}

//============================//
// NEW AUDIO ENGINE FUNCTIONS
// Thanks to: Richard Roháè
//============================//
/*
// Reset engine function
typedef void(__cdecl *CLSNDRES)(void);
extern CLSNDRES ClientSoundReset;

void EXResetEngine(void)
{
	ClientSoundReset();
};

// Precache sound function - sample
typedef void(__cdecl *CLSNDPRE)(void *);
extern CLSNDPRE ClientSoundPrecache;

void EXPrecacheSound(char *szFile)
{
	ClientSoundPrecache(szFile);
};

// Emit sound function - sample, edict, entindex, volume, atten, flags, pitch
typedef void(__cdecl *CLSNDEMIT)(void *, void *, int, float, float, int, int, int);
extern CLSNDEMIT ClientSoundEmit;

void EXEmitSound(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch)
{
	//ALERT ( at_console, "PASS -> %f\n", volume );
	ClientSoundEmit((void *)entity, (void *)sample, ENTINDEX(entity), volume, attenuation, fFlags, pitch, channel);
};

// Emit ambient function - sample, edict, entindex, volume, atten, position, flags, pitch
typedef void(__cdecl *CLSNDAMB)(void *, void *, int, float, float, float *, int, int);
extern CLSNDAMB ClientSoundAmbient;

void EXEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
	ClientSoundAmbient((void *)entity, (void *)samp, ENTINDEX(entity), vol, attenuation, pos, fFlags, pitch);
};
*/