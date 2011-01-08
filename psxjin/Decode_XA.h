//============================================
//=== Audio XA decoding
//=== Kazzuya
//============================================

#ifndef DECODEXA_H
#define DECODEXA_H

#include "PsxCommon.h"
#include "emufile.h"

class EMUFILE;

struct ADPCM_Decode_t
{
	s32	y0, y1;
	void save(EMUFILE* fp);
	void load(EMUFILE* fp);
};

struct xa_decode_t
{
	s32				freq;
	s32				nbits;
	s32				stereo;
	s32				nsamples;
	ADPCM_Decode_t	left, right;
	s16			pcm[16384];

	void save(EMUFILE* fp);
	void load(EMUFILE* fp);
};


long xa_decode_sector( xa_decode_t *xdp,
                       unsigned char *sectorp,
                       int is_first_sector );
#endif
