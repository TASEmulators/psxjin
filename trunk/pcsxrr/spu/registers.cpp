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
#include "regs.h"
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
	s_chan[ch].iLeftVolRaw=vol;

	if (vol&0x4000) printf("SPU: PHASE INVERT\n");

	if (vol&0x8000)                                       // sweep?
	{
		short sInc=1;                                       // -> sweep up?
		if (vol&0x2000) sInc=-1;                            // -> or down?
		if (vol&0x1000) vol^=0xffff;                        // -> mmm... phase inverted? have to investigate this
		vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
		vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
		vol*=128;
	}
	else                                                  // no sweep:
	{
		if (vol&0x4000)                                     // -> mmm... phase inverted? have to investigate this
			//vol^=0xffff;
			vol=0x3fff-(vol&0x3fff);
	}

	vol&=0x3fff;

	// store volume
	if(left) spu->channels[ch].iLeftVolume=vol;
	spu->channels[ch].iRightVolume = vol;
}

void SetPitch(SPU_struct* spu, int ch,unsigned short val)
{
	//int NP;
	//if (val>0x3fff) NP=0x3fff;                            // get pitch val
	//else           NP=val;

	//should we clamp the pitch??
	if(val>0x3fff) printf("SPU: OUT OF RANGE PITCH: %08X\n",val);
	
	spu->channels[ch].rawPitch = val;
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
			spu->channels[ch].bNoise = TRUE;
		} else spu->channels[ch].bNoise = FALSE;
			//s_chan[ch].bNoise=1;
		//else s_chan[ch].bNoise=0;
	}
}


////////////////////////////////////////////////////////////////////////
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

void SPUwriteRegister(unsigned long reg, unsigned short val)
{
	const unsigned long r=reg&0xfff;

	//cache a copy of the register for reading back.
	//we have no reason yet to doubt that this will work.
	regArea[(r-0xc00)>>1] = val;

	//channel parameters range:
	if (r>=0x0c00 && r<0x0d80)
	{
		// calc channel
		int ch=(r>>4)-0xc0;
		switch (r&0x0f)
		{
		case 0x0:
			//left volume
			SetVolume(&SPU_core, true, ch,val);
			break;
		case 0x2:
			//right volume
			SetVolume(&SPU_core, true, ch,val);
			break;
		case 0x4:
			// pitch
			SetPitch(&SPU_core, ch, val);
			break;
		case 0x6:
			// start
			SetStart(&SPU_core, ch, val);
			break;
			
		case 0x8:
		{
			// level with pre-calcs
			SPU_core.channels[ch].ADSR.AttackModeExp = (val&0x8000)?1:0;
			SPU_core.channels[ch].ADSR.AttackRate = ((val>>8) & 0x007f)^0x7f;
			SPU_core.channels[ch].ADSR.DecayRate = 4*(((val>>4) & 0x000f)^0x1f);
			SPU_core.channels[ch].ADSR.SustainLevel = (val & 0x000f) << 27;
		}
		break;
		case 0x0A:
		{
			// adsr times with pre-calcs
			SPU_core.channels[ch].ADSR.SustainModeExp = (val&0x8000)?1:0;
			SPU_core.channels[ch].ADSR.SustainIncrease= (val&0x4000)?0:1;
			SPU_core.channels[ch].ADSR.SustainRate = ((val>>6) & 0x007f)^0x7f;
			SPU_core.channels[ch].ADSR.ReleaseModeExp = (val&0x0020)?1:0;
			SPU_core.channels[ch].ADSR.ReleaseRate = 4*((val & 0x001f)^0x1f);
		}
		break;
		
		case 0x0C:
			// READS BACK current adsr volume... not sure what writing here does
			printf("SPU: WROTE TO ADSR READBACK\n");
			break;
		case 0x0E: 
			//this can be tested nicely in the ff7 intro credits screen where the harp won't work unless this gets set
			SPU_core.channels[ch].loopStartAddr = val<<3;
			if(val!=0) printf("[%02d] Loopstart set to %d\n",ch,val<<3);
			break;
		}

		return;
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
		printf("SPU: irq set to %08x spuIrq",spuIrq<<3);
		pSpuIrq=spuMemC+((unsigned long) val<<3);
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
	
	case H_SPUon1:
		SoundOn(&SPU_core,0,16,val);
		break;

	case H_SPUon2:
		SoundOn(&SPU_core,16,24,val);
		break;

	case H_SPUoff1:
		SoundOff(&SPU_core,0,16,val);
		break;

	case H_SPUoff2:
		SoundOff(&SPU_core,16,24,val);
		break;

	case H_CDLeft:
		iLeftXAVol=val  & 0x7fff;
		if (cddavCallback) cddavCallback(0,val);
		break;

	case H_CDRight:
		iRightXAVol=val & 0x7fff;
		if (cddavCallback) cddavCallback(1,val);
		break;

	case H_FMod1:
		FModOn(&SPU_core,0,16,val);
		break;

	case H_FMod2:
		FModOn(&SPU_core,16,24,val);
		break;

	case H_Noise1:
		NoiseOn(&SPU_core,0,16,val);
		break;

	case H_Noise2:
		NoiseOn(&SPU_core,16,24,val);
		break;

	case H_RVBon1:
		ReverbOn(&SPU_core,0,16,val);
		break;

	case H_RVBon2:
		ReverbOn(&SPU_core,16,24,val);
		break;

	case H_Reverb+0:

		rvb.FB_SRC_A=val;

		// OK, here's the fake REVERB stuff...
		// depending on effect we do more or less delay and repeats... bah
		// still... better than nothing :)

		SetREVERB(val);
		break;


	case H_Reverb+2   :
		rvb.FB_SRC_B=(short)val;
		break;
	case H_Reverb+4   :
		rvb.IIR_ALPHA=(short)val;
		break;
	case H_Reverb+6   :
		rvb.ACC_COEF_A=(short)val;
		break;
	case H_Reverb+8   :
		rvb.ACC_COEF_B=(short)val;
		break;
	case H_Reverb+10  :
		rvb.ACC_COEF_C=(short)val;
		break;
	case H_Reverb+12  :
		rvb.ACC_COEF_D=(short)val;
		break;
	case H_Reverb+14  :
		rvb.IIR_COEF=(short)val;
		break;
	case H_Reverb+16  :
		rvb.FB_ALPHA=(short)val;
		break;
	case H_Reverb+18  :
		rvb.FB_X=(short)val;
		break;
	case H_Reverb+20  :
		rvb.IIR_DEST_A0=(short)val;
		break;
	case H_Reverb+22  :
		rvb.IIR_DEST_A1=(short)val;
		break;
	case H_Reverb+24  :
		rvb.ACC_SRC_A0=(short)val;
		break;
	case H_Reverb+26  :
		rvb.ACC_SRC_A1=(short)val;
		break;
	case H_Reverb+28  :
		rvb.ACC_SRC_B0=(short)val;
		break;
	case H_Reverb+30  :
		rvb.ACC_SRC_B1=(short)val;
		break;
	case H_Reverb+32  :
		rvb.IIR_SRC_A0=(short)val;
		break;
	case H_Reverb+34  :
		rvb.IIR_SRC_A1=(short)val;
		break;
	case H_Reverb+36  :
		rvb.IIR_DEST_B0=(short)val;
		break;
	case H_Reverb+38  :
		rvb.IIR_DEST_B1=(short)val;
		break;
	case H_Reverb+40  :
		rvb.ACC_SRC_C0=(short)val;
		break;
	case H_Reverb+42  :
		rvb.ACC_SRC_C1=(short)val;
		break;
	case H_Reverb+44  :
		rvb.ACC_SRC_D0=(short)val;
		break;
	case H_Reverb+46  :
		rvb.ACC_SRC_D1=(short)val;
		break;
	case H_Reverb+48  :
		rvb.IIR_SRC_B1=(short)val;
		break;
	case H_Reverb+50  :
		rvb.IIR_SRC_B0=(short)val;
		break;
	case H_Reverb+52  :
		rvb.MIX_DEST_A0=(short)val;
		break;
	case H_Reverb+54  :
		rvb.MIX_DEST_A1=(short)val;
		break;
	case H_Reverb+56  :
		rvb.MIX_DEST_B0=(short)val;
		break;
	case H_Reverb+58  :
		rvb.MIX_DEST_B1=(short)val;
		break;
	case H_Reverb+60  :
		rvb.IN_COEF_L=(short)val;
		break;
	case H_Reverb+62  :
		rvb.IN_COEF_R=(short)val;
		break;
	}

}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

u16 SPUreadRegister(unsigned long reg)
{
	const unsigned long r=reg&0xfff;

	//printf("SPUreadRegister: %04X\n",r);

	//handle individual channel registers
	if(r>=0x0c00 && r<0x0d80) {
		const int ch=(r>>4)-0xc0;
		switch(r&0x0f) {
			case 0x0C: {
				//get adsr vol

				//this is the old spu logic. until proven otherwise, I don't like it.
				//if(s_chan[ch].bNew) return 1;                   // we are started, but not processed? return 1
				//if(s_chan[ch].ADSRX.lVolume &&                  // same here... we haven't decoded one sample yet, so no envelope yet. return 1 as well
				//   !s_chan[ch].ADSRX.EnvelopeVol)
				//	return 1;

				//test case: without this, beyond the beyond flute in isla village (first town) will get cut off
				//since the game will choose to cut out the flute lead sound to play other instruments.
				//with this here, the game knows that the sound isnt dead yet.

				return (u16)(((u32)SPU_core.channels[ch].ADSR.EnvelopeVol)>>16);
			}

			case 0x0E: {
				// get loop address
				printf("SPUreadRegister: reading ch %02d loop address\n");
				return SPU_core.channels[ch].loopStartAddr;
			}
		}
	} //end individual channel regs

	switch(r) {
		case H_SPUctrl:
			return spuCtrl;

		case H_SPUstat:
			return spuStat;

		case H_SPUaddr:
			return (unsigned short)(spuAddr>>3);

		case H_SPUdata: {
			unsigned short s=spuMem[spuAddr>>1];
			spuAddr+=2;
			if(spuAddr>0x7ffff) spuAddr=0;
			return s;
		}

		case H_SPUirqAddr:
			return spuIrq;

		case H_SPUon1:
			printf("Reading H_SPUon1\n");
//			return IsSoundOn(0,16);

		case H_SPUon2:
			printf("Reading H_SPUon2\n");
//			return IsSoundOn(16,24);
	}

	return regArea[(r-0xc00)>>1];
}
