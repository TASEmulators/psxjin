/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
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

#include "System.h"
#if defined(__DREAMCAST__)
#include <zlib/zlib.h>
#else
#include <zlib.h>
#endif

#if defined(__WIN32__)

#include <windows.h>

typedef struct {
	HWND hWnd;           // Main window handle
	HINSTANCE hInstance; // Application instance
	HMENU hMenu;         // Main window menu
} AppData;

#elif defined (__LINUX__) || defined (__MACOSX__)

#include <sys/types.h>

#define __inline inline

#endif

#if defined (__LINUX__) || defined (__MACOSX__)
#define strnicmp strncasecmp
#endif

// Basic types
#if defined(_MSC_VER_)

typedef __int8  s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#elif defined(__LINUX__) || defined(__DREAMCAST__) || \
	  defined(__MINGW32__) || defined(__MACOSX__)

typedef char s8;
typedef short s16;
typedef long s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

#endif

#define _(msgid) msgid
#define N_(msgid) msgid

extern int Log;
void __Log(char *fmt, ...);

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
	char Lang[256];
	long Xa;
	long Sio;
	long Mdec;
	long PsxAuto;
	long PsxType; // ntsc - 0 | pal - 1
	long QKeys;
	long Cdda;
	long HLE;
	long Cpu;
	long PsxOut;
	long SpuIrq;
	long RCntFix;
	long UseNet;
	long VSyncWA;
	long Pause;
} PcsxConfig;

PcsxConfig Config;

extern long LoadCdBios;
extern int StatesC;
extern int cdOpenCase;
extern int cheatsEnabled;
extern int NetOpened;

typedef struct
{
	unsigned char controllerType;                           //controler type
	unsigned short buttonStatus;                            //normal buttons, all types use it
	// next values are in range 0-255 where 128 is center
	unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY; //for analog
	unsigned char moveX, moveY;                             //for mouse
} PadDataS;

#define MOVIE_MAX_METADATA 512
#define MOVIE_MAX_CDROM_IDS 252

struct MovieType {
	PadDataS lastPad1;                   //last joypad1 buttons polled
	PadDataS lastPad2;                   //last joypad2 buttons polled
	char control;                        //frame control (reset, enable cheats, disable hacks, etc.)
	char movieFlags;                     //movie flags byte used in header
	unsigned char padType1;              //joypad1 type
	unsigned char padType2;              //joypad2 type
	unsigned long totalFrames;           //total movie frames
	unsigned long currentFrame;          //current frame in movie
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
	char authorInfo[MOVIE_MAX_METADATA]; //author info
	unsigned long formatVersion;         //movie file format version number
	unsigned long emuVersion;            //emulator version used in recording
	char movieFilenameMini[MAX_PATH];    //short movie filename (ex: "movie") used for savestates
	char* movieFilename;                 //full path file name (ex:"c:/pcsx/movies/movie.pxm")
	char bytesPerFrame;                  //size of each frame in bytes
	char palTiming;                      //PAL mode (50 FPS instead of 60)
	char currentCdrom;                   //in which CD number are we at now?
	char CdromCount;                     //how many different cds are used in the movie
	char CdromIds[MOVIE_MAX_CDROM_IDS];  //every CD ID used in the movie
	char capture;                        //0: not capturing an AVI | 1: capturing
	char aviFilename[MAX_PATH];          //filename used in AVI capture
	char wavFilename[MAX_PATH];          //filename used in WAV capture
	char startAvi;                       //start AVI capture at first emulated frame?
	char startWav;                       //start WAV capture at first emulated frame?
	uint8* inputBuffer;                  //full movie input buffer
	uint32 inputBufferSize;              //movie input buffer size
	uint8* inputBufferPtr;               //pointer to the full movie input buffer
};

struct MovieControlType {
	char reset;
	char cdCase;
	char sioIrq;
	char spuIrq;
	char cheats;
};

#define EMU_VERSION 6
#define MOVIE_VERSION 2

#define MOVIE_FLAG_FROM_SAVESTATE (1<<1)
#define MOVIE_FLAG_PAL_TIMING     (1<<2)
#define MOVIE_FLAG_MEMORY_CARDS   (1<<3)
#define MOVIE_FLAG_CHEAT_LIST     (1<<4)
#define MOVIE_FLAG_IRQ_HACKS      (1<<5)

#define MOVIE_CONTROL_RESET       (1<<1)
#define MOVIE_CONTROL_CDCASE      (1<<2)
#define MOVIE_CONTROL_SIOIRQ      (1<<3)
#define MOVIE_CONTROL_SPUIRQ      (1<<4)
#define MOVIE_CONTROL_CHEATS      (1<<5)

#define MODE_FLAG_RECORD (1<<1)
#define MODE_FLAG_REPLAY (1<<2)
#define MODE_FLAG_PAUSED (1<<3)

#define gzfreeze(ptr, size) \
	if (Mode == 1) gzwrite(f, ptr, size); \
	if (Mode == 0) gzread(f, ptr, size);

#define gzfreezel(ptr) gzfreeze(ptr, sizeof(ptr))

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
#include "CdRom.h"
#include "Sio.h"
#include "Spu.h"
#include "plugins.h"
#include "Decode_XA.h"
#include "Misc.h"
#include "Debug.h"
#include "Gte.h"

#endif /* __PSXCOMMON_H__ */
