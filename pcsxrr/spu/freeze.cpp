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
#include "regs.h"
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

	fp->write32le(iRVBOffset);
	fp->write32le(iRVBRepeat);
	fp->write32le(iRVBNum);
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

	fp->read32le(&iRVBOffset);
	fp->read32le(&iRVBRepeat);
	fp->read32le(&iRVBNum);
}

void SPUfreeze_new(EMUFILE* fp)
{
	//TODO - save noise seed

	const u32 tag = 0xBEEFFACE;
	fp->write32le(tag);

	fp->write32le((u32)0); //version

	fp->fwrite(spuMem,0x80000);
	fp->fwrite(regArea,0x200);
	fp->write32le(spuAddr);
	fp->write16le(spuIrq);

	for(int i=0;i<MAXCHAN;i++) SPU_core.channels[i].save(fp);

	//this is a bit odd. xa playback needs to be reengineered so its less finicky
	//and so that it uses a proper fifo which can save its state
	xa_decode_t tempxa;
	memset(&tempxa,0,sizeof(xa_decode_t));
	if(xapGlobal) tempxa = *xapGlobal;
	tempxa.save(fp);
}

bool SPUunfreeze_new(EMUFILE* fp)
{
	u32 tag = fp->read32le();
	if(tag != 0xBEEFFACE) return false;

	u32 version = fp->read32le();
	if(version != 0) return false;

	fp->fread(spuMem,0x80000);
	fp->fread(regArea,0x200);
	fp->read32le(&spuAddr);
	fp->read16le(&spuIrq);

	for(int i=0;i<MAXCHAN;i++) SPU_core.channels[i].load(fp);

	//this is WRONG. this state needs to be independent of xapGlobal.
	//not only that, our fifo isnt restored.
	xa_decode_t tempxa;
	tempxa.load(fp);
	if(xapGlobal) *xapGlobal = tempxa;

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

	return true;
	
}
