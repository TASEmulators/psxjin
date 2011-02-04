/***************************************************************************
                         menu.c  -  description
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
// 2005/04/15 - Pete
// - Changed user frame limit to floating point value
//
// 2003/07/27 - Pete
// - Added pad core flags display
//
// 2002/12/14 - Pete
// - Added dithering menu item
//
// 2002/05/25 - Pete
// - Added menu support for linuzappz's fast forward skipping
//
// 2002/01/13 - linuzappz
// - Added timing for the szDebugText (to 2 secs)
//
// 2001/12/22 - syo
// - modified for "transparent menu"
//   (Pete: added 'V' display for WaitVBlank)
//
// 2001/11/09 - Darko Matesic
// - added recording status
//   (Pete: added terminate zero to the menu buffer ;)
//
// 2001/10/28 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#ifdef _WINDOWS

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "gpu_record.h"

#endif

#define _IN_MENU

#include "externals.h"
#include "draw.h"
#include "menu.h"
#include "gpu.h"
#include "PSXCommon.h"

unsigned long dwCoreFlags=0;
char cCurrentFrame[14];
char cCurrentLag[7];
char cCurrentInput[36];
char cCurrentMode[14];
extern int currentFrame;
extern int totalFrames;
extern int currentLag;
extern int currentInput;
extern int ThereIsLag;

extern char modeFlags;
#define MODE_FLAG_RECORD (1<<1)
#define MODE_FLAG_REPLAY (1<<2)
#define MODE_FLAG_PAUSED (1<<3)

////////////////////////////////////////////////////////////////////////
// create lists/stuff for fonts (actually there are no more lists, but I am too lazy to change the func names ;)
////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
HFONT hGFont=NULL;
HFONT StatusFont=NULL;
BOOL bTransparent=FALSE;
#endif

void InitMenu(void)
{
#ifdef _WINDOWS
	StatusFont = CreateFont(24, 0, 0, 0, FW_DEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, FF_SWISS, "Webdings");
	hGFont=CreateFont(//-8,
	         13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
	         ANSI_CHARSET,OUT_DEFAULT_PRECIS,
	         CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
	         DEFAULT_PITCH,
	         //"Courier New");
	         //"MS Sans Serif");
	         "Arial");
#endif
}

////////////////////////////////////////////////////////////////////////
// kill existing lists/fonts
////////////////////////////////////////////////////////////////////////

void CloseMenu(void)
{
#ifdef _WINDOWS
	if (hGFont) DeleteObject(hGFont);
	hGFont=NULL;
	if (StatusFont) DeleteObject(StatusFont);
	StatusFont=NULL;
#else
	DestroyPic();
#endif
}

////////////////////////////////////////////////////////////////////////
// DISPLAY FPS/MENU TEXT
////////////////////////////////////////////////////////////////////////

#include <time.h>
extern time_t tStart;

int iMPos=0;                                           // menu arrow pos

void DisplayText(void)                                 // DISPLAY TEXT
{
#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;

	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	SetTextColor(hdc,RGB(0,255,0));
	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));

	if (szDebugText[0] && ((time(NULL) - tStart) < 2) && (ulKeybits&KEY_SHOWFPS))    // special debug text? show it
	{
		RECT r={2,PSXDisplay.DisplayMode.y-200,1022,1024};
		DrawText(hdc,szDebugText,lstrlen(szDebugText),&r,DT_LEFT|DT_NOCLIP);
	}
	else                                                  // else standard gpu menu
	{
		szDebugText[0]=0;	
	}
	

	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
#endif
}

void DisplayFrames(void)
{
#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;

	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	if (ThereIsLag)
	{
		SetTextColor(hdc,RGB(255,0,0));
	}
	else SetTextColor(hdc,RGB(255,255,255));

	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));

	ExtTextOut(hdc,3,2,0,NULL,cCurrentFrame,lstrlen(cCurrentFrame),NULL);

	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
#endif
}

void DisplayLag(void)
{
#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;

	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	SetTextColor(hdc,RGB(255,0,0));
	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));

	ExtTextOut(hdc,3,12,0,NULL,cCurrentLag,lstrlen(cCurrentLag),NULL);

	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
#endif
}

void DisplayMovMode(void)
{
#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;

	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);

	hFO=(HFONT)SelectObject(hdc,StatusFont);

	if (modeFlags&MODE_FLAG_RECORD)
		SetTextColor(hdc,RGB(255,0,0));
	else if (modeFlags&MODE_FLAG_REPLAY)
		SetTextColor(hdc,RGB(255,255,255));
	else
		SetTextColor(hdc,RGB(0,255,0));

	SetBkMode(hdc,TRANSPARENT);
	SetTextAlign(hdc, TA_TOP | TA_RIGHT);

	ExtTextOut(hdc,PSXDisplay.DisplayMode.x-3,2,0,NULL,cCurrentMode,lstrlen(cCurrentMode),NULL);

	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
#endif
}
void DisplayInput(short P1, short P2)
{
	#ifdef _WINDOWS	
	const unsigned short Order[] = {0x80, 0x10, 0x20, 0x40, 0x8, 0x1, 0x8000, 0x4000, 0x2000, 0x1000, 0x400, 0x400, 0x800, 0x800, 0x100, 0x100, 0x200, 0x200};	
	HDC hdc;
	HFONT hFO;	
	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	SetTextColor(hdc,RGB(0,255,0));
	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));
	ExtTextOut(hdc,2,PSXDisplay.DisplayMode.y-12,0,NULL,szInputBuf,36,NULL);	
	for (int i = 0; i < 18; i++)
	{
		if (P1&Order[i])
		{NULL;} else szInputBuf[i] = ' ';
	}
	for (int i = 0; i < 18; i++)
	{
		if (P2&Order[i])
		{} else szInputBuf[i+18] = ' ';
	}
	SetTextColor(hdc,RGB(0,255,255));
	SetBkMode(hdc,TRANSPARENT);
	ExtTextOut(hdc,2,PSXDisplay.DisplayMode.y-12,0,NULL,szInputBuf,36,NULL);
	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
	#endif
}
void DisplayAnalog(PadDataS padd1, PadDataS padd2)
{
	#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;
	char tempstr[128];
	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	SetTextColor(hdc,RGB(0,255,0));
	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));
	int n = 1;
	if (padd2.controllerType !=4)
	{
		sprintf(tempstr,"%03d %03d %03d %03d",padd2.leftJoyX, padd2.leftJoyY, padd2.rightJoyX, padd2.rightJoyY);
		ExtTextOut(hdc,PSXDisplay.DisplayMode.x-90,PSXDisplay.DisplayMode.y-12,0,NULL,tempstr,lstrlen(tempstr),NULL);
		n++;
	}	
	if (padd1.controllerType !=4)
	{
		sprintf(tempstr,"%03d %03d %03d %03d",padd1.leftJoyX, padd1.leftJoyY, padd1.rightJoyX, padd1.rightJoyY);
		ExtTextOut(hdc,PSXDisplay.DisplayMode.x-90,PSXDisplay.DisplayMode.y-(n*12),0,NULL,tempstr,lstrlen(tempstr),NULL);
	}	
	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
	#endif
}

void DisplayRecording(int RecNum, int MaxPlayer)
{
	#ifdef _WINDOWS
	HDC hdc;
	HFONT hFO;
	char *text;
	text = new char[10];
	IDirectDrawSurface_GetDC(DX.DDSRender,&hdc);
	hFO=(HFONT)SelectObject(hdc,hGFont);

	SetTextColor(hdc,RGB(0,255,0));
	if (bTransparent)
		SetBkMode(hdc,TRANSPARENT);
	else SetBkColor(hdc,RGB(0,0,0));
	if (RecNum <= MaxPlayer)
	{	
		sprintf(text,"Player %d\0",RecNum);
		ExtTextOut(hdc,2,PSXDisplay.DisplayMode.y-24,0,NULL,text,8,NULL);
	}
	else if (RecNum == MaxPlayer+1)
		ExtTextOut(hdc,2,PSXDisplay.DisplayMode.y-24,0,NULL,"None\0",4,NULL);
	else
		ExtTextOut(hdc,2,PSXDisplay.DisplayMode.y-24,0,NULL,"All\0",3,NULL);			
	SelectObject(hdc,hFO);
	IDirectDrawSurface_ReleaseDC(DX.DDSRender,hdc);
	delete text;
	#endif
}

////////////////////////////////////////////////////////////////////////
// Build Menu buffer (== Dispbuffer without FPS)...
////////////////////////////////////////////////////////////////////////

void BuildDispMenu(int iInc)
{
	unsigned char nStatusSymbols[3] = {0x3D, 0x34, 0x3B};
	int j = 0;
	int i = 0;
	if (!(ulKeybits&KEY_SHOWFPS)) return;                 // mmm, cheater ;)

	iMPos+=iInc;                                          // up or down
	if (iMPos<0) iMPos=3;                                 // wrap around
	if (iMPos>3) iMPos=0;

	strcpy(szMenuBuf,"   FL   FS   DI   GF        ");     // main menu items

	if (UseFrameLimit)                                    // set marks
	{
		if (iFrameLimit==1) szMenuBuf[2]  = '+';
		else               szMenuBuf[2]  = '*';
	}
	if (iFastFwd)       szMenuBuf[7]  = '~';
	else
		if (UseFrameSkip)   szMenuBuf[7]  = '*';

	if (iUseDither)                                       // set marks
	{
		if (iUseDither==1) szMenuBuf[12]  = '+';
		else              szMenuBuf[12]  = '*';
	}

	if (dwActFixes)     szMenuBuf[17] = '*';

	sprintf( cCurrentLag,"%d", currentLag);
	if (modeFlags&MODE_FLAG_REPLAY)
		sprintf( cCurrentFrame,"%d / %d", currentFrame,totalFrames);
	else
		sprintf( cCurrentFrame,"%d", currentFrame);

	memset(cCurrentMode, 0, sizeof(cCurrentMode));
	for (i = 0; i < 3; i++)
	{
		if (modeFlags & (1 << (i+1)))
		{
			cCurrentMode[j] = nStatusSymbols[i];
			j++;
		}
	}
	
	cCurrentInput[0] = currentInput&0x80?' ':'<';
	cCurrentInput[1] = currentInput&0x10?' ':'^';
	cCurrentInput[2] = currentInput&0x20?' ':'>';
	cCurrentInput[3] = currentInput&0x40?' ':'v';
	cCurrentInput[4] = currentInput&0x8?' ':'S';
	cCurrentInput[5] = currentInput&0x1?' ':'s';
	cCurrentInput[6] = currentInput&0x8000?' ':19;
	cCurrentInput[7] = currentInput&0x4000?' ':'X';
	cCurrentInput[8] = currentInput&0x2000?' ':'O';
	cCurrentInput[9] = currentInput&0x1000?' ':'T';
	cCurrentInput[10] = currentInput&0x400?' ':'L'; //l1
	cCurrentInput[11] = currentInput&0x400?' ':'1'; //l1
	cCurrentInput[12] = currentInput&0x800?' ':'R'; //r1
	cCurrentInput[13] = currentInput&0x800?' ':'1'; //r1
	cCurrentInput[14] = currentInput&0x100?' ':'L'; //l2
	cCurrentInput[15] = currentInput&0x100?' ':'2'; //l2
	cCurrentInput[16] = currentInput&0x200?' ':'R'; //r2
	cCurrentInput[17] = currentInput&0x200?' ':'2'; //r2
	cCurrentInput[18] = currentInput&0x800000?' ':'<';
	cCurrentInput[19] = currentInput&0x100000?' ':'^';
	cCurrentInput[20] = currentInput&0x200000?' ':'>';
	cCurrentInput[21] = currentInput&0x400000?' ':'v';
	cCurrentInput[22] = currentInput&0x80000?' ':'S';
	cCurrentInput[23] = currentInput&0x10000?' ':'s';
	cCurrentInput[24] = currentInput&0x80000000?' ':19;
	cCurrentInput[25] = currentInput&0x40000000?' ':'X';
	cCurrentInput[26] = currentInput&0x20000000?' ':'O';
	cCurrentInput[27] = currentInput&0x10000000?' ':'T';
	cCurrentInput[28] = currentInput&0x4000000?' ':'L'; //l1
	cCurrentInput[29] = currentInput&0x4000000?' ':'1'; //l1
	cCurrentInput[30] = currentInput&0x8000000?' ':'R'; //r1
	cCurrentInput[31] = currentInput&0x8000000?' ':'1'; //r1
	cCurrentInput[32] = currentInput&0x1000000?' ':'L'; //l2
	cCurrentInput[33] = currentInput&0x1000000?' ':'2'; //l2
	cCurrentInput[34] = currentInput&0x2000000?' ':'R'; //r2
	cCurrentInput[35] = currentInput&0x2000000?' ':'2'; //r2
    memcpy(szInputBuf,cCurrentInput,36);
	if (dwCoreFlags&1)  szMenuBuf[23]  = 'A';
	if (dwCoreFlags&2)  szMenuBuf[23]  = 'M';
	if (dwCoreFlags&0xff00)                               //A/M/G/D
	{
		if ((dwCoreFlags&0x0f00)==0x0000)                   // D
			szMenuBuf[23]  = 'D';
		else
			if ((dwCoreFlags&0x0f00)==0x0100)                   // A
				szMenuBuf[23]  = 'A';
			else
				if ((dwCoreFlags&0x0f00)==0x0200)                   // M
					szMenuBuf[23]  = 'M';
				else
					if ((dwCoreFlags&0x0f00)==0x0300)                   // G
						szMenuBuf[23]  = 'G';

		szMenuBuf[24]='0'+(char)((dwCoreFlags&0xf000)>>12);                         // number
	}

#ifdef _WINDOWS
	if (bVsync_Key)     szMenuBuf[25]  = 'V';
#endif
	if (lSelectedSlot)  szMenuBuf[26]  = '0'+(char)lSelectedSlot;

	szMenuBuf[(iMPos+1)*5]='<';                           // set arrow

#ifdef _WINDOWS
	if (RECORD_RECORDING)
	{
		szMenuBuf[27]  = ' ';
		szMenuBuf[28]  = ' ';
		szMenuBuf[29]  = ' ';
		szMenuBuf[30]  = 'R';
		szMenuBuf[31]  = 'e';
		szMenuBuf[32]  = 'c';
		szMenuBuf[33]  = 0;
	}

	if (DX.DDSScreenPic) ShowTextGpuPic();
#endif
}

////////////////////////////////////////////////////////////////////////
// Some menu action...
////////////////////////////////////////////////////////////////////////

void SwitchDispMenu(int iStep)                         // SWITCH DISP MENU
{
	if (!(ulKeybits&KEY_SHOWFPS)) return;                 // tststs

	switch (iMPos)
	{//////////////////////////////////////////////////////
	case 0:                                             // frame limit
	{
		int iType=0;
		bInitCap = TRUE;

#ifdef _WINDOWS
		if (iFrameLimit==1 && UseFrameLimit &&
		    GetAsyncKeyState(VK_SHIFT)&32768)
		{
			fFrameRate+=iStep;
			if (fFrameRate<1.0f) fFrameRate=1.0f;
			SetAutoFrameCap();
			break;
		}
#endif

		if (UseFrameLimit) iType=iFrameLimit;
		iType+=iStep;
		if (iType<0) iType=2;
		if (iType>2) iType=0;
		if (iType==0) UseFrameLimit=0;
		else
		{
			UseFrameLimit=1;
			iFrameLimit=iType;
			SetAutoFrameCap();
		}
	}
	break;
	//////////////////////////////////////////////////////
	case 1:                                             // frame skip
		bInitCap = TRUE;
		if (iStep>0)
		{
			if (!UseFrameSkip)
			{
				UseFrameSkip=1;
				iFastFwd = 0;
			}
			else
			{
				if (!iFastFwd) iFastFwd=1;
				else
				{
					UseFrameSkip=0;
					iFastFwd = 0;
				}
			}
		}
		else
		{
			if (!UseFrameSkip)
			{
				UseFrameSkip=1;
				iFastFwd = 1;
			}
			else
			{
				if (iFastFwd) iFastFwd=0;
				else
				{
					UseFrameSkip=0;
					iFastFwd = 0;
				}
			}
		}
		bSkipNextFrame=FALSE;
		break;
		//////////////////////////////////////////////////////
	case 2:                                             // dithering
		iUseDither+=iStep;
		if (iUseDither<0) iUseDither=2;
		if (iUseDither>2) iUseDither=0;
		break;
		//////////////////////////////////////////////////////
	case 3:                                             // special fixes
		if (iUseFixes)
		{
			iUseFixes=0;
			dwActFixes=0;
		}
		else
		{
			iUseFixes=1;
			dwActFixes=dwCfgFixes;
		}
		SetFixes();
		if (iFrameLimit==2) SetAutoFrameCap();
		break;
	}

	BuildDispMenu(0);                                     // update info
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

