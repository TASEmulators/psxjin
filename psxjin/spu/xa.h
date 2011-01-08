/***************************************************************************
                            xa.h  -  description
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

#ifndef _XA_H_
#define _XA_H_

#include "PsxCommon.h"
#include <deque>
#include "emufile.h"
struct xa_decode_t;

class EMUFILE;

struct xa_sample
{
	xa_sample() {}
	xa_sample(s16 l, s16 r)
		: left(l)
		, right(r)
	{
	}

	union {
		s16 both[2];
		struct {
			s16 left, right;
		};
	};
	s32 freq;

	void freeze(EMUFILE* fp);
	bool unfreeze(EMUFILE* fp);
};

class xa_queue : public std::deque<xa_sample>
{
public:
	xa_queue();
	void enqueue(xa_decode_t* xap);
	void advance();
	void freeze(EMUFILE* fp);
	bool unfreeze(EMUFILE* fp);
	void feed(xa_decode_t *xap);
	void fetch(s16* fourStereoSamples);
	void fetch(s32* left, s32* right);

private:
	double counter;
	double lastFrac;

	std::deque<xa_sample> curr;

};

#endif //_XA_H_
