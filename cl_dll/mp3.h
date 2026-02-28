#pragma once
// mp3 support added by Killar

#ifndef MP3_H
#define MP3_H

#include "fmod.h"
#include "fmod_errors.h"

//Fix problems with ARRAYSIZE and HSPRITE SDK definitions conflicting with Windows header definitions
//TODO: ARRAYSIZE should be replaced with std::size, which is a superior replacement
#pragma push_macro("ARRAYSIZE")
#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define HSPRITE WINDOWS_HSPRITE
#include "windows.h"
#undef HSPRITE
#pragma pop_macro("ARRAYSIZE")

class CMP3
{
private:
	float(_stdcall * VER)	(void);//AJH get fmod dll version
	signed char(_stdcall * SCL)	(FSOUND_STREAM *stream);
	signed char(_stdcall * SOP)	(int outputtype);
	signed char(_stdcall * SBS)	(int len_ms);
	signed char(_stdcall * SDRV)	(int driver);
	signed char(_stdcall * INIT)	(int mixrate, int maxsoftwarechannels, unsigned int flags);
	signed char(_stdcall * SVL)		(int channel, int vol);
	signed char(_stdcall * SCP)		(FSOUND_STREAM *stream, unsigned int ms);
	int(_stdcall * GCP)				(FSOUND_STREAM *stream);
	signed char(_stdcall * SP)		(int channel, signed char paused);
	FSOUND_STREAM*		(_stdcall * SOF)	(const char *filename, unsigned int mode, int memlength);				//AJH old fmod
	FSOUND_STREAM*		(_stdcall * SO)	(const char *filename, unsigned int mode, int offset, int memlength);	//AJH use new fmod
	int(_stdcall * SPLAY)	(int channel, FSOUND_STREAM *stream);
	void(_stdcall * CLOSE)	(void);

	FSOUND_STREAM  *m_Stream;
	HINSTANCE	m_hFMod;

public:
	int		Initialize();
	int		Shutdown();
	int		PlayMP3(const char *pszSong);
	int		StopMP3();
	void	PauseMP3(bool pause);
	bool 	Update(void);
	float	fMp3time;
	int		m_iIsPlaying;
	bool	m_bPaused;
};

extern CMP3 gMP3;
#endif

/*
#undef FMOD_ERR_CHANNEL_ALLOC
#undef FMOD_ERR_FILE_BAD
#undef FMOD_ERR_FILE_NOTFOUND
#undef FMOD_ERR_INVALID_PARAM
#undef FMOD_ERR_MEMORY
#undef FMOD_ERR_OUTPUT_FORMAT
#undef FMOD_ERR_RECORD
#undef FMOD_ERR_UNINITIALIZED
#undef FMOD_ERR_VERSION
*/