/***************************************************************************
                          reverb.h  -  description
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

#ifndef _REVERB_H_
#define _REVERB_H_

#include "spu.h"

void InitREVERB();
void ShutdownREVERB();
void StartREVERB(SPU_chan* pChannel);
void StoreREVERB(SPU_chan* pChannel,s32 left, s32 right);
s32 MixREVERBLeft();
s32 MixREVERBRight();
void REVERB_initSample();

#endif //_REVERB_H_