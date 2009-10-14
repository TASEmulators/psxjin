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
#include "Decode_XA.h"
#include "emufile.h"

#define NSSIZE 1

void SPUplayBRRchannel(xa_decode_t *xap);

#define CHANSTATUS_STOPPED          0
#define CHANSTATUS_PLAY             1
#define CHANSTATUS_KEYOFF             2

struct _ADSRInfo
{
	s32            State;
	s32            AttackModeExp;
	s32            AttackRate;
	s32            DecayRate;
	s32            SustainLevel;
	s32            SustainModeExp;
	s32            SustainIncrease;
	s32            SustainRate;
	s32            ReleaseModeExp;
	s32            ReleaseRate;
	s32            EnvelopeVol;
	s32           lVolume;
	void save(EMUFILE* fp) {
		fp->write32le((u32)0); //version
		fp->write32le(&State);
		fp->write32le(&AttackModeExp);
		fp->write32le(&AttackRate);
		fp->write32le(&DecayRate);
		fp->write32le(&SustainLevel);
		fp->write32le(&SustainModeExp);
		fp->write32le(&SustainIncrease);
		fp->write32le(&SustainRate);
		fp->write32le(&ReleaseModeExp);
		fp->write32le(&EnvelopeVol);
		fp->write32le(&lVolume);
	}
	void load(EMUFILE* fp) {
		u32 version = fp->read32le();
		fp->read32le(&State);
		fp->read32le(&AttackModeExp);
		fp->read32le(&AttackRate);
		fp->read32le(&DecayRate);
		fp->read32le(&SustainLevel);
		fp->read32le(&SustainModeExp);
		fp->read32le(&SustainIncrease);
		fp->read32le(&SustainRate);
		fp->read32le(&ReleaseModeExp);
		fp->read32le(&EnvelopeVol);
		fp->read32le(&lVolume);
	}
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

	void save(EMUFILE* fp);
	void load(EMUFILE* fp);

	void keyon();
	void keyoff();

	u8 status;
	s32               iLeftVolume;                        // left volume
	s32               iLeftVolRaw;                        // left psx volume value
	s32               iRightVolume;                        // left volume
	s32               iRightVolRaw;                        // left psx volume value

	//call this to update smpinc from the provided pitch
	void updatePitch(u16 rawPitch);

	//increment value 0000-3fff with 1000 being unit speed
	u16 rawPitch;
	
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

	//noise state
	s32 iOldNoise;
	u8 bNoise;

	//BRR decoding and state
	//note that the block is larger than the 28 sample blocksize.
	//this is a clever hack to help keep previous samples available for interpolation.
	//the final four samples of a block will get moved to 28,29,30,31 in order to wrap around
	//when interpolating using samples before sample 0 of the current block
	s16 block[32];
	s32 s_1,s_2;
	u8 flags;
	void decodeBRR(s32* out);

};

class SPU_struct
{
public:
	SPU_struct();

	u32 mixIrqCounter;

	SPU_chan channels[24];
	s16 outbuf[100000];
};

extern SPU_struct SPU_core;
extern u16  spuCtrl;
extern int iUseReverb;
extern unsigned short  spuMem[256*1024];

void SPUfreeze_new(EMUFILE* fp);
bool SPUunfreeze_new(EMUFILE* fp);

#endif
