//xa.cpp
//original (C) 2002 by Pete Bernert
//nearly entirely rewritten for pcsxrr by zeromus

//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version. See also the license.txt file for
//additional informations.                                              


#include "stdafx.h"
#include "PsxCommon.h"

#include <deque>
#include "spu.h"

extern int bSPUIsOpen;
s32 _Interpolate(s16 a, s16 b, s16 c, s16 d, double _ratio);

struct xa_sample
{
	union {
		s16 both[2];
		struct {
			s16 left, right;
		};
	};
	s32 freq;

	void freeze(EMUFILE* fp)
	{
		fp->write16le(&left);
		fp->write16le(&right);
		fp->write32le(&freq);
	}
	bool unfreeze(EMUFILE* fp)
	{
		fp->read16le(&left);
		fp->read16le(&right);
		fp->read32le(&freq);
		return true;
	}
};

//this code is really naive. it's probably all wrong, i'm not good at this kind of stuff.
//really ought to make a circular buffer.
//alternatively we could keep these in the blocks they originally came in
//instead of splitting them all into samples.
//the timing logic is terribly slow.
//going for a simple reference implementation here.
//but it is a little strange because the blocks can be different samplerates.
//another reason for implementing it this way is so that the xa_queue can represent some kind of reliable hardware state
//and the spu can fetch samples from it at whatever rate it needs to (whatever rate, one day, the user has specified)
//instead of resampling everything as soon as it is received.
//
//there may be something wrong with the interpolation, but it is hard to tell, since I think there is also
//something wrong with the xa adpcm decoding.
class xa_queue : public std::deque<xa_sample>, public xa_queue_base
{
public:
	xa_queue()
		: counter(0)
		, lastFrac(0)
	{
		//we need to be bootstrapped with something
		xa_sample blank;
		blank.left = blank.right = 0;
		blank.freq = 44100;
		for(int i=0;i<4;i++)
			curr.push_back(blank);
	}

	void enqueue(xa_decode_t* xap)
	{
		xa_sample blank;
		blank.left = blank.right = 0;
		blank.freq = xap->freq;

		//the first time we enqueue any data, put a little delay.
		//this may help, but probably not, since we have some timing problem
		//that makes us replay too fast
		if(size()==0)
			for(int i=0;i<882;i++)
				push_back(blank);

		if(xap->stereo)
			for(int i=0;i<xap->nsamples;i++)
			{
				xa_sample temp;
				temp.freq = xap->freq;
				temp.left = xap->pcm[i*2];
				temp.right = xap->pcm[i*2+1];
				push_back(temp);
			}
		else
			for(int i=0;i<xap->nsamples;i++)
			{
				xa_sample temp;
				temp.freq = xap->freq;
				temp.left = temp.right = xap->pcm[i];
				push_back(temp);
			}
	}

	double counter;
	double lastFrac;

	std::deque<xa_sample> curr;

	void fetch(s16* fourStereoSamples)
	{
		for(int i=0;i<4;i++)
		{
			fourStereoSamples[i*2] = curr[3-i].left;
			fourStereoSamples[i*2+1] = curr[3-i].right;
		}
	}

	void advance()
	{
		if(size()==0) return;
		counter += 1.0/44100;
		for(;;)
		{
			if(size()==0) {
				printf("empty\n");
				lastFrac = 0;
				counter = 0;
				return;
			}
			double target = 1.0/front().freq;
			if(counter>=target)
			{
				curr.pop_front();
				curr.push_back(front());
				pop_front();
				counter -= target;
			} else break;
		}
		lastFrac = counter/(1.0/front().freq);
		//lastFrac = 0;
	}

	virtual void freeze(EMUFILE* fp)
	{
		fp->write32le((u32)0); //version
		fp->writedouble(counter);
		fp->writedouble(lastFrac);
		fp->write32le(curr.size());
		for(size_t i=0;i<curr.size();i++)
			curr[i].freeze(fp);
		fp->write32le(size());
		for(size_t i=0;i<size();i++)
			operator[](i).freeze(fp);
	}

	virtual bool unfreeze(EMUFILE* fp)
	{
		u32 version;
		fp->read32le(&version);
		fp->readdouble(&counter);
		fp->readdouble(&lastFrac);
		u32 temp;
		fp->read32le(&temp);
		for(size_t i=0;i<temp;i++)
		{
			xa_sample samp;
			samp.unfreeze(fp);
			curr.push_back(samp);
		}
		fp->read32le(&temp);
		for(size_t i=0;i<temp;i++)
		{
			xa_sample samp;
			samp.unfreeze(fp);
			push_back(samp);
		}
		return true;
	}

	void feed(xa_decode_t *xap)
	{
		printf("xa feeding xa nsamp=%d chans=%d freq=%d\n",xap->nsamples,xap->stereo*2,xap->freq);
		//printf("%d\n",xaqueue.size());

		if (!bSPUIsOpen) return;

		enqueue(xap);
	}

	void fetch(s32* left, s32* right)
	{
		s16 samples[8];

		fetch(samples);
		*left = _Interpolate(samples[0], samples[2], samples[4], samples[6], lastFrac);
		*right = _Interpolate(samples[1], samples[3], samples[5], samples[7], lastFrac);

		*left = (*left * SPU_core->iLeftXAVol)/32767;
		*right = (*right * SPU_core->iRightXAVol)/32767;

		advance();
	}
};

xa_queue_base* xa_queue_base::construct() { return new xa_queue(); }