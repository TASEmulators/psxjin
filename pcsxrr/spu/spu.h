/***************************************************************************
                            spu.h  -  description
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
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#ifndef _SPU_H_
#define _SPU_H_

#include "PsxCommon.h"

#define NSSIZE 1

void SetupTimer(void);
void RemoveTimer(void);
void SPUplayBRRchannel(xa_decode_t *xap);

#define CHANSTATUS_STOPPED          0
#define CHANSTATUS_PLAY             1
#define CHANSTATUS_KEYOFF             2

struct _ADSRInfo
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
};

//typedef struct
//{
//	int            AttackModeExp;
//	long           AttackTime;
//	long           DecayTime;
//	long           SustainLevel;
//	int            SustainModeExp;
//	long           SustainModeDec;
//	long           SustainTime;
//	int            ReleaseModeExp;
//	unsigned long  ReleaseVal;
//	long           ReleaseTime;
//	long           ReleaseStartTime;
//	long           ReleaseVol;
//	long           lTime;
//	long           lVolume;
//} ;


class SPU_chan
{
public:
	u32 ch;

	SPU_chan();

	void keyon();
	void keyoff();

	u8 status;
	int               iLeftVolume;                        // left volume
	int               iLeftVolRaw;                        // left psx volume value
	int               iRightVolume;                        // left volume
	int               iRightVolRaw;                        // left psx volume value

	//increment value 0000-3fff with 1000 being unit speed
	int rawPitch;
	
	u16 rawStartAddr; //= RealStartAddr = startAddr<<3, used as a blockAddress
	u32 loopStartAddr; //the loop start point, used as a blockAddress

	//stores adsr values from the software and tracks them at runtime
	_ADSRInfo ADSR;

	//runtime information never really exposed to cpu
	double smpinc; //sample frequency stepper value i.e. 2.000 equals octave+1
	double smpcnt; //current sample counter within the current block, kept under 28
	s32 blockAddress; //the current 16B encoded block address

	//are these features enabled for this channel?
	u8 bFMod,bReverb;

	//this is set if the channel starts with bReverb enabled
	//(so that changes to the reverb master enable don't affect currently running samples?)
	//(is that even right?)
	u8 bRVBActive;

	//BRR decoding and state
	s16 block[28];
	s32 s_1,s_2;
	u8 flags;
	void decodeBRR(s32* out);

	//hacky shit for the hacky reverb mode
	int               iRVBOffset;                         // reverb offset
	int               iRVBRepeat;                         // reverb repeat
	int               iRVBNum;                            // another reverb helper
};

class SPU_struct
{
public:
	SPU_struct();

	SPU_chan channels[24];
	s16 outbuf[100000];
};

extern SPU_struct SPU_core;
extern u16  spuCtrl;
extern int iUseReverb;
extern unsigned short  spuMem[256*1024];

#endif
