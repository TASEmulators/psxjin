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

#ifdef _WINDOWS

#define  STRICT
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

#ifdef __MINGW32__
#include <ddraw.h>
#include <d3d.h>
#else
#include "mingw_ddraw.h"                       
#include "mingw_d3dtypes.h"
#include "mingw_d3d.h"
#endif

// stupid intel compiler warning on extern __inline funcs
//#pragma warning (disable:864)
// disable stupid MSVC2005 warnings as well...
#ifndef __MINGW32__
#pragma warning (disable:4996)
#pragma warning (disable:4244)
#endif

// enable that for auxprintf();
//#define SMALLDEBUG
//#include <dbgout.h>
//void auxprintf (LPCTSTR pFormat, ...);

#ifdef __MINGW32__
#ifndef _MINGW_FIX_DEFINED
#define _MINGW_FIX_DEFINED
#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED 0x00000001
#define ES_DISPLAY_REQUIRED 0x00000002
#define ES_USER_PRESENT 0x00000004
#define ES_CONTINUOUS 0x80000000
#endif
typedef DWORD EXECUTION_STATE;
#endif
#endif
#else

#ifndef _SDL
#define __X11_C_ 
//X11 render
#define __inline inline
#define CALLBACK

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/time.h> 
#include <GL/gl.h>  
#include <GL/glx.h>  
#include <math.h> 
#include <X11/cursorfont.h> 

#else 		//SDL render

#define __inline inline
#define CALLBACK

#include <SDL/SDL.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/time.h> 
#include <math.h> 

#endif
 
#endif

