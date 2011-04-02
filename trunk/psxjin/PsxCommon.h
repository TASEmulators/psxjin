/*  PSXjin - Pc Psx Emulator
 *  Copyright (C) 1999-2003  PSXjin Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__

#include <inttypes.h>
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32; 

typedef uint8_t uint8;  
typedef uint16_t uint16;
typedef uint32_t uint32;

#if defined(__cplusplus) || defined(c_plusplus)
#define EXTERN_C extern "C"
#define START_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#define EXTERN_C extern
#define START_EXTERN_C
#define END_EXTERN_C
#endif

#include "System.h"
#include <zlib.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include "dinput.h"


#include <windows.h>

typedef struct {
	HWND hWnd;           // Main window handle
	HINSTANCE hInstance; // Application instance
	HMENU hMenu;         // Main window menu
} AppData;

extern AppData gApp;




typedef __int8  s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;


typedef struct
{
	u32 key;
	u32 event;
} keyEvent;

typedef struct
{
	u32 keys[2][21];
} ConfigKey;


#define INLINE __inline


#define _(msgid) msgid
#define N_(msgid) msgid

extern int Log;
void __Log(char *fmt, ...);

typedef struct
{	
	int devcnt;	
	u16 padStat[2];
	int padID[2];
	int padMode1[2];
	int padMode2[2];
	int padModeE[2];
	int padModeC[2];
	int padModeF[2];
	int padVib0[2];
	int padVib1[2];
	int padVibF[2][4];
	int padVibC[2];
	DWORD padPress[2][16];
	int curPad;
	int curByte;
	int curCmd;
	int cmdLen;
} PadDef;

typedef struct {	
	int StartPosMM;
	int StartPosSS;
	int StartPosFF;
	int EndPosMM;
	int EndPosSS;
	int EndPosFF;
	char FileName[256];	
} CueStruct;
typedef struct {
	char Gpu[256];
	char Spu[256];
	char Cdr[256];
	char Pad1[256];
	char Pad2[256];
	char Net[256];
	char Mcd1[256];
	char Mcd2[256];
	char OldMcd1[256];
	char OldMcd2[256];
	char Bios[256];
	char BiosDir[256];
	char PluginsDir[256];
	char SstatesDir[256];
	char SnapDir[256];
	char MovieDir[256];
	char MemCardsDir[256];
	char Lang[256];
	long Xa;
	long Sio;
	long Mdec;
	long PsxAuto;
	long PsxType; // ntsc - 0 | pal - 1
	long QKeys;
	long Cdda;
	long HLE;
	long PsxOut;
	long RCntFix;
	long VSyncWA;
	long PauseAfterPlayback;
	char Conf_File[256];	
	long SplitAVI;
	int CurWinX;
	int CurWinY;
	bool WriteAnalog;
	bool enable_extern_analog;
	int PadLeftX;
	int PadLeftY;
	int PadRightX;
	int PadRightY;
	bool EnableAutoHold;
	bool EnableAutoFire;
	bool GetAutoHold;
	bool GetAutoFire;
	bool AutoFireFrame;
	unsigned short Pad1AutoHold;
	unsigned short Pad2AutoHold;
	unsigned short Pad1AutoFire;
	unsigned short Pad2AutoFire;
	int UsingAnalogHack;
	int CueTracks;
	CueStruct CueList[99];
	ConfigKey KeyConfig;
	PadDef PadState;
} psxjinconfig;

extern psxjinconfig Config;

extern long LoadCdBios;
extern int StatesC;
extern int cdOpenCase;
extern int cheatsEnabled;
extern int NetOpened;

typedef struct
{
	unsigned char controllerType;                           //controler type
	unsigned char padding;
	unsigned short buttonStatus;                            //normal buttons, all types use it
	// next values are in range 0-255 where 128 is center
	unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY; //for analog
	unsigned char moveX, moveY;                             //for mouse
} PadDataS;

#define MOVIE_MAX_METADATA 512
#define MOVIE_MAX_CDROM_IDS 252

struct MovieType {	
	int isText;							 //set if Movie type is Text, other wise it is binary
	bool CDSwap;						//Set if swap is going to occur
	PadDataS lastPads1[4];
	PadDataS lastPads2[4];
	char control;                        //frame control (reset, enable cheats, disable hacks, etc.)
	int16 movieFlags;                     //movie flags byte used in header
	unsigned char padType1;              //joypad1 type
	unsigned char padType2;              //joypad2 type
	unsigned long totalFrames;           //total movie frames
	unsigned long currentFrame;          //current frame in movie
	unsigned long MaxRecFrames;			 //Maximum frame hit for recording
	unsigned long lagCounter;            //current lag count
	unsigned char mode;                  //movie is | 1: recording | 2: playing | 0: not active
	unsigned char readOnly;              //movie is | 1: read-only | 0: read+write
	unsigned long rerecordCount;         //total movie rerecords
	unsigned char saveStateIncluded;     //0: no save state | 1: includes save state
	unsigned char memoryCardIncluded;    //0: no memory cards | 1: includes memory cards
	unsigned char cheatListIncluded;     //0: no cheat list | 1: includes cheat list
	unsigned char irqHacksIncluded;      //0: no irq hacks used | 1: uses irq hacks
	unsigned long saveStateOffset;       //savestate chunk location in file
	unsigned long memoryCard1Offset;     //memcard1 chunk location in file
	unsigned long memoryCard2Offset;     //memcard2 chunk location in file
	unsigned long cheatListOffset;       //cheat list chunk location in file
	unsigned long cdIdsOffset;           //cdIds chunk location in file
	unsigned long inputOffset;           //input chunk location in file
	unsigned long memoryCard1Size;       //memcard1 file size
	unsigned long memoryCard2Size;       //memcard2 file size
	unsigned long authorInfoOffset;		 //Offset to author info
	char authorInfo[MOVIE_MAX_METADATA]; //author info
	unsigned long formatVersion;         //movie file format version number
	unsigned long emuVersion;            //emulator version used in recording
	char movieFilenameMini[256];         //short movie filename (ex: "movie") used for savestates
	char movieFilename[256];             //full path file name (ex:"c:/pcsx/movies/movie.pjm")
	char bytesPerFrame;                  //size of each frame in bytes
	char palTiming;                      //PAL mode (50 FPS instead of 60)
	char currentCdrom;                   //in which CD number are we at now?
	char CdromCount;                     //how many different cds are used in the movie
	char CdromIds[MOVIE_MAX_CDROM_IDS];  //every CD ID used in the movie
	char capture;                        //0: not capturing an AVI | 1: capturing
	char aviFilename[256];               //filename used in AVI capture
	char wavFilename[256];               //filename used in WAV capture
	char startAvi;                       //start AVI capture at first emulated frame?
	char startWav;                       //start WAV capture at first emulated frame?
	unsigned long stopCapture;           //stop AVI/WAV capture at what emulated frame?
	uint8* inputBuffer;                  //full movie input buffer
	uint32 inputBufferSize;              //movie input buffer size
	uint8* inputBufferPtr;               //pointer to the full movie input buffer
	int AviCount;						 //Number of AVIs created
	char AviDrive[256];					 //Drive where avi will be stored - for splitting
	char AviDirectory[256];				 //Directory where avi will be stored - for splitting
	char AviFnameShort[256];			 //Filename where avi will be stored - for splitting
	int MultiTrack;						//Enable Disable Multitrack
	int RecordPlayer;					//Which Player are we currently Recording?
	int Port1_Mtap;						//Is player 1 a Multitap?
	int Port2_Mtap;						//Is player 2 a Multitap?
	int NumPlayers;						//Number of players in the movie
	int P2_Start;						//Where does pad2 start? 
	bool UsingAnalogHack;				//Stupid Analog Hack for Final Fantasy 8. Yes, I added a hack just for me.
	int UsingRCntFix;					//Parasite Eve Fix

};

struct MovieControlType {
	char reset;
	char cdCase;
	char sioIrq;
	char cheats;
	char RCntFix;
	char VSyncWA;
};

#define MOVIE_VERSION 2
#define PSE_PAD_TYPE_NONE 8 
#define MOVIE_FLAG_FROM_SAVESTATE (1<<1)
#define MOVIE_FLAG_PAL_TIMING     (1<<2)
#define MOVIE_FLAG_MEMORY_CARDS   (1<<3)
#define MOVIE_FLAG_CHEAT_LIST     (1<<4)
#define MOVIE_FLAG_IRQ_HACKS      (1<<5)
#define MOVIE_FLAG_IS_TEXT		  (1<<6)
#define MOVIE_FLAG_P1_MTAP		  (1<<7)
#define MOVIE_FLAG_P2_MTAP		  (1<<8)
#define MOVIE_FLAG_ANALOG_HACK	  (1<<9)
#define MOVIE_FLAG_RCNTFIX		  (1<<10)

#define MOVIE_CONTROL_RESET       (1<<1)
#define MOVIE_CONTROL_CDCASE      (1<<2)
#define MOVIE_CONTROL_SIOIRQ      (1<<3)
#define MOVIE_CONTROL_CHEATS      (1<<5)
#define MOVIE_CONTROL_RCNTFIX     (1<<6)
#define MOVIE_CONTROL_VSYNCWA     (1<<7)

#define MODE_FLAG_RECORD (1<<1)
#define MODE_FLAG_REPLAY (1<<2)
#define MODE_FLAG_PAUSED (1<<3)

enum EMOVIEMODE {
	MOVIEMODE_INACTIVE,
	MOVIEMODE_RECORD,
	MOVIEMODE_PLAY
};

enum EMUSPEED_SET
{
	EMUSPEED_SLOWEST=0,
	EMUSPEED_SLOWER,
	EMUSPEED_NORMAL,
	EMUSPEED_FASTER,
	EMUSPEED_FASTEST,
	EMUSPEED_TURBO,
	EMUSPEED_MAXIMUM
};
void SetEmulationSpeed(int cmd);

#define gzfreeze(ptr, size) \
	if (Mode == 1) fwrite(ptr, 1, size, (FILE*)f); \
	if (Mode == 0) fread(ptr, 1, size, (FILE*)f);

template<typename T> void _gzfreezel(int Mode, FILE* f, T* ptr) { gzfreeze(ptr, sizeof(T)); }
#define gzfreezel(ptr) _gzfreezel(Mode, ((FILE*)f), ptr)
#define gzfreezelarr(arr) gzfreeze(&arr[0],sizeof(arr))

char *GetSavestateFilename(int newState);

//#define BIAS	4
#define BIAS	2
#define PSXCLK	33868800	/* 33.8688 Mhz */

#include "R3000A.h"
#include "PsxMem.h"
#include "PsxHw.h"
#include "PsxBios.h"
#include "PsxDma.h"
#include "PsxCounters.h"
#include "PsxHLE.h"
#include "Mdec.h"
//#include "CdRom.h"
#include "Sio.h"
#include "spu/Spu.h"
#include "plugins.h"
//#include "Decode_XA.h"
#include "Misc.h"
#include "Debug.h"
#include "Gte.h"
#include "Movie.h"
#include "Cheat.h"
#include "LuaEngine.h"


inline u64 double_to_u64(double d) {
	union {
		u64 a;
		double b;
	} fuxor;
	fuxor.b = d;
	return fuxor.a;
}

inline double u64_to_double(u64 u) {
	union {
		u64 a;
		double b;
	} fuxor;
	fuxor.a = u;
	return fuxor.b;
}

inline u32 float_to_u32(float f) {
	union {
		u32 a;
		float b;
	} fuxor;
	fuxor.b = f;
	return fuxor.a;
}

inline float u32_to_float(u32 u) {
	union {
		u32 a;
		float b;
	} fuxor;
	fuxor.a = u;
	return fuxor.b;
}



#ifndef CTASSERT
#define	CTASSERT(x)		typedef char __assert ## y[(x) ? 1 : -1]
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

template<typename T> inline void reconstruct(T* t) { 
	t->~T();
	new(t) T();
}

template<typename T, typename A1> inline void reconstruct(T* t, const A1& a1) { 
	t->~T();
	new(t) T(a1);
}

#endif /* __PSXCOMMON_H__ */
