/***************************************************************************
                         registers.c  -  description
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
// 2004/09/18 - LDChen
// - pre-calculated ADSRX values
//
// 2003/02/09 - kode54
// - removed &0x3fff from reverb volume registers, fixes a few games,
//   hopefully won't be breaking anything
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_REGISTERS

#include "externals.h"
#include "registers.h"
#include "reverb.h"
#include "spu.h"

/*
// adsr time values (in ms) by James Higgs ... see the end of
// the adsr.c source for details

#define ATTACK_MS     514L
#define DECAYHALF_MS  292L
#define DECAY_MS      584L
#define SUSTAIN_MS    450L
#define RELEASE_MS    446L
*/

// we have a timebase of 1.020408f ms, not 1 ms... so adjust adsr defines
#define ATTACK_MS      494L
#define DECAYHALF_MS   286L
#define DECAY_MS       572L
#define SUSTAIN_MS     441L
#define RELEASE_MS     437L


//handles the SoundOn command--each bit can keyon a channel
void SoundOn(SPU_struct *spu, int start,int end,unsigned short val)
{
	for (int ch=start;ch<end;ch++,val>>=1)
	{
		if(val&1) spu->channels[ch].keyon();

		//? dont understand this.. was it a sanity check?
		//if ((val&1) && s_chan[ch].pStart)                   // mmm... start has to be set before key on !?!
		
		//{
		//	s_chan[ch].bIgnoreLoop=0;
		//	s_chan[ch].bNew=1;
		//	dwNewChannel|=(1<<ch);                            // bitfield for faster testing
		//}
	}
}

//same as above
void SoundOff(SPU_struct *spu, int start,int end,unsigned short val) 
{
	for (int ch=start;ch<end;ch++,val>>=1)
	{
		if (val&1) spu->channels[ch].keyoff();              
			 // && s_chan[i].bOn)  mmm...
			//s_chan[ch].bStop=1;
	}
}


// please note: sweep and phase invert are wrong... but I've never seen
// them used
void SetVolume(SPU_struct* spu, bool left, unsigned char ch,short vol)            // LEFT VOLUME
{
	if (vol&0x4000) printf("SPU: PHASE INVERT (setting vol to %04X)\n",vol);

	if (vol&0x8000)                                       // sweep?
	{
		printf("SPU: VOL SWEEP\n");
		short sInc=1;                                       // -> sweep up?
		if (vol&0x2000) sInc=-1;                            // -> or down?
		if (vol&0x1000) vol^=0xffff;                        // -> mmm... phase inverted? have to investigate this
		vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
		vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
		vol*=128;
	}
	else 
	{ //no sweep:

		//phase invert: just make a negative volume
		if (vol&0x4000) 
			//vol=0x3fff-(vol&0x3fff);
			vol=-(vol&0x3fff);
		else vol = vol&0x3fff;
	}

	// store volume
	//printf("set volume %s to %04X\n",left?"left":"right",vol);
	if(left) spu->channels[ch].iLeftVolume=vol;
	else spu->channels[ch].iRightVolume = vol;
}

void SetPitch(SPU_struct* spu, int ch,unsigned short val)
{
	//should we clamp the pitch??
	//need a verification case
	if(val>0x3fff) {
		printf("[%02d] SPU: OUT OF RANGE PITCH: %08X\n",ch,val);
		val &= 0x3fff;
	}
	
	spu->channels[ch].rawPitch = val;
	spu->channels[ch].updatePitch(val);
}

static void SetStart(SPU_struct* spu, int ch, u16 val)
{
	spu->channels[ch].rawStartAddr = val;
}


void FModOn(SPU_struct* spu, int start,int end,unsigned short val)
{
	for (int ch=start;ch<end;ch++,val>>=1) 
	{
		if (val&1)                                          // -> fmod on/off
		{
			if (ch>0)
			{
				printf("[%02d] fmod on\n",ch);
				spu->channels[ch].bFMod = TRUE;
				//s_chan[ch].bFMod=1;                             // --> sound channel
				//s_chan[ch-1].bFMod=2;                           // --> freq channel
			}
		}
		else
		{
			//s_chan[ch].bFMod=0;                               // --> turn off fmod
			spu->channels[ch].bFMod = FALSE;
		}
	}
}

void ReverbOn(SPU_struct* spu, int start,int end,unsigned short val)    // REVERB ON PSX COMMAND
{
	for (int ch=start;ch<end;ch++,val>>=1)                    // loop channels
	{
		if (val&1)
			spu->channels[ch].bReverb=TRUE;
		else
			spu->channels[ch].bReverb=FALSE;
	}
}


void NoiseOn(SPU_struct* spu, int start,int end,unsigned short val)
{
	for (int ch=start;ch<end;ch++,val>>=1)
	{
		if (val&1)  {
			printf("[%02d] NOISE on\n",ch);
			spu->channels[ch].pending |= SPU_chan::NOISE_PENDING;
		} else spu->channels[ch].pending &= ~SPU_chan::NOISE_PENDING;
	}
}

void SPU_struct::writeRegister(u32 r, u16 val)
{
	//channel parameters range:
	if (r>=0x0c00 && r<0x0d80)
	{
		// calc channel
		int ch=(r>>4)-0xc0;
		switch (r&0x0f)
		{
		case 0x0:
			//left volume
			SetVolume(this, true, ch,val);
			break;
		case 0x2:
			//right volume
			SetVolume(this, false, ch,val);
			break;
		case 0x4:
			// pitch
			SetPitch(this, ch, val);
			break;
		case 0x6:
			// start
			SetStart(this, ch, val);
			break;
			
		case 0x8:
		{
			//level with pre-calcs
			channels[ch].ADSR.AttackModeExp = (val&0x8000)?1:0;
			channels[ch].ADSR.AttackRate = ((val>>8) & 0x007f)^0x7f;
			channels[ch].ADSR.DecayRate = 4*(((val>>4) & 0x000f)^0x1f);
			channels[ch].ADSR.SustainLevel = (val & 0x000f) << 27;
		}
		break;
		case 0x0A:
		{
			//adsr times with pre-calcs
			channels[ch].ADSR.SustainModeExp = (val&0x8000)?1:0;
			channels[ch].ADSR.SustainIncrease= (val&0x4000)?0:1;
			channels[ch].ADSR.SustainRate = ((val>>6) & 0x007f)^0x7f;
			channels[ch].ADSR.ReleaseModeExp = (val&0x0020)?1:0;
			channels[ch].ADSR.ReleaseRate = 4*((val & 0x001f)^0x1f);
		}
		break;
		
		case 0x0C:
			// READS BACK current adsr volume... not sure what writing here does
			printf("SPU: WROTE TO ADSR READBACK (unknown behaviour)\n");
			break;
		case 0x0E: 
			//this can be tested nicely in the ff7 intro credits screen where the harp won't work unless this gets set
			channels[ch].loopStartAddr = val<<3;
			//if(val!=0) printf("[%02d] Loopstart set to %d\n",ch,val<<3);
			break;
		}

		return;
	}

	switch(r)
	{
		case H_FMod1: FModOn(this,0,16,val); return;
		case H_FMod2: FModOn(this,16,24,val); return;
		case H_Noise1: NoiseOn(this,0,16,val); return;
		case H_Noise2: NoiseOn(this,16,24,val); return;
		case H_RVBon1: ReverbOn(this,0,16,val); return;
		case H_RVBon2: ReverbOn(this,16,24,val); return;
		case H_SPUon1: SoundOn(this,0,16,val); return;
		case H_SPUon2: SoundOn(this,16,24,val); return;
		case H_SPUoff1: SoundOff(this,0,16,val); return;
		case H_SPUoff2: SoundOff(this,16,24,val); return;
	}

	switch (r)
	{
	case H_SPUaddr:
		spuAddr = (unsigned long) val<<3;
		break;

	case H_SPUdata:
		spuMem[spuAddr>>1] = val;
		spuAddr+=2;
		if (spuAddr>0x7ffff) spuAddr=0;
		break;

	case H_SPUctrl:
		spuCtrl=val;
		break;

	case H_SPUstat:
		spuStat=val & 0xf800;
		break;

	case H_SPUReverbAddr:
		if (val==0xFFFF || val<=0x200)
		{
			rvb.StartAddr=rvb.CurrAddr=0;
		}
		else
		{
			const long iv=(unsigned long)val<<2;
			if (rvb.StartAddr!=iv)
			{
				rvb.StartAddr=(unsigned long)val<<2;
				rvb.CurrAddr=rvb.StartAddr;
			}
		}
		break;
	
	case H_SPUirqAddr:
		spuIrq = val;
		//printf("SPU: irq set to %08x spuIrq\n",((u32)spuIrq)<<3);
		break;

	case H_SPUrvolL:
		rvb.VolLeft=val;
		break;

	case H_SPUrvolR:
		rvb.VolRight=val;
		break;
		//-------------------------------------------------//

		/*
		    case H_ExtLeft:
		     //auxprintf("EL %d\n",val);
		      break;
		    //-------------------------------------------------//
		    case H_ExtRight:
		     //auxprintf("ER %d\n",val);
		      break;
		    //-------------------------------------------------//
		    case H_SPUmvolL:
		     //auxprintf("ML %d\n",val);
		      break;
		    //-------------------------------------------------//
		    case H_SPUmvolR:
		     //auxprintf("MR %d\n",val);
		      break;
		    //-------------------------------------------------//
		    case H_SPUMute1:
		     //auxprintf("M0 %04x\n",val);
		      break;
		    //-------------------------------------------------//
		    case H_SPUMute2:
		     //auxprintf("M1 %04x\n",val);
		      break;
		*/
	
	case H_CDLeft:
		iLeftXAVol=val  & 0x7fff;
		if (cddavCallback) cddavCallback(0,val);
		break;

	case H_CDRight:
		iRightXAVol=val & 0x7fff;
		if (cddavCallback) cddavCallback(1,val);
		break;


	case H_Reverb+0:
		rvb.FB_SRC_A=val;
		break;
	case H_Reverb+2:
		rvb.FB_SRC_B=(s16)val;
		break;
	case H_Reverb+4   :
		rvb.IIR_ALPHA=(s16)val;
		break;
	case H_Reverb+6   :
		rvb.ACC_COEF_A=(s16)val;
		break;
	case H_Reverb+8   :
		rvb.ACC_COEF_B=(s16)val;
		break;
	case H_Reverb+10  :
		rvb.ACC_COEF_C=(s16)val;
		break;
	case H_Reverb+12  :
		rvb.ACC_COEF_D=(s16)val;
		break;
	case H_Reverb+14  :
		rvb.IIR_COEF=(s16)val;
		break;
	case H_Reverb+16  :
		rvb.FB_ALPHA=(s16)val;
		break;
	case H_Reverb+18  :
		rvb.FB_X=(s16)val;
		break;
	case H_Reverb+20  :
		rvb.IIR_DEST_A0=(s16)val;
		break;
	case H_Reverb+22  :
		rvb.IIR_DEST_A1=(s16)val;
		break;
	case H_Reverb+24  :
		rvb.ACC_SRC_A0=(s16)val;
		break;
	case H_Reverb+26  :
		rvb.ACC_SRC_A1=(s16)val;
		break;
	case H_Reverb+28  :
		rvb.ACC_SRC_B0=(s16)val;
		break;
	case H_Reverb+30  :
		rvb.ACC_SRC_B1=(s16)val;
		break;
	case H_Reverb+32  :
		rvb.IIR_SRC_A0=(s16)val;
		break;
	case H_Reverb+34  :
		rvb.IIR_SRC_A1=(s16)val;
		break;
	case H_Reverb+36  :
		rvb.IIR_DEST_B0=(s16)val;
		break;
	case H_Reverb+38  :
		rvb.IIR_DEST_B1=(s16)val;
		break;
	case H_Reverb+40  :
		rvb.ACC_SRC_C0=(s16)val;
		break;
	case H_Reverb+42  :
		rvb.ACC_SRC_C1=(s16)val;
		break;
	case H_Reverb+44  :
		rvb.ACC_SRC_D0=(s16)val;
		break;
	case H_Reverb+46  :
		rvb.ACC_SRC_D1=(s16)val;
		break;
	case H_Reverb+48  :
		rvb.IIR_SRC_B1=(s16)val;
		break;
	case H_Reverb+50  :
		rvb.IIR_SRC_B0=(s16)val;
		break;
	case H_Reverb+52  :
		rvb.MIX_DEST_A0=(s16)val;
		break;
	case H_Reverb+54  :
		rvb.MIX_DEST_A1=(s16)val;
		break;
	case H_Reverb+56  :
		rvb.MIX_DEST_B0=(s16)val;
		break;
	case H_Reverb+58  :
		rvb.MIX_DEST_B1=(s16)val;
		break;
	case H_Reverb+60  :
		rvb.IN_COEF_L=(s16)val;
		break;
	case H_Reverb+62  :
		rvb.IN_COEF_R=(s16)val;
		break;
	}
}

//WRITE REGISTERS: called by main emu
void SPUwriteRegister(u32 reg, u16 val)
{
	const u32 r=reg&0xfff;

	//printf("write reg %08x %04x\n",r,val);

	//cache a copy of the register for reading back.
	//we have no reason yet to doubt that this will work.
	regArea[(r-0xc00)>>1] = val;

	SPU_core->writeRegister(r,val);
	if(SPU_user)
		SPU_user->writeRegister(r,val);
}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

u16 SPUreadRegister(u32 reg)
{
	const u32 r=reg&0xfff;

	//printf("read reg %08x\n",r);

	//printf("SPUreadRegister: %04X\n",r);

	//handle individual channel registers
	if(r>=0x0c00 && r<0x0d80) {
		const int ch=(r>>4)-0xc0;
		switch(r&0x0f) {
			case 0x0C: {
				//get adsr vol

				SPU_chan& chan = SPU_core->channels[ch];

				//printf("returning adsr vol %08X while status=%d\n",chan.ADSR.EnvelopeVol,chan.status);
				
				//test case: without this, various sound effects in SOTN will not get a chance to retrigger as their voices get stuck
				//(in other words, the game polls this to see whether to retrigger instead of discovering whether the channel is stopped)
				if(chan.status == CHANSTATUS_STOPPED)
					return 0;

				//test case: without this, beyond the beyond flute in isla village (first town) will get cut off
				//since the game will choose to cut out the flute lead sound to play other instruments.
				//with this here, the game knows that the sound isnt dead yet.
				u16 ret = (u16)(((u32)chan.ADSR.EnvelopeVol)>>16);
				return ret;
			}

			case 0x0E: {
				//get loop address
				printf("SPUreadRegister: reading ch %02d loop address\n");
				return SPU_core->channels[ch].loopStartAddr;
			}
		}
	} //end individual channel regs

	switch(r) {
		case H_SPUctrl:
			return SPU_core->spuCtrl;

		case H_SPUstat:
			return SPU_core->spuStat;

		case H_SPUaddr:
			return (u16)(SPU_core->spuAddr>>3);

		case H_SPUdata: {
			u16 s=SPU_core->spuMem[SPU_core->spuAddr>>1];
			SPU_core->spuAddr+=2;
			if(SPU_core->spuAddr>0x7ffff) SPU_core->spuAddr=0;
			
			//SPU_user needs to be notified of this so that subsequent writes will be in the right place
			if(SPU_user) SPU_user->spuAddr = SPU_core->spuAddr;
			return s;
		}

		case H_SPUirqAddr:
			return SPU_core->spuIrq>>3;

		case H_SPUon1:
			printf("NOTIMPLEMENTED! Reading H_SPUon1\n");
			//return IsSoundOn(0,16);
			break;

		case H_SPUon2:
			printf("NOTIMPLEMENTED! Reading H_SPUon2\n");
			//return IsSoundOn(16,24);
			break;
	}

	return regArea[(r-0xc00)>>1];
}
