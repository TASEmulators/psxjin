//============================================
//=== Audio XA decoding
//=== Kazzuya
//============================================

#ifndef DECODEXA_H
#define DECODEXA_H

#include "PsxCommon.h"
#include "emufile.h"

struct ADPCM_Decode_t
{
	s32	y0, y1;
	void save(EMUFILE* fp) {
		fp->write32le(&y0);
		fp->write32le(&y1);
	}
	void load(EMUFILE* fp) {
		fp->read32le(&y0);
		fp->read32le(&y1);
	}
};

struct xa_decode_t
{
	s32				freq;
	s32				nbits;
	s32				stereo;
	s32				nsamples;
	ADPCM_Decode_t	left, right;
	s16			pcm[16384];

	void save(EMUFILE* fp) {
		fp->write32le((u32)0); //version
		fp->write32le(&freq);
		fp->write32le(&nbits);
		fp->write32le(&stereo);
		fp->write32le(&nsamples);
		left.save(fp);
		right.save(fp);
		for(int i=0;i<16384;i++)
			fp->write16le(&pcm[i]);
	}

	void load(EMUFILE* fp) {
		u32 version = fp->read32le();
		fp->read32le(&freq);
		fp->read32le(&nbits);
		fp->read32le(&stereo);
		fp->read32le(&nsamples);
		left.load(fp);
		right.load(fp);
		for(int i=0;i<16384;i++)
			fp->read16le(&pcm[i]);
	}
};


long xa_decode_sector( xa_decode_t *xdp,
                       unsigned char *sectorp,
                       int is_first_sector );
#endif
