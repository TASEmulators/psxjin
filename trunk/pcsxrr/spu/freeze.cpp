/***************************************************************************
                          freeze.c  -  description
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
// 2004/09/18 - Pete
// - corrected LDChen ADSRX values after save state loading
//
// 2003/03/20 - Pete
// - fix to prevent the new interpolations from crashing when loading a save state
//
// 2003/01/06 - Pete
// - small changes for version 1.3 adsr save state loading
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_FREEZE

#include "externals.h"
#include "registers.h"
#include "spu.h"
#include "emufile.h"

void SPU_chan::save(EMUFILE* fp)
{
	fp->write32le((u32)1); //version

	fp->write8le(status);

	fp->write32le(iLeftVolume);
	fp->write32le(iLeftVolRaw);
	fp->write32le(iRightVolume);
	fp->write32le(iRightVolRaw);

	fp->write16le(rawPitch);

	fp->write16le(rawStartAddr);
	fp->write32le(loopStartAddr);

	ADSR.save(fp);

	fp->writedouble(smpinc);
	fp->writedouble(smpcnt);
	fp->write32le(blockAddress);

	fp->write8le(bFMod);
	fp->write8le(bReverb);
	fp->write8le(bRVBActive);

	fp->write32le(iOldNoise);
	fp->write8le(bNoise);

	for(int i=0;i<32;i++)
		fp->write16le(block[i]);
	fp->write32le(s_1);
	fp->write32le(s_2);
	fp->write8le(flags);

	fp->write32le((u32)0);
	fp->write32le((u32)0);
	fp->write32le((u32)0);
}

void SPU_chan::load(EMUFILE* fp)
{
	u32 version = fp->read32le();

	fp->read8le(&status);

	fp->read32le(&iLeftVolume);
	fp->read32le(&iLeftVolRaw);
	fp->read32le(&iRightVolume);
	fp->read32le(&iRightVolRaw);

	fp->read16le(&rawPitch);

	fp->read16le(&rawStartAddr);
	fp->read32le(&loopStartAddr);

	ADSR.load(fp);

	fp->readdouble(&smpinc);
	fp->readdouble(&smpcnt);
	fp->read32le(&blockAddress);

	fp->read8le(&bFMod);
	fp->read8le(&bReverb);
	fp->read8le(&bRVBActive);

	fp->read32le(&iOldNoise);
	fp->read8le(&bNoise);

	for(int i=0;i<28;i++)
		fp->read16le(&block[i]);
	if(version==1)
		for(int i=28;i<32;i++)
			fp->read16le(&block[i]);

	fp->read32le(&s_1);
	fp->read32le(&s_2);
	fp->read8le(&flags);

	s32 temp;
	fp->read32le(&temp);
	fp->read32le(&temp);
	fp->read32le(&temp);
}

void SPUfreeze_new(EMUFILE* fp)
{
	//TODO - save noise seed

	const u32 tag = 0xBEEFFACE;
	fp->write32le(tag);

	fp->write32le((u32)0); //version

	CTASSERT(sizeof(SPU_core->spuMem)==0x80000);
	fp->fwrite(SPU_core->spuMem,0x80000);
	fp->fwrite(regArea,0x200);
	fp->write32le(&SPU_core->spuAddr);
	fp->write16le(&SPU_core->spuCtrl);
	fp->write16le(&SPU_core->spuStat);
	fp->write16le(&SPU_core->spuIrq);

	fp->write32le(&SPU_core->mixIrqCounter);

	fp->write32le(&SPU_core->sRVBBuf[0]);
	fp->write32le(&SPU_core->sRVBBuf[1]);
	SPU_core->rvb.save(fp);

	for(int i=0;i<MAXCHAN;i++) SPU_core->channels[i].save(fp);

	SPU_core->xaqueue.freeze(fp);

	//a bit weird to do this here, but i wanted to have a more solid XA state saver
	//and the cdr freeze sucks. I made sure this saves and loads after the cdr state
	cdr.Xa.save(fp);

	const u32 endtag = 0xBAADF00D;
	fp->write32le(endtag);
}

bool SPUunfreeze_new(EMUFILE* fp)
{
	reconstruct(SPU_core,true);
	reconstruct(SPU_user,false);

	u32 tag = fp->read32le();
	if(tag != 0xBEEFFACE) return false;

	u32 version = fp->read32le();
	//if(version<3) return false;

	fp->fread(SPU_core->spuMem,0x80000);
	fp->fread(regArea,0x200);
	fp->read32le(&SPU_core->spuAddr);
	fp->read16le(&SPU_core->spuCtrl);
	fp->read16le(&SPU_core->spuStat);
	fp->read16le(&SPU_core->spuIrq);

	fp->read32le(&SPU_core->mixIrqCounter);

	fp->read32le(&SPU_core->sRVBBuf[0]);
	fp->read32le(&SPU_core->sRVBBuf[1]);
	SPU_core->rvb.load(fp);

	for(int i=0;i<MAXCHAN;i++) SPU_core->channels[i].load(fp);

	SPU_core->xaqueue.unfreeze(fp);

// repair some globals
	for (int i=0;i<=62;i+=2)
		SPUwriteRegister(H_Reverb+i,regArea[(H_Reverb+i-0xc00)>>1]);
	SPUwriteRegister(H_SPUReverbAddr,regArea[(H_SPUReverbAddr-0xc00)>>1]);
	SPUwriteRegister(H_SPUrvolL,regArea[(H_SPUrvolL-0xc00)>>1]);
	SPUwriteRegister(H_SPUrvolR,regArea[(H_SPUrvolR-0xc00)>>1]);

	SPUwriteRegister(H_SPUctrl,(unsigned short)(regArea[(H_SPUctrl-0xc00)>>1]|0x4000));
	SPUwriteRegister(H_SPUstat,regArea[(H_SPUstat-0xc00)>>1]);
	SPUwriteRegister(H_CDLeft,regArea[(H_CDLeft-0xc00)>>1]);
	SPUwriteRegister(H_CDRight,regArea[(H_CDRight-0xc00)>>1]);

// fix to prevent new interpolations from crashing
	//for (i=0;i<MAXCHAN;i++) s_chan[i].SB[28]=0;

// repair LDChen's ADSR changes
	for (int i=0;i<24;i++)
	{
		SPUwriteRegister(0x1f801c00+(i<<4)+0x08,regArea[((i<<4)+0xc00+0x08)>>1]);
		SPUwriteRegister(0x1f801c00+(i<<4)+0x0A,regArea[((i<<4)+0xc00+0x0A)>>1]);
	}

	//a bit weird to do this here, but i wanted to have a more solid XA state saver
	//and the cdr freeze sucks. I made sure this saves and loads after the cdr state
	cdr.Xa.load(fp);


	u32 endtag = fp->read32le();
	if(endtag != 0xBAADF00D) return false;

	SPUcloneUser();

	return true;
}

