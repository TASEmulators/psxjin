//spu.cpp
//original (C) 2002 by Pete Bernert
//nearly entirely rewritten for PSXjin by zeromus
//original changelog is at end of file, for reference

//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version. See also the license.txt file for
//additional informations.                                              

//TODO - volume sweep not emulated

//-------------------------------

#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932386
#endif

#include <stdio.h>
#include <queue>

//FILE* wavout = NULL;

#define _IN_SPU

#include "externals.h"
#include "cfg.h"
#include "dsoundoss.h"
#include "record.h"
#include "resource.h"
#include "PsxCommon.h"
#include "spu.h"
#include "adsr.h"
#include "reverb.h"
#include "gauss_i.h"
#include "metaspu/metaspu.h"
#include "xa.h"
#include "movie.h"
#include "registers.h"

SPU_struct *SPU_core, *SPU_user;

//global spu values
u16 regArea[0x100]; //register cache

static ISynchronizingAudioBuffer* synchronizer = NULL;

// user settings
#define SOUND_MODE_ASYNCH 0
#define SOUND_MODE_DUAL 1
#define SOUND_MODE_SYNCH 2
int iSoundMode=SOUND_MODE_DUAL;

int iUseXA=1;
int iVolume=3;
int iXAPitch=0;
int iSynchMethod=0;
int iRecordMode=0;
int iUseReverb=1;
int iUseInterpolation=2;

int bSpuInit=0;

#ifdef _WINDOWS
HWND    hWMain=0;                                      // window handle
HWND    hWRecord=0;
static HANDLE   hMainThread;                           
#else
// 2003/06/07 - Pete
#ifndef NOTHREADLIB
static pthread_t thread = -1;                          // thread id (linux)
#endif
#endif

extern volatile int win_sound_samplecounter;
CRITICAL_SECTION win_execute_sync;
Lock::Lock() : m_cs(&win_execute_sync) { EnterCriticalSection(m_cs); }
Lock::Lock(CRITICAL_SECTION& cs) : m_cs(&cs) { EnterCriticalSection(m_cs); }
Lock::~Lock() { LeaveCriticalSection(m_cs); }

unsigned long dwNewChannel=0;                          // flags for faster testing, if new channel starts


void (CALLBACK *cddavCallback)(unsigned short,unsigned short)=0;

void irqCallback(void) {
	psxHu32ref(0x1070)|= SWAPu32(0x200);
}

INLINE int limit( int v )
{
	if( v < -32768 )
	{
		return -32768;
	}
	else if( v > 32767 )
	{
		return 32767;
	}
	return v;
}

// certain globals (were local before, but with the new timeproc I need em global)

//this is used by the BRR decoder
const int f[5][2] = 
{   {    0,  0  },
	{   60,  0  },
	{  115, -52 },
	{   98, -55 },
	{  122, -60 }
};

int iCycle=0;
short * pS;

static int lastch=-1;      // last channel processed on spu irq in timer mode
static int lastns=0;       // last ns pos
static int iSecureStart=0; // secure start counter

//--------------------------------------------------------------

void SPU_chan::updatePitch(u16 pitch)
{
	smpinc = 44100.0 / 44100.0 / 4096 * pitch;
	//smpinc = 0.078f;
	//smpinc = 1.0f;
}

void SPU_chan::keyon()
{
	//? what happens when we keyon an already on channel? just guessing that its benign
	status = CHANSTATUS_PLAY;
	
	//DO NOT DO THIS even though spu.txt made you believe it was necessary.
	//ff7 sets loop starts before keyon and this overrides it
	//loopStartAddr = 0;

	iOldNoise = 0;
	flags = 0;
	smpcnt = 28;

	updatePitch(rawPitch);
	
	blockAddress = (rawStartAddr<<3);
	
	//if(spu->isCore) printf("[%02d] Keyon at %08X with smpinc %f\n",ch,blockAddress,smpinc);

	//init interpolation state with zeros
	block[24] = block[25] = block[26] = block[27] = 0;

	//init BRR
	s_1 = s_2 = 0;

	StartADSR(this);
	StartREVERB(this);
};

void SPU_chan::keyoff()
{
	//printf("[%02d] keyoff\n",ch);
	
	//SPECULATIVE: keyoff if we're not playing? probably not
	if(status == CHANSTATUS_PLAY)
		status = CHANSTATUS_KEYOFF;
}

//---------------------------------------

//old functions handy to refer to

//INLINE void VoiceChangeFrequency(SPUCHAN * pChannel)
//{
//	pChannel->iUsedFreq=pChannel->iActFreq;               // -> take it and calc steps
//	pChannel->sinc=pChannel->iRawPitch<<4;
//	if (!pChannel->sinc) pChannel->sinc=1;
//	if (iUseInterpolation==1) pChannel->SB[32]=1;         // -> freq change in simle imterpolation mode: set flag
//}

//INLINE void FModChangeFrequency(SPUCHAN * pChannel,int ns)
//{
//	int NP=pChannel->iRawPitch;
//
//	NP=((32768L+iFMod[ns])*NP)/32768L;
//
//	if (NP>0x3fff) NP=0x3fff;
//	if (NP<0x1)    NP=0x1;
//
//	NP=(44100L*NP)/(4096L);                               // calc frequency
//
//	pChannel->iActFreq=NP;
//	pChannel->iUsedFreq=NP;
//	pChannel->sinc=(((NP/10)<<16)/4410);
//	if (!pChannel->sinc) pChannel->sinc=1;
//	if (iUseInterpolation==1) pChannel->SB[32]=1;         // freq change in simple interpolation mode
//
//	iFMod[ns]=0;
//}


//-----------------------------------------------------------------------

// noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff

INLINE int iGetNoiseVal(SPU_chan* pChannel)
{
	int fa;

	u32 &dwNoiseVal = pChannel->spu->dwNoiseVal;
	if ((dwNoiseVal<<=1)&0x80000000L)
	{
		dwNoiseVal^=0x0040001L;
		fa=((dwNoiseVal>>2)&0x7fff);
		fa=-fa;
	}
	else fa=(dwNoiseVal>>2)&0x7fff;

//// mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
	fa=pChannel->iOldNoise+((fa-pChannel->iOldNoise)/((0x001f-((pChannel->spu->spuCtrl&0x3f00)>>9))+1));
	if (fa>32767L)  fa=32767L;
	if (fa<-32767L) fa=-32767L;
	pChannel->iOldNoise=fa;

//	if (iUseInterpolation<2)                              // no gauss/cubic interpolation?
//		pChannel->SB[29] = fa;                               // -> store noise val in "current sample" slot

	return fa;


}

//these functions are an unreliable, inaccurate floor.
//it should only be used for positive numbers
//this isnt as fast as it could be if we used a visual c++ intrinsic, but those appear not to be universally available
FORCEINLINE u32 u32floor(float f)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvtt_ss2si(_mm_set_ss(f));
#else
	return (u32)f;
#endif
}
FORCEINLINE u32 u32floor(double d)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvttsd_si32(_mm_set_sd(d));
#else
	return (u32)d;
#endif
}

//same as above but works for negative values too.
//be sure that the results are the same thing as floorf!
FORCEINLINE s32 s32floor(float f)
{
#ifdef ENABLE_SSE2
	return _mm_cvtss_si32( _mm_add_ss(_mm_set_ss(-0.5f),_mm_add_ss(_mm_set_ss(f), _mm_set_ss(f))) ) >> 1;
#else
	return (s32)floorf(f);
#endif
}

static FORCEINLINE u32 sputrunc(float f) { return u32floor(f); }
static FORCEINLINE u32 sputrunc(double d) { return u32floor(d); }

enum SPUInterpolationMode
{
	SPUInterpolation_None = 0,
	SPUInterpolation_Linear = 1,
	SPUInterpolation_Gaussian = 2,
	SPUInterpolation_Cubic = 3,
	SPUInterpolation_Cosine = 4,
};

//a is the most recent sample, going back to d as the oldest
s32 _Interpolate(s16 a, s16 b, s16 c, s16 d, double _ratio)
{
	SPUInterpolationMode INTERPOLATE_MODE = (SPUInterpolationMode)iUseInterpolation;
	float ratio = (float)_ratio;

	switch(INTERPOLATE_MODE)
	{
	case SPUInterpolation_None:
		return a;
	case SPUInterpolation_Cosine:
		{
			//why did we change it away from the lookup table? somebody should research that
			ratio = ratio - (int)ratio;
			double ratio2 = ((1.0 - cos(ratio * M_PI)) * 0.5);
			//double ratio2 = (1.0f - cos_lut[((int)(ratio*256.0))&0xFF]) / 2.0f;
			return (s32)(((1-ratio2)*b) + (ratio2*a));
		}
	case SPUInterpolation_Linear:
		{
			//linear interpolation
			ratio = ratio - sputrunc(ratio);
			s32 temp = s32floor((1-ratio)*b + ratio*a);
			//printf("%d %d %d %f\n",a,b,temp,_ratio);
			return temp;
		} 
	case SPUInterpolation_Gaussian:
		{
			//I'm not sure whether I am doing this right.. 
			//low frequency things (try low notes on channel 10 of ff7 prelude song credits screen)
			//pop a little bit
			//the bit logic is taken from libopenspc
			ratio = ratio - sputrunc(ratio);
			ratio *= 256;
			s32 index = sputrunc(ratio)*4;
			s32 result = (gauss[index]*d)&~2047;
			result += (gauss[index+1]*c)&~2047;
			result += (gauss[index+2]*b)&~2047;
			result += (gauss[index+3]*a)&~2047;
			result = (result>>11)&~1;
			return result;
		}
	case SPUInterpolation_Cubic:
		{
			float y0 = d, y1 = c, y2 = b, y3 = a;
			float mu = ratio - (int)ratio;
			float mu2 = mu*mu;
			float a0 = y3 - y2 - y0 + y1;
			float a1 = y0 - y1 - a0;
			float a2 = y2 - y0;
			float a3 = y1;

			float result = a0*mu*mu2+a1*mu2+a2*mu+a3;
			return (s32)result;
		}
	default:
		return 0;
	}
}

static FORCEINLINE s32 Interpolate(s16 a, s16 b, s16 c, s16 d, double _ratio)
{
	return _Interpolate(a,b,c,d,_ratio);
}

////////////////////////////////////////////////////////////////////////

//triggers an irq if the irq address is in the specified range
void SPU_struct::triggerIrqRange(u32 base, u32 size)
{
	if(!isCore) return;

	//can't trigger an irq without the flag enabled
	if(!(spuCtrl&0x40)) return;

	u32 irqAddr = ((u32)spuIrq)<<3;
	if(base <= irqAddr && irqAddr < base+size) {
		//printf("triggering irq with args %08X %08X\n",base,size);
		irqCallback();
	}
}

SPU_chan::SPU_chan()
	: status(CHANSTATUS_STOPPED)
	, bFMod(FALSE)
	, bReverb(FALSE)
	, bNoise(FALSE)
{
}

SPU_struct::SPU_struct(bool _isCore)
: isCore(_isCore)
, iLeftXAVol(32767)
, iRightXAVol(32767)
, iReverbCycle(0)
, spuAddr(0xffffffff)
, spuCtrl(0)
, spuStat(0)
, spuIrq(0)
, mixIrqCounter(0)
, mixtime(0)
, dwNoiseVal(1)
{
	//for debugging purposes it is handy for each channel to know what index he is.
	for(int i=0;i<24;i++)
	{
		channels[i].ch = i;
		channels[i].spu = this;
	}

	memset(spuMem,0,sizeof(spuMem));
}

SPU_struct::~SPU_struct() {
}

s32 SPU_chan::decodeBRR(SPU_struct* spu)
{
	//find out which block we need and decode a new one if necessary.
	//it is safe to only check for overflow once since we can only play samples at +2 octaves (4x too fast)
	//and so as long as we are outputting at near psx resolution (44100) we can only advance through samples
	//at rates that are low enough to be safe.
	//if we permitted 11025khz output the story would be different.

restart:

	//printf("%d %f\n",ch,smpcnt);
	int sampnum = (int)smpcnt;
	if(sampnum>=28)
	{
		//end or loop
		if(flags&1)
		{
			//there seems to be a special condition which triggers the irq if the loop address equals the irq address
			//(even if this sample is stopping)
			//either that, or the hardware always reads the block referenced by loop target even if the sample is ending.
			//thousand arms will need this in order to trigger the very first voiceover
			//should this be 1 or 16? equality or range?
			spu->triggerIrqRange(loopStartAddr,16);

			//when the old spu would kill a sound this way, it was immediate.
			//maybe adsr release is only for keyoff
			if(flags != 3) {
				status = CHANSTATUS_STOPPED;
				return 0;
			}
			else {
				//printf("[%02d] looping to %d\n",ch,loopStartAddr);
				blockAddress = loopStartAddr;
				flags = 0;
				goto restart;
			}
		}

		//do not do this before "end or loop" or else looping will glitch as it fails to
		//immediately decode a block.
		//why did I ever try this? I can't remember.
		smpcnt -= 28;

		sampnum = (int)smpcnt;

		//this will be tested by the impressive valkyrie profile new game sound
		//which is large and apparently streams in
		spu->triggerIrqRange(blockAddress,16);

		u8 header0 = spu->readSpuMem(blockAddress);
		
		flags = spu->readSpuMem(blockAddress+1);

		//this flag bit means that we are setting the loop start point
		if(flags&4) {
			loopStartAddr = blockAddress;
			//printf("[%02d] embedded loop addr set to %d\n",ch,loopStartAddr);
		}

		blockAddress+=2;

		s32 predict_nr = header0;
		s32 shift_factor = predict_nr&0xf;
		predict_nr >>= 4;

		//before we decode a new block, save the last 4 values of the old block
		block[28] = block[24];
		block[29] = block[25];
		block[30] = block[26];
		block[31] = block[27];

		//decode 
		for(int i=0,j=0;i<14;i++)
		{
			s32 d = spu->readSpuMem(blockAddress++);
			s32 s = (d&0xf)<<12;
			if(s&0x8000) s|=0xffff0000;
			s32 fa = (s>>shift_factor);
			fa = fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
			s_2 = s_1;
			s_1 = fa;
			
			block[j++] = fa;

			s = (d&0xf0)<<8;
			if(s&0x8000) s|=0xffff0000;

			fa = (s>>shift_factor);
			fa = fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
			s_2 = s_1;
			s_1 = fa;

			block[j++] = fa;
		}
	}

	//perform interpolation. hardcoded for now
	//if(GetAsyncKeyState('I'))
	//it is safe to only check for overflow once since we can only play samples at +2 octaves (4x too fast)
	if(true)
	{
		s16 a = block[sampnum&31];
		s16 b = block[(sampnum-1)&31];
		s16 c = block[(sampnum-2)&31];
		s16 d = block[(sampnum-3)&31];
		return Interpolate(a,b,c,d,smpcnt);
	}
	else return block[sampnum];

	//printf("%d\n",*out);
}


void mixAudio(bool killReverb, SPU_struct* spu, int length)
{
	memset(spu->outbuf, 0, length*4*2);

	//(todo - analyze master volumes etc.)

	bool checklog[24]; for(int i=0;i<24;i++) checklog[i] = false;

	for(int j=0;j<length;j++)
	{
		s32 left_accum = 0, right_accum = 0;
		s32 fmod;

		spu->REVERB_initSample();

		for(int i=0;i<24;i++)
		{
			SPU_chan *chan = &spu->channels[i];

			if (chan->status == CHANSTATUS_STOPPED) continue;

			s32 samp;
			if(chan->bNoise)
				samp = iGetNoiseVal(chan);
			else
				samp = chan->decodeBRR(spu);

			//channel may have ended at any time (from the BRR decode or from a previous envelope calculation
			if (chan->status == CHANSTATUS_STOPPED) {
				fmod = 0;
				continue;
			}

			//since we tick the envelope when we keyon, we need to retrieve that value here.
			s32 adsrLevel = chan->ADSR.lVolume;

			//and now immediately afterwards tick it again
			MixADSR(chan);


	
			checklog[i] = true;

			samp = samp * adsrLevel/1023;

			//apply the modulation
			if(chan->bFMod)
			{
				//this was a little hard to test. ff7 battle fx were using it, but 
				//its hard to tell since I dont think our noise is very good. need a better test.
				chan->smpcnt += chan->smpinc;
				s32 pitch = chan->rawPitch;
				pitch = ((32768L+fmod)*pitch)/32768L;
				if (pitch>0x3fff) pitch=0x3fff;
				if (pitch<0x1)    pitch=0x1;
				chan->updatePitch((u16)pitch);
			
				//just some diagnostics
				//static u16 lastPitch=0xFFFF, lastRawPitch=0xFFFF; if(chan->rawPitch != lastRawPitch || lastPitch != pitch) printf("fmod bending pitch from %04X to %04X\n",lastRawPitch=chan->rawPitch,lastPitch=pitch);
				//printf("fmod: %d\n",fmod);
			}

			//don't output this channel; it is used to modulate the next channel
			if(i<23 && spu->channels[i+1].bFMod)
			{
				//should this be limited? lets check it.
				if(samp < -32768 || samp > 32767) printf("[%02d]: limiting fmod value of %d !\n",samp);
				fmod = limit(samp);
				continue;
			}

			chan->smpcnt += chan->smpinc;

			s32 left = (samp * chan->iLeftVolume) / 0x4000;
			s32 right = (samp * chan->iRightVolume) / 0x4000;

			left_accum += left;
			right_accum += right;

			if(!killReverb)
				if (chan->bRVBActive) 
					spu->StoreREVERB(chan,left,right);
		} //channel loop

		if(!killReverb)
		{
			left_accum += spu->MixREVERBLeft();
			right_accum += spu->MixREVERBRight();
		}

		{
			s32 left, right;
			spu->xaqueue.fetch(&left,&right);

			left_accum += left;
			right_accum += right;
		}

		//handle spu mute
		if ((spu->spuCtrl&0x4000)==0) {
			left_accum = 0;
			right_accum = 0;
		}

		s16 output[] = { limit(left_accum), limit(right_accum) };

		if(iSoundMode == SOUND_MODE_SYNCH)
			synchronizer->enqueue_samples(output,1);

		spu->outbuf[j*2] = output[0];
		spu->outbuf[j*2+1] = output[1];

		//fwrite(&left_out,2,1,wavout);
		//fwrite(&right_out,2,1,wavout);
		//fflush(wavout);

		// special irq handling in the decode buffers (0x0000-0x1000)
		// we know:
		// the decode buffers are located in spu memory in the following way:
		// 0x0000-0x03ff  CD audio left
		// 0x0400-0x07ff  CD audio right
		// 0x0800-0x0bff  Voice 1
		// 0x0c00-0x0fff  Voice 3
		// and decoded data is 16 bit for one sample
		// we assume:
		// even if voices 1/3 are off or no cd audio is playing, the internal
		// play positions will move on and wrap after 0x400 bytes.
		// Therefore: we just need a pointer from spumem+0 to spumem+3ff, and
		// increase this pointer on each sample by 2 bytes. If this pointer
		// (or 0x400 offsets of this pointer) hits the spuirq address, we generate
		// an IRQ. 

		if(spu->isCore)
		{
			spu->triggerIrqRange(spu->mixIrqCounter,2);
			spu->triggerIrqRange(spu->mixIrqCounter+0x400,2);
			spu->triggerIrqRange(spu->mixIrqCounter+0x800,2);
			spu->triggerIrqRange(spu->mixIrqCounter+0xC00,2);

			spu->mixIrqCounter += 2;
			spu->mixIrqCounter &= 0x3FF;
		}

	} //sample loop

	//this prints which channels are active
	//for(int i=0;i<24;i++) {
	//	if(i==16) printf(" ");
	//	if(checklog[i]) printf("%1x",i&15);
	//	else printf(" ");
	//}
	//printf("\n");

	if (spu == SPU_core) 
		RecordBuffer(&spu->outbuf[0], length*4);
}

u16 SPU_struct::SPUreadDMA(void)
{
	//printf("SPU single read dma %08X\n",spuAddr);

	u16 s=SPU_core->spuMem[spuAddr>>1];

	//triggerIrqRange(spuAddr,2);

	spuAddr+=2;
	if (spuAddr>0x7ffff) spuAddr=0;

	return s;
}

void SPU_struct::SPUreadDMAMem(u16 * pusPSXMem,int iSize)
{
	//printf("SPU multi read dma %08X %d\n",spuAddr, iSize);

	for (int i=0;i<iSize;i++)
	{
		*pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
		//triggerIrqRange(spuAddr,2);
		spuAddr+=2;                                         // inc spu addr
		if (spuAddr>0x7ffff) spuAddr=0;                     // wrap
	}
}

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

void SPU_struct::SPUwriteDMA(u16 val)
{
	//printf("SPU single write dma %08X\n",spuAddr);

	spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister
	//triggerIrqRange(spuAddr,2);

	spuAddr+=2;                                           // inc spu addr
	if (spuAddr>0x7ffff) spuAddr=0;                       // wrap
}


void SPU_struct::SPUwriteDMAMem(u16 * pusPSXMem,int iSize)
{
	//printf("SPU multi write dma %08X %d\n",spuAddr, iSize);

	for (int i=0;i<iSize;i++)
	{
		spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
		//triggerIrqRange(spuAddr,2);
		spuAddr+=2;                                         // inc spu addr
		if (spuAddr>0x7ffff) spuAddr=0;                     // wrap
	}
}

u16 SPUreadDMA() { return SPU_core->SPUreadDMA(); }
void SPUreadDMAMem(u16 * pusPSXMem,int iSize) { SPU_core->SPUreadDMAMem(pusPSXMem,iSize); }
void SPUwriteDMA(u16 val) {
	SPU_core->SPUwriteDMA(val);
	if(SPU_user) SPU_user->SPUwriteDMA(val);
}
void SPUwriteDMAMem(u16 * pusPSXMem,int iSize) { 
	SPU_core->SPUwriteDMAMem(pusPSXMem,iSize);
	if(SPU_user) SPU_user->SPUwriteDMAMem(pusPSXMem,iSize);
}

//measured 16123034 cycles per second
//this is around 15.3761 MHZ
//this is pasted from the gpu:
//fFrameRateHz=33868800.0f/677343.75f;        // 50.00238
//else fFrameRateHz=33868800.0f/680595.00f;        // 49.76351
//16934400 is half this. we'll go with that assumption for now
static const double time_per_cycle = (double)1.0/(PSXCLK);
static const double samples_per_cycle = time_per_cycle * 44100;


_ADSRInfo::_ADSRInfo()
:	State(0)
,	AttackModeExp(0)
,	AttackRate(0)
,	DecayRate(0)
,	SustainLevel(0)
,	SustainModeExp(0)
,	SustainIncrease(0)
,	SustainRate(0)
,	ReleaseModeExp(0)
,	ReleaseRate(0)
,	EnvelopeVol(0)
,	lVolume(0)
{
}

void SNDDXUpdateAudio(s16 *buffer, u32 num_samples);

void SPUasync(unsigned long cycle)
{
	Lock lock;

	SPU_core->mixtime += samples_per_cycle*cycle*2;
	int mixtodo = (int)SPU_core->mixtime;
	SPU_core->mixtime -= mixtodo;

	win_sound_samplecounter = mixtodo;

	switch(iSoundMode)
	{
	case SOUND_MODE_ASYNCH:
		break;
	case SOUND_MODE_DUAL:
		//produce the logically correct amount of sound, just for emulation's sake (output will be discarded)
		//TODO (as an optimization, the output could actually be non-mixed. i think this may be done in desmume)
		mixAudio(true,SPU_core,mixtodo);
		break;
	case SOUND_MODE_SYNCH:
		//printf("mixing %d cycles (%d samples) (adjustobuf size:%d)\n",cycle,mixtodo,adjustobuf.size);

		//produce the logically correct amount of sound. 
		//these will get enqueued in the synchronizer
		mixAudio(false,SPU_core,mixtodo);
		break;
	}
}

void _ADSRInfo::save(EMUFILE* fp) {
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
	fp->write32le(&ReleaseRate);
	fp->write32le(&EnvelopeVol);
	fp->write32le(&lVolume);
}
void _ADSRInfo::load(EMUFILE* fp) {
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
	fp->read32le(&ReleaseRate);
	fp->read32le(&EnvelopeVol);
	fp->read32le(&lVolume);
}

void REVERBInfo::save(EMUFILE* fp) {
	fp->write32le((u32)0); //version
	for(int i=0;i<41;i++)
		fp->write32le(&arr[i]);
}
void REVERBInfo::load(EMUFILE* fp) {
	u32 version = fp->read32le();
	for(int i=0;i<41;i++)
		fp->read32le(&arr[i]);
}


////////////////////////////////////////////////////////////////////////
// XA AUDIO
////////////////////////////////////////////////////////////////////////

//this is called from the cdrom system with a buffer of decoded xa audio
//we are supposed to grab it and then play it. 
void SPUplayADPCMchannel(xa_decode_t *xap)
{
	if (!iUseXA)    return;                               // no XA? bye
	if (!xap)       return;
	if (!xap->freq) return;                               // no xa freq ? bye

	Lock lock;
	SPU_core->xaqueue.feed(xap);
	
	switch(iSoundMode)
	{
	case SOUND_MODE_ASYNCH:
	case SOUND_MODE_DUAL:
		SPU_user->xaqueue.feed(xap);
		break;
	case SOUND_MODE_SYNCH:
		break;
	}
}

//this func will be called first by the main emu
long SPUinit(void)
{
	//wavout = fopen("c:\\pcsx.raw","wb");
	SPU_core = new SPU_struct(true);
	SPU_user = new SPU_struct(false);
	StaticInitADSR();
	SPUReset();
	return 0;
}

void SPUReset()
{
	ESynchMethod method;
	switch(iSynchMethod)
	{
	case 0: method = ESynchMethod_N; break;
	case 1: method = ESynchMethod_Z; break;
	case 2: method = ESynchMethod_P; break;
	}

	delete synchronizer;
	synchronizer = metaspu_construct(method);
	
	//call this from reset as well (then we wont have to track the user spu during non-dual modes)
	//TODO - make same optimization in desmume
	SPUcloneUser();
}

void SPUcloneUser()
{
	*SPU_user = *SPU_core;
	SPU_user->isCore = false;
	for(int i=0;i<24;i++)
		SPU_user->channels[i].spu = SPU_user;
}

bool SPUunfreeze_new(EMUFILE* fp)
{
	Lock lock;

	reconstruct(SPU_core,true);
	reconstruct(SPU_user,false);

	u32 tag = fp->read32le();
	if(tag != 0xBEEFFACE) return false;

	u32 version = fp->read32le();
	//if(version<3) return false;

	fp->fread(SPU_core->spuMem,0x80000);
	fp->fread(regArea,0x200);

	fp->readdouble(&SPU_core->mixtime);
	fp->read32le(&SPU_core->dwNoiseVal);
	fp->read32le(&SPU_core->iLeftXAVol);
	fp->read32le(&SPU_core->iRightXAVol);

	fp->read8le(&SPU_core->iReverbCycle);
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

	//a bit weird to do this here, but i wanted to have a more solid XA state saver
	//and the cdr freeze sucks. I made sure this saves and loads after the cdr state
	cdr.Xa.load(fp);


	u32 endtag = fp->read32le();
	if(endtag != 0xBAADF00D) return false;

	SPUcloneUser();

	return true;
}




////////////////////////////////////////////////////////////////////////
// SPUOPEN: called by main emu after init
////////////////////////////////////////////////////////////////////////

static volatile bool doterminate;
static volatile bool terminated;

void SPU_Emulate_user()
{
	if(!SPU_user)
		return;

	u32 SNDDXGetAudioSpace();
	u32 audiosize = SNDDXGetAudioSpace();

	if (audiosize > 0)
	{
		//if (audiosize > SPU_user->bufsize)
		//	audiosize = SPU_user->bufsize;

		s16* outbuf;
		int samplesOutput;
		switch(iSoundMode)
		{
		case SOUND_MODE_ASYNCH:
			mixAudio(false,SPU_core,audiosize);
			outbuf = SPU_core->outbuf;
			samplesOutput = audiosize;
			break;
		case SOUND_MODE_DUAL:
			mixAudio(false,SPU_user,audiosize);
			outbuf = SPU_user->outbuf;
			samplesOutput = audiosize;
			break;
		case SOUND_MODE_SYNCH:
			samplesOutput = synchronizer->output_samples(outbuf = SPU_user->outbuf, audiosize);
			break;
		}

		if(samplesOutput)
		{
			//1 - loudest
			//5 - mute
			int vol = 5-iVolume;
			for(int i=0;i<samplesOutput*2;i++)
			{
				int s = outbuf[i];
				s = s*vol/4;
				outbuf[i] = s;
			}
			SNDDXUpdateAudio(outbuf,samplesOutput);
		}
	}
}

DWORD WINAPI SNDDXThread( LPVOID )
{
	for(;;) {
		if(doterminate) break;
		{
			Lock lock;
			SPU_Emulate_user();
		}
		Sleep(10);
	}
	terminated = true;
	return 0;
}

static bool soundInitialized = false;
#ifdef _WINDOWS
long SPUopen(HWND hW)
#else
long SPUopen(void)
#endif
{
	if(soundInitialized) return PSE_SPU_ERR_SUCCESS;

	soundInitialized = true;
	InitializeCriticalSection(&win_execute_sync);

	iUseXA=1;                                             // just small setup
	iVolume=3;

#ifdef _WINDOWS
//	LastWrite=0xffffffff;
//	LastPlay=0;                      // init some play vars
	if (!IsWindow(hW)) hW=GetActiveWindow();
	hWMain = hW;                                          // store hwnd
#endif

	ReadConfig();                                         // read user stuff

	SetupSound();                                         // setup sound (before init!)

#ifdef _WINDOWS
	if (iRecordMode)                                      // windows recording dialog
	{
		hWRecord=CreateDialog(0,MAKEINTRESOURCE(IDD_RECORD),
		                      NULL,(DLGPROC)RecordDlgProc);
		SetWindowPos(hWRecord,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE);
		UpdateWindow(hWRecord);
		SetFocus(hWMain);
	}
#endif

	doterminate = false;
	terminated = false;
	CreateThread(0,0,SNDDXThread,0,0,0);

	return PSE_SPU_ERR_SUCCESS;
}

void SPUmute()
{
	void SNDDXMuteAudio();
	SNDDXMuteAudio();
}

void SPUunMute()
{
	void SNDDXUnMuteAudio();
	SNDDXUnMuteAudio();
}


////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS
void SPUsetConfigFile(char * pCfg)
{
	pConfigFile=pCfg;
}
#endif

////////////////////////////////////////////////////////////////////////
// SPUCLOSE: called before shutdown
////////////////////////////////////////////////////////////////////////

long SPUclose(void)
{
#ifdef _WINDOWS
	if (IsWindow(hWRecord)) DestroyWindow(hWRecord);
	hWRecord=0;
#endif

	RemoveSound();                                        // no more sound handling

	return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUSHUTDOWN: called by main emu on final exit
////////////////////////////////////////////////////////////////////////

long SPUshutdown(void)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUCONFIGURE: call config dialog
////////////////////////////////////////////////////////////////////////

long SPUconfigure(void)
{
#ifdef _WINDOWS
	INT_PTR wtf = DialogBox(gApp.hInstance,MAKEINTRESOURCE(IDD_CFGDLG),
	          GetActiveWindow(),(DLGPROC)DSoundDlgProc);
	DWORD arg = GetLastError();
#else
	StartCfgTool("CFG");
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////
// SETUP CALLBACKS
// this functions will be called once,
// passes a callback that should be called on SPU-IRQ/cdda volume change
////////////////////////////////////////////////////////////////////////

void SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(unsigned short,unsigned short))
{
	cddavCallback = CDDAVcallback;
}

////////////////////////////////////////////////////////////////////////
// COMMON PLUGIN INFO FUNCS
////////////////////////////////////////////////////////////////////////

void SPUstartWav(char* filename)
{
	strncpy(szRecFileName,filename,260);
	iDoRecord=1;
	RecordStart();
}
void SPUstopWav()
{
	RecordStop();
}



//---------------------------------------------------------------
//original changelog

// History of changes:
//
// 2004/09/19 - Pete
// - added option: IRQ handling in the decoded sound buffer areas (Crash Team Racing)
//
// 2004/09/18 - Pete
// - changed global channel var handling to local pointers (hopefully it will help LDChen's port)
//
// 2004/04/22 - Pete
// - finally fixed frequency modulation and made some cleanups
//
// 2003/04/07 - Eric
// - adjusted cubic interpolation algorithm
//
// 2003/03/16 - Eric
// - added cubic interpolation
//
// 2003/03/01 - linuzappz
// - libraryName changes using ALSA
//
// 2003/02/28 - Pete
// - added option for type of interpolation
// - adjusted spu irqs again (Thousant Arms, Valkyrie Profile)
// - added MONO support for MSWindows DirectSound
//
// 2003/02/20 - kode54
// - amended interpolation code, goto GOON could skip initialization of gpos and cause segfault
//
// 2003/02/19 - kode54
// - moved SPU IRQ handler and changed sample flag processing
//
// 2003/02/18 - kode54
// - moved ADSR calculation outside of the sample decode loop, somehow I doubt that
//   ADSR timing is relative to the frequency at which a sample is played... I guess
//   this remains to be seen, and I don't know whether ADSR is applied to noise channels...
//
// 2003/02/09 - kode54
// - one-shot samples now process the end block before stopping
// - in light of removing fmod hack, now processing ADSR on frequency channel as well
//
// 2003/02/08 - kode54
// - replaced easy interpolation with gaussian
// - removed fmod averaging hack
// - changed .sinc to be updated from .iRawPitch, no idea why it wasn't done this way already (<- Pete: because I sometimes fail to see the obvious, haharhar :)
//
// 2003/02/08 - linuzappz
// - small bugfix for one usleep that was 1 instead of 1000
// - added iDisStereo for no stereo (Linux)
//
// 2003/01/22 - Pete
// - added easy interpolation & small noise adjustments
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/12 - Pete
// - added recording window handlers
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/12/28 - Pete
// - adjusted spu irq handling, fmod handling and loop handling
//
// 2002/08/14 - Pete
// - added extra reverb
//
// 2002/06/08 - linuzappz
// - SPUupdate changed for SPUasync
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release