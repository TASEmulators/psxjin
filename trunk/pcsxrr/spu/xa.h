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
struct xa_decode_t;

class xa_queue_base
{
public:
	virtual void freeze(EMUFILE* fp)=0;
	virtual bool unfreeze(EMUFILE* fp)=0;
	static xa_queue_base* construct();
	virtual void feed(xa_decode_t *xap)=0;
	virtual void fetch(s32* left, s32* right)=0;
};

#endif //_XA_H_
