/***************************************************************************
                         externals.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/04/04 - Pete
// - increased channel struct for interpolation
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#ifndef _SPU_EXTERNALS_H
#define _SPU_EXTERNALS_H

#include "PsxCommon.h"
#include "Decode_XA.h"

/////////////////////////////////////////////////////////
// generic defines
/////////////////////////////////////////////////////////

#define PSE_LT_SPU                  4
#define PSE_SPU_ERR_SUCCESS         0
#define PSE_SPU_ERR                 -60
#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR - 1
#define PSE_SPU_ERR_INIT            PSE_SPU_ERR - 2

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// sound buffer sizes
// 400 ms complete sound buffer
#define SOUNDSIZE   70560
// 137 ms test buffer... if less than that is buffered, a new upload will happen
#define TESTSIZE    24192

// num of channels
#define MAXCHAN     24

// ~ 1 ms of data
//#define NSSIZE 45

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////

// ADSR INFOS PER CHANNEL
typedef struct
{
	int            AttackModeExp;
	long           AttackTime;
	long           DecayTime;
	long           SustainLevel;
	int            SustainModeExp;
	long           SustainModeDec;
	long           SustainTime;
	int            ReleaseModeExp;
	unsigned long  ReleaseVal;
	long           ReleaseTime;
	long           ReleaseStartTime;
	long           ReleaseVol;
	long           lTime;
	long           lVolume;
} ADSRInfo;

typedef struct
{
	int            State;
	int            AttackModeExp;
	int            AttackRate;
	int            DecayRate;
	int            SustainLevel;
	int            SustainModeExp;
	int            SustainIncrease;
	int            SustainRate;
	int            ReleaseModeExp;
	int            ReleaseRate;
	int            EnvelopeVol;
	long           lVolume;
	long           lDummy1;
	long           lDummy2;
} ADSRInfoEx;

///////////////////////////////////////////////////////////

// Tmp Flags

// used for simple interpolation
#define FLAG_IPOL0 2
#define FLAG_IPOL1 4

///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////


#ifdef _WINDOWS
extern HINSTANCE hInst;
#define WM_MUTE (WM_USER+543)
#endif

///////////////////////////////////////////////////////////
// SPU.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_SPU

// psx buffers / addresses

extern unsigned short  regArea[];
extern unsigned char * spuMemC;
extern unsigned char * pSpuIrq;
extern unsigned char * pSpuBuffer;

// user settings

extern int        iUseXA;
extern int        iVolume;
extern int        iXAPitch;
extern int        iUseTimer;
extern int        iSPUIRQWait;
extern int        iRecordMode;
extern int        iUseReverb;
extern int        iUseInterpolation;

// MISC

extern unsigned long dwNoiseVal;
extern unsigned short spuCtrl;
extern unsigned short spuStat;
extern u16 spuIrq;
extern int      bEndThread;
extern int      bThreadEnded;
extern int      bSpuInit;
extern unsigned long dwNewChannel;

extern int      SSumR[];
extern int      SSumL[];
extern int      iCycle;
extern short *  pS;

extern int iSpuAsyncWait;

#ifdef _WINDOWS
extern HWND    hWMain;                               // window handle
#endif

extern void (CALLBACK *cddavCallback)(unsigned short,unsigned short);

#endif

///////////////////////////////////////////////////////////
// CFG.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_CFG

#ifndef _WINDOWS
extern char * pConfigFile;
#endif

#endif

///////////////////////////////////////////////////////////
// DSOUND.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_DSOUND

#ifdef _WINDOWS
extern unsigned long LastWrite;
extern unsigned long LastPlay;
#endif

#endif

///////////////////////////////////////////////////////////
// RECORD.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_RECORD

#ifdef _WINDOWS
extern int iDoRecord;
#endif

#endif

#endif //_SPU_EXTERNALS_H
