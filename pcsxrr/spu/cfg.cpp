/***************************************************************************
                            cfg.c  -  description
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
// 2003/06/07 - Pete
// - added Linux NOTHREADLIB define
//
// 2003/02/28 - Pete
// - added option for kode54's interpolation and linuzappz's mono mode
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2002/08/04 - Pete
// - small linux bug fix: now the cfg file can be in the main emu directory as well
//
// 2002/06/08 - linuzappz
// - Added combo str for SPUasync, and MAXMODE is now defined as 2
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_CFG

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// WINDOWS CONFIG/ABOUT HANDLING
////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS

#include "resource.h"


////////////////////////////////////////////////////////////////////////
// READ CONFIG: from win registry
////////////////////////////////////////////////////////////////////////

// timer mode 2 (spuupdate sync mode) can be enabled for windows
// by setting MAXMODE to 2.
// Attention: that mode is not much tested, maybe the dsound buffers
// need to get adjusted to use that mode safely. Also please note:
// sync sound updates will _always_ cause glitches, if the system is
// busy by, for example, long lasting cdrom accesses. OK, you have
// be warned :)

#define MAXMODE 2
//#define MAXMODE 1

void ReadConfig(void)
{
	HKEY myKey;
	DWORD temp;
	DWORD type;
	DWORD size;

	iUseXA=1;                                             // init vars
	iVolume=3;
	iXAPitch=0;
	iUseTimer=0;
	iSPUIRQWait=0;
	iRecordMode=0;
	iUseReverb=1;
	iUseInterpolation=2;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\PCSX-RR\\SPU",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
	{
		size = 4;
		if (RegQueryValueEx(myKey,"UseXA",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iUseXA=(int)temp;
		size = 4;
		if (RegQueryValueEx(myKey,"Volume",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iVolume=(int)temp;
		size = 4;
		if (RegQueryValueEx(myKey,"XAPitch",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iXAPitch=(int)temp;
		size = 4;
		if(RegQueryValueEx(myKey,"UseTimer",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iUseTimer=(int)temp;
		size = 4;
		if (RegQueryValueEx(myKey,"SPUIRQWait",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iSPUIRQWait=(int)temp;
		size = 4;
		if (RegQueryValueEx(myKey,"RecordMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iRecordMode=(int)temp;
		size = 4;
		if (RegQueryValueEx(myKey,"UseReverb",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iUseReverb=(int)temp;
		if(iUseReverb==2) iUseReverb=1; //old reverb mode was removed
		size = 4;
		if (RegQueryValueEx(myKey,"UseInterpolation",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
			iUseInterpolation=(int)temp;
		size = 4;

		RegCloseKey(myKey);
	}

 if(iUseTimer>MAXMODE) iUseTimer=MAXMODE;              // some checks
	if (iVolume<1) iVolume=1;
	if (iVolume>5) iVolume=5;
}

////////////////////////////////////////////////////////////////////////
// WRITE CONFIG: in win registry
////////////////////////////////////////////////////////////////////////

void WriteConfig(void)
{
	HKEY myKey;
	DWORD myDisp;
	DWORD temp;

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\PCSX-RR\\SPU",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&myDisp);
	temp=iUseXA;
	RegSetValueEx(myKey,"UseXA",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iVolume;
	RegSetValueEx(myKey,"Volume",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iXAPitch;
	RegSetValueEx(myKey,"XAPitch",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseTimer;
 RegSetValueEx(myKey,"UseTimer",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iSPUIRQWait;
	RegSetValueEx(myKey,"SPUIRQWait",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iRecordMode;
	RegSetValueEx(myKey,"RecordMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iUseReverb;
	RegSetValueEx(myKey,"UseReverb",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
	temp=iUseInterpolation;
	RegSetValueEx(myKey,"UseInterpolation",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));

	RegCloseKey(myKey);
}

////////////////////////////////////////////////////////////////////////
// INIT WIN CFG DIALOG
////////////////////////////////////////////////////////////////////////

BOOL OnInitDSoundDialog(HWND hW)
{
	HWND hWC;
	int tempDest;

	ReadConfig();

	if (iUseXA)      CheckDlgButton(hW,IDC_ENABXA,TRUE);

	if (iXAPitch)    CheckDlgButton(hW,IDC_XAPITCH,TRUE);

	hWC=GetDlgItem(hW,IDC_USETIMER);
	tempDest=ComboBox_AddString(hWC, "0: Asynchronous (normal, tas UNSAFE)");
	tempDest=ComboBox_AddString(hWC, "1: Dual SPU Synch/Asynch (tas safe, doesnt work yet)");
	tempDest=ComboBox_AddString(hWC, "2: Synchronous (tas safe, sometimes needed for streams)");
	tempDest=ComboBox_SetCurSel(hWC,iUseTimer);

	hWC=GetDlgItem(hW,IDC_VOLUME);
	tempDest=ComboBox_AddString(hWC, "0: mute");
	tempDest=ComboBox_AddString(hWC, "1: low");
	tempDest=ComboBox_AddString(hWC, "2: medium");
	tempDest=ComboBox_AddString(hWC, "3: loud");
	tempDest=ComboBox_AddString(hWC, "4: loudest");
	tempDest=ComboBox_SetCurSel(hWC,5-iVolume);

	if (iSPUIRQWait)  CheckDlgButton(hW,IDC_IRQWAIT,TRUE);
	if (iRecordMode)  CheckDlgButton(hW,IDC_RECORDMODE,TRUE);

	hWC=GetDlgItem(hW,IDC_USEREVERB);
	tempDest=ComboBox_AddString(hWC, "0: Don't Output Reverb (but process it anyway to be safe)");
	tempDest=ComboBox_AddString(hWC, "1: Output Reverb");
	tempDest=ComboBox_SetCurSel(hWC,iUseReverb);

	hWC=GetDlgItem(hW,IDC_INTERPOL);
	tempDest=ComboBox_AddString(hWC, "0: None (fastest)");
	tempDest=ComboBox_AddString(hWC, "1: Linear interpolation (typical)");
	tempDest=ComboBox_AddString(hWC, "2: Gaussian interpolation (good quality)");
	tempDest=ComboBox_AddString(hWC, "3: Cubic interpolation (better treble)");
	tempDest=ComboBox_AddString(hWC, "4: Cosine interpolation (no comment)");
	tempDest=ComboBox_SetCurSel(hWC,iUseInterpolation);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG OK
////////////////////////////////////////////////////////////////////////

void OnDSoundOK(HWND hW)
{
	HWND hWC;

	if (IsDlgButtonChecked(hW,IDC_ENABXA))
		iUseXA=1;
	else iUseXA=0;

	if (IsDlgButtonChecked(hW,IDC_XAPITCH))
		iXAPitch=1;
	else iXAPitch=0;

	hWC=GetDlgItem(hW,IDC_USETIMER);
	iUseTimer=ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_VOLUME);
	iVolume=5-ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_USEREVERB);
	iUseReverb=ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_INTERPOL);
	iUseInterpolation=ComboBox_GetCurSel(hWC);

	if (IsDlgButtonChecked(hW,IDC_IRQWAIT))
		iSPUIRQWait=1;
	else iSPUIRQWait=0;

	if (IsDlgButtonChecked(hW,IDC_RECORDMODE))
		iRecordMode=1;
	else iRecordMode=0;

	WriteConfig();                                        // write registry

	EndDialog(hW,TRUE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG CANCEL
////////////////////////////////////////////////////////////////////////

void OnDSoundCancel(HWND hW)
{
	EndDialog(hW,FALSE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG PROC
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DSoundDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDSoundDialog(hW);

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			OnDSoundCancel(hW);
			return TRUE;
		case IDOK:
			OnDSoundOK(hW);
			return TRUE;
		}
	}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// LINUX CONFIG/ABOUT HANDLING
////////////////////////////////////////////////////////////////////////

#else

char * pConfigFile=NULL;

#include <unistd.h>

////////////////////////////////////////////////////////////////////////
// START EXTERNAL CFG TOOL
////////////////////////////////////////////////////////////////////////

void StartCfgTool(char * pCmdLine)
{
	FILE * cf;
	char filename[255],t[255];

	strcpy(filename,"cfg/cfgPeopsOSS");
	cf=fopen(filename,"rb");
	if (cf!=NULL)
	{
		fclose(cf);
		getcwd(t,255);
		chdir("cfg");
		sprintf(filename,"./cfgPeopsOSS %s",pCmdLine);
		system(filename);
		chdir(t);
	}
	else
	{
		strcpy(filename,"cfgPeopsOSS");
		cf=fopen(filename,"rb");
		if (cf!=NULL)
		{
			fclose(cf);
			sprintf(filename,"./cfgPeopsOSS %s",pCmdLine);
			system(filename);
		}
		else
		{
			sprintf(filename,"%s/cfgPeopsOSS",getenv("HOME"));
			cf=fopen(filename,"rb");
			if (cf!=NULL)
			{
				fclose(cf);
				getcwd(t,255);
				chdir(getenv("HOME"));
				sprintf(filename,"./cfgPeopsOSS %s",pCmdLine);
				system(filename);
				chdir(t);
			}
			else printf("cfgPeopsOSS not found!\n");
		}
	}
}

/////////////////////////////////////////////////////////
// READ LINUX CONFIG FILE
/////////////////////////////////////////////////////////

void ReadConfigFile(void)
{
	FILE *in;
	char t[256];
	int len;
	char * pB, * p;

	if (pConfigFile)
	{
		strcpy(t,pConfigFile);
		in = fopen(t,"rb");
		if (!in) return;
	}
	else
	{
		strcpy(t,"cfg/spuPeopsOSS.cfg");
		in = fopen(t,"rb");
		if (!in)
		{
			strcpy(t,"spuPeopsOSS.cfg");
			in = fopen(t,"rb");
			if (!in)
			{
				sprintf(t,"%s/spuPeopsOSS.cfg",getenv("HOME"));
				in = fopen(t,"rb");
				if (!in) return;
			}
		}
	}

	pB=(char *)malloc(32767);
	memset(pB,0,32767);

	len = fread(pB, 1, 32767, in);
	fclose(in);

	strcpy(t,"\nVolume");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p) iVolume=atoi(p+len);
	if (iVolume<1) iVolume=1;
	if (iVolume>5) iVolume=5;

	strcpy(t,"\nUseXA");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p) iUseXA=atoi(p+len);
	if (iUseXA<0) iUseXA=0;
	if (iUseXA>1) iUseXA=1;

	strcpy(t,"\nXAPitch");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p) iXAPitch=atoi(p+len);
	if (iXAPitch<0) iXAPitch=0;
	if (iXAPitch>1) iXAPitch=1;

	strcpy(t,"\nSPUIRQWait");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p)  iSPUIRQWait=atoi(p+len);
	if (iSPUIRQWait<0) iSPUIRQWait=0;
	if (iSPUIRQWait>1) iSPUIRQWait=1;

	strcpy(t,"\nUseReverb");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p)  iUseReverb=atoi(p+len);
	if (iUseReverb<0) iUseReverb=0;
	if (iUseReverb>2) iUseReverb=2;

	strcpy(t,"\nUseInterpolation");
	p=strstr(pB,t);
	if (p)
	{
		p=strstr(p,"=");
		len=1;
	}
	if (p)  iUseInterpolation=atoi(p+len);
	if (iUseInterpolation<0) iUseInterpolation=0;
	if (iUseInterpolation>3) iUseInterpolation=3;

	free(pB);
}

/////////////////////////////////////////////////////////
// READ CONFIG called by spu funcs
/////////////////////////////////////////////////////////

void ReadConfig(void)
{
	iVolume=3;
	iUseXA=1;
	iXAPitch=0;
	iSPUIRQWait=1;
	iUseTimer=0;
	iUseReverb=2;
	iUseInterpolation=2;
	iDisStereo=0;

	ReadConfigFile();
}

#endif


