/***************************************************************************
                        stdafx.h  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
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
// 2001/10/28 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//


//#define  STRICT
#define  D3D_OVERLOADS
#define  DIRECT3D_VERSION 0x600
#define  CINTERFACE

#include <WINDOWS.H>
#include <WINDOWSX.H>
#include <TCHAR.H>
#include "resource.h"

// Pete: since my last OS + compiler reinstall, I needed to user newer
// defines/libs, therefore I've decided to use the mingw headers and
// the d3dx.lib (old libs: d3dim.lib dxguid.lib)


#include "mingw_ddraw.h"
#include "mingw_d3dtypes.h"
#include "mingw_d3d.h"

// stupid intel compiler warning on extern __inline funcs
//#pragma warning (disable:864)
// disable stupid MSVC2005 warnings as well...

#pragma warning (disable:4996)
#pragma warning (disable:4244)


