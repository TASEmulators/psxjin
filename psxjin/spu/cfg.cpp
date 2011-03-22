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

#define MAXMODE 2
#define MAXMETHOD 2

void WriteConfig()
{
	char Str_Tmp[1024];

	wsprintf(Str_Tmp, "%d", iUseXA);
	WritePrivateProfileString("Sound", "UseXA", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iVolume);
	WritePrivateProfileString("Sound", "iVolume", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iXAPitch);
	WritePrivateProfileString("Sound", "iXAPitch", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iSoundMode);
	WritePrivateProfileString("Sound", "iSoundMode", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iSynchMethod);
	WritePrivateProfileString("Sound", "iSynchMethod", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iRecordMode);
	WritePrivateProfileString("Sound", "iRecordMode", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iUseReverb);
	WritePrivateProfileString("Sound", "iUseReverb", Str_Tmp, Config.Conf_File);
	wsprintf(Str_Tmp, "%d", iUseInterpolation);
	WritePrivateProfileString("Sound", "iUseInterpolation", Str_Tmp, Config.Conf_File);
}

void ReadConfig()
{
	iUseXA = GetPrivateProfileInt("Sound", "UseXA", 1, Config.Conf_File);
	iVolume = GetPrivateProfileInt("Sound", "iVolume", 3, Config.Conf_File);
	iXAPitch = GetPrivateProfileInt("Sound", "iXAPitch", iXAPitch, Config.Conf_File);
	iSoundMode = GetPrivateProfileInt("Sound", "iSoundMode", iSoundMode, Config.Conf_File);
	iSynchMethod = GetPrivateProfileInt("Sound", "iSynchMethod", iSynchMethod, Config.Conf_File);
	iRecordMode = GetPrivateProfileInt("Sound", "iRecordMode", iRecordMode, Config.Conf_File);
	iUseReverb = GetPrivateProfileInt("Sound", "iUseReverb", iUseReverb, Config.Conf_File);
	iUseInterpolation = GetPrivateProfileInt("Sound", "iUseInterpolation", iUseInterpolation, Config.Conf_File);
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
	tempDest=ComboBox_AddString(hWC, "1: Dual SPU (tas safe, slight glitches [default])");
	tempDest=ComboBox_AddString(hWC, "2: Synchronous (tas safe, buffering glitches)");
	tempDest=ComboBox_SetCurSel(hWC,iSoundMode);

	hWC=GetDlgItem(hW,IDC_SYNCHMETHOD);
	tempDest=ComboBox_AddString(hWC, "Method \"N\" (super cool) [default]");
	tempDest=ComboBox_AddString(hWC, "Method \"Z\" (super strange)");
	tempDest=ComboBox_AddString(hWC, "Method \"P\" (super laggy but magical)");
	tempDest=ComboBox_SetCurSel(hWC,iSynchMethod);

	hWC=GetDlgItem(hW,IDC_VOLUME);
	tempDest=ComboBox_AddString(hWC, "0: mute");
	tempDest=ComboBox_AddString(hWC, "1: low");
	tempDest=ComboBox_AddString(hWC, "2: medium");
	tempDest=ComboBox_AddString(hWC, "3: loud");
	tempDest=ComboBox_AddString(hWC, "4: loudest");
	tempDest=ComboBox_SetCurSel(hWC,5-iVolume);

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
	iSoundMode=ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_SYNCHMETHOD);
	iSynchMethod=ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_VOLUME);
	iVolume=5-ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_USEREVERB);
	iUseReverb=ComboBox_GetCurSel(hWC);

	hWC=GetDlgItem(hW,IDC_INTERPOL);
	iUseInterpolation=ComboBox_GetCurSel(hWC);

	if (IsDlgButtonChecked(hW,IDC_RECORDMODE))
		iRecordMode=1;
	else iRecordMode=0;

	WriteConfig();                                        // write registry

	EndDialog(hW,TRUE);

	SPUReset();
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

#endif
