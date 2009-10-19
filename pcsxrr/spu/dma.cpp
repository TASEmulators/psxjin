/***************************************************************************
                            dma.c  -  description
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

#include "stdafx.h"

#define _IN_DMA

#include "externals.h"

void triggerIrqRange(u32 base, u32 size);

// READ DMA (one value)
u16 SPUreadDMA(void)
{
	u16 s;

	//printf("SPU single read dma %08X\n",spuAddr);

	s=spuMem[spuAddr>>1];

	//triggerIrqRange(spuAddr,2);

	spuAddr+=2;
	if (spuAddr>0x7ffff) spuAddr=0;

	return s;
}

//READ DMA (many values)
void SPUreadDMAMem(u16 * pusPSXMem,int iSize)
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

//WRITE DMA (one value)
void SPUwriteDMA(u16 val)
{
	//printf("SPU single write dma %08X\n",spuAddr);

	spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister
	//triggerIrqRange(spuAddr,2);

	spuAddr+=2;                                           // inc spu addr
	if (spuAddr>0x7ffff) spuAddr=0;                       // wrap
}

//WRITE DMA (many values)
void SPUwriteDMAMem(u16 * pusPSXMem,int iSize)
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


