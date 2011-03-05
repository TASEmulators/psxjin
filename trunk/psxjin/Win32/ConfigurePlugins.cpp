/*  PSXjin - Pc Psx Emulator
 *  Copyright (C) 1999-2003  PSXjin Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "PsxCommon.h"
#include "plugin.h"
#include "resource.h"
#include "Win32.h"
#include "maphkeys.h"

int tempDest; //this is for the compiler to not throw in a million of warnings

//adelikat: Changed from storing into the registry to saving into a config file
void SaveConfig()
{
	char Str_Tmp[1024];
	char Conf_File[1024] = ".\\PSXjin.ini";	//TODO: make a global for other files
	
	WritePrivateProfileString("Plugins", "Bios", Config.Bios, Conf_File);
	WritePrivateProfileString("Plugins", "GPU", Config.Gpu , Conf_File);
	WritePrivateProfileString("Plugins", "SPU", Config.Spu , Conf_File);
	WritePrivateProfileString("Plugins", "CDR", Config.Cdr , Conf_File);
	WritePrivateProfileString("Plugins", "Pad1", Config.Pad1 , Conf_File);
	WritePrivateProfileString("Plugins", "Pad2", Config.Pad2 , Conf_File);
	WritePrivateProfileString("Plugins", "MCD1", Config.Mcd1 , Conf_File);
	WritePrivateProfileString("Plugins", "MCD2", Config.Mcd2 , Conf_File);
	wsprintf(Str_Tmp, "%d", Config.Xa);
	WritePrivateProfileString("Plugins", "Xa", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.Sio);
	WritePrivateProfileString("Plugins", "Sio", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.Mdec);
	WritePrivateProfileString("Plugins", "Mdec", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.PsxAuto);
	WritePrivateProfileString("Plugins", "PsxAuto", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.PsxType);
	WritePrivateProfileString("Plugins", "PsxType", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.QKeys);
	WritePrivateProfileString("Plugins", "QKeys", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.Cdda);
	WritePrivateProfileString("Plugins", "Cdda", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.PauseAfterPlayback);
	WritePrivateProfileString("Plugins", "PauseAfterPlayback", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.PsxOut);
	WritePrivateProfileString("Plugins", "PsxOut", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.RCntFix);
	WritePrivateProfileString("Plugins", "RCntFix", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Config.VSyncWA);
	WritePrivateProfileString("Plugins", "VSyncWA", Str_Tmp, Conf_File);

	for (int i = 0; i <= EMUCMDMAX; i++) 
	{
		wsprintf(Str_Tmp, "%d", EmuCommandTable[i].key);
		WritePrivateProfileString("Hotkeys", EmuCommandTable[i].name, Str_Tmp, Conf_File);
	}

	for (int i = 0; i <= EMUCMDMAX; i++)
	{
		wsprintf(Str_Tmp, "%d", EmuCommandTable[i].keymod);
		WritePrivateProfileString("HotkeysKeyMods", EmuCommandTable[i].name, Str_Tmp, Conf_File);
	}
}

void LoadConfig()
{
	char Conf_File[1024] = ".\\PSXjin.ini";	//TODO: make a global for other files

	GetPrivateProfileString("Plugins", "Bios", "scph1001.bin", &Config.Bios[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "GPU", "gpuTASsoft.dll", &Config.Gpu[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "SPU", "", &Config.Spu[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "CDR", "cdrTASiso.dll", &Config.Cdr[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "Pad1", "padSeguDPP.dll", &Config.Pad1[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "Pad2", "padSeguDPP.dll", &Config.Pad2[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "MCD1", "", &Config.Mcd1[0], 256, Conf_File);
	GetPrivateProfileString("Plugins", "MCD2", "", &Config.Mcd2[0], 256, Conf_File);
	Config.Xa = GetPrivateProfileInt("Plugins", "Xa", 0, Conf_File);
	Config.Mdec = GetPrivateProfileInt("Plugins", "Mdec", 0, Conf_File);
	Config.PsxAuto = GetPrivateProfileInt("Plugins", "PsxAuto", 0, Conf_File);
	Config.PsxType = GetPrivateProfileInt("Plugins", "PsxType", 0, Conf_File);
	Config.QKeys = GetPrivateProfileInt("Plugins", "QKeys", 0, Conf_File);
	Config.Cdda = GetPrivateProfileInt("Plugins", "Cdda", 0, Conf_File);
	Config.PauseAfterPlayback = GetPrivateProfileInt("Plugins", "PauseAfterPlayback", 0, Conf_File);
	Config.PsxOut = GetPrivateProfileInt("Plugins", "PsxOut", 0, Conf_File);
	Config.RCntFix = GetPrivateProfileInt("Plugins", "RCntFix", 0, Conf_File);
	Config.VSyncWA = GetPrivateProfileInt("Plugins", "VSyncWA", 0, Conf_File);

	int temp;
	for (int i = 0; i <= EMUCMDMAX-1; i++)
	{
		temp = GetPrivateProfileInt("Hotkeys", EmuCommandTable[i].name, 65535, Conf_File);
		if (temp != 65535)
			EmuCommandTable[i].key = temp;
	}

	
	for (int i = 0; i <= EMUCMDMAX-1; i++) 
	{
		temp = GetPrivateProfileInt("HotkeysKeyMods", EmuCommandTable[i].name, 65535, Conf_File);
		if (temp != 65535)
			EmuCommandTable[i].keymod = temp;
	}
}

/////////////////////////////////////////////////////////

#define ComboAddPlugin(hw, str) { \
	lp = (char *)malloc(strlen(FindData.cFileName)+8); \
	sprintf(lp, "%s", FindData.cFileName); \
	i = ComboBox_AddString(hw, tmpStr); \
	tempDest = ComboBox_SetItemData(hw, i, lp); \
	if (_stricmp(str, lp)==0) \
		tempDest = ComboBox_SetCurSel(hw, i); \
}

BOOL OnConfigurePluginsDialog(HWND hW) {
	WIN32_FIND_DATA FindData;
	HANDLE Find;
	HANDLE Lib;
	PSEgetLibType    PSE_GetLibType;
	PSEgetLibName    PSE_GetLibName;
	PSEgetLibVersion PSE_GetLibVersion;
	HWND hWC_GPU=GetDlgItem(hW,IDC_LISTGPU);
	HWND hWC_SPU=GetDlgItem(hW,IDC_LISTSPU);
	HWND hWC_CDR=GetDlgItem(hW,IDC_LISTCDR);
	HWND hWC_PAD1=GetDlgItem(hW,IDC_LISTPAD1);
	HWND hWC_PAD2=GetDlgItem(hW,IDC_LISTPAD2);
	HWND hWC_BIOS=GetDlgItem(hW,IDC_LISTBIOS);
	char tmpStr[256];
	char *lp;
	int i;

	strcpy(tmpStr, Config.PluginsDir);
	strcat(tmpStr, "*.dll");
	Find = FindFirstFile(tmpStr, &FindData);

	do {
		if (Find==INVALID_HANDLE_VALUE) break;
		sprintf(tmpStr,"%s%s", Config.PluginsDir, FindData.cFileName);
		Lib = LoadLibrary(tmpStr);
		if (Lib!=NULL) {
			PSE_GetLibType = (PSEgetLibType) GetProcAddress((HMODULE)Lib,"PSEgetLibType");
			PSE_GetLibName = (PSEgetLibName) GetProcAddress((HMODULE)Lib,"PSEgetLibName");
			PSE_GetLibVersion = (PSEgetLibVersion) GetProcAddress((HMODULE)Lib,"PSEgetLibVersion");

			if (PSE_GetLibType != NULL && PSE_GetLibName != NULL && PSE_GetLibVersion != NULL) {
				unsigned long version = PSE_GetLibVersion();
				long type;

				sprintf(tmpStr, "%s %d.%d", PSE_GetLibName(), (int)(version>>8)&0xff, (int)version&0xff);
				type = PSE_GetLibType();
				if (type & PSE_LT_CDR) {
					ComboAddPlugin(hWC_CDR, Config.Cdr);
				}

				if (type & PSE_LT_SPU) {
					ComboAddPlugin(hWC_SPU, Config.Spu);
				}

				if (type & PSE_LT_GPU) {
					ComboAddPlugin(hWC_GPU, Config.Gpu);
				}

				if (type & PSE_LT_PAD) {
					PADquery query;

					query = (PADquery)GetProcAddress((HMODULE)Lib, "PADquery");
					if (query != NULL) {
						if (query() & 0x1)
							ComboAddPlugin(hWC_PAD1, Config.Pad1);
						if (query() & 0x2)
							ComboAddPlugin(hWC_PAD2, Config.Pad2);
					} else { // just a guess
						ComboAddPlugin(hWC_PAD1, Config.Pad1);
					}
				}
			}
		}
	} while (FindNextFile(Find,&FindData));

	if (Find!=INVALID_HANDLE_VALUE) FindClose(Find);

// BIOS

	lp=(char *)malloc(strlen("HLE") + 1);
	sprintf(lp, "HLE");
	i=ComboBox_AddString(hWC_BIOS, "Internal HLE Bios");
	tempDest = ComboBox_SetItemData(hWC_BIOS, i, lp);
	if (_stricmp(Config.Bios, lp)==0)
		tempDest = ComboBox_SetCurSel(hWC_BIOS, i);

	strcpy(tmpStr, Config.BiosDir);
	strcat(tmpStr, "*");
	Find=FindFirstFile(tmpStr, &FindData);

	do {
		if (Find==INVALID_HANDLE_VALUE) break;
		if (!strcmp(FindData.cFileName, ".")) continue;
		if (!strcmp(FindData.cFileName, "..")) continue;
		if (FindData.nFileSizeLow != 1024 * 512) continue;
		lp = (char *)malloc(strlen(FindData.cFileName)+8);
		sprintf(lp, "%s", (char *)FindData.cFileName);
		i = ComboBox_AddString(hWC_BIOS, FindData.cFileName);
		tempDest = ComboBox_SetItemData(hWC_BIOS, i, lp);
		if (_stricmp(Config.Bios, FindData.cFileName)==0)
			tempDest = ComboBox_SetCurSel(hWC_BIOS, i);
	} while (FindNextFile(Find,&FindData));

	if (Find!=INVALID_HANDLE_VALUE) FindClose(Find);

	if (ComboBox_GetCurSel(hWC_CDR ) == -1)
		tempDest = ComboBox_SetCurSel(hWC_CDR,  0);
	if (ComboBox_GetCurSel(hWC_GPU ) == -1)
		tempDest = ComboBox_SetCurSel(hWC_GPU,  0);
	if (ComboBox_GetCurSel(hWC_SPU ) == -1)
		tempDest = ComboBox_SetCurSel(hWC_SPU,  0);
	if (ComboBox_GetCurSel(hWC_PAD1) == -1)
		tempDest = ComboBox_SetCurSel(hWC_PAD1, 0);
	if (ComboBox_GetCurSel(hWC_PAD2) == -1)
		tempDest = ComboBox_SetCurSel(hWC_PAD2, 0);
	if (ComboBox_GetCurSel(hWC_BIOS) == -1)
		tempDest = ComboBox_SetCurSel(hWC_BIOS, 0);

	return TRUE;
}
	
#define CleanCombo(item) \
	hWC = GetDlgItem(hW, item); \
	iCnt = ComboBox_GetCount(hWC); \
	for (i=0; i<iCnt; i++) { \
		lp = (char *)ComboBox_GetItemData(hWC, i); \
		if (lp) free(lp); \
	} \
	tempDest = ComboBox_ResetContent(hWC);

void CleanUpCombos(HWND hW) {
	int i,iCnt;HWND hWC;char * lp;

	CleanCombo(IDC_LISTGPU);
	CleanCombo(IDC_LISTSPU);
	CleanCombo(IDC_LISTCDR);
	CleanCombo(IDC_LISTPAD1);
	CleanCombo(IDC_LISTPAD2);
	CleanCombo(IDC_LISTBIOS);
}


void OnCancel(HWND hW) {
	CleanUpCombos(hW);
	EndDialog(hW,FALSE);
}


char *GetSelDLL(HWND hW,int id) {
	HWND hWC = GetDlgItem(hW,id);
	int iSel;
	iSel = ComboBox_GetCurSel(hWC);
	if (iSel<0) return NULL;
	return (char *)ComboBox_GetItemData(hWC, iSel);
}


void OnOK(HWND hW) {
	char * pad1DLL=GetSelDLL(hW,IDC_LISTPAD1);
	char * pad2DLL=GetSelDLL(hW,IDC_LISTPAD2);
	char * biosFILE=GetSelDLL(hW,IDC_LISTBIOS);

    if ((pad1DLL==NULL) ||
	   (pad2DLL==NULL) ||(biosFILE==NULL)) {
		MessageBox(hW,"Configuration not OK!","Error",MB_OK|MB_ICONERROR);
		return;
	}

	strcpy(Config.Bios, biosFILE);
	strcpy(Config.Pad1, pad1DLL);
	strcpy(Config.Pad2, pad2DLL);

	SaveConfig();

	CleanUpCombos(hW);

	if (!ConfPlug) {
		NeedReset = 1;
		ReleasePlugins();
		LoadPlugins();
	}
	EndDialog(hW,TRUE);
}


#define ConfPlugin(src, confs, name) \
	void *drv; \
	src conf; \
	char * pDLL = GetSelDLL(hW, confs); \
	char file[256]; \
	if(pDLL==NULL) return; \
	strcpy(file, Config.PluginsDir); \
	strcat(file, pDLL); \
	drv = SysLoadLibrary(file); \
	if (drv == NULL) return; \
	conf = (src) SysLoadSym(drv, name); \
	if (SysLibError() == NULL) conf(); \
	SysCloseLibrary(drv);

void ConfigureGPU(HWND hW) {
	GPUconfigure();
}

void ConfigureSPU(HWND hW) {
	SPUconfigure();
}

void ConfigurePAD1(HWND hW) {
	ConfPlugin(PADconfigure, IDC_LISTPAD1, "PADconfigure");
}

void ConfigurePAD2(HWND hW) {
	ConfPlugin(PADconfigure, IDC_LISTPAD2, "PADconfigure");
}

void AboutPAD1(HWND hW) {
	ConfPlugin(PADabout, IDC_LISTPAD1, "PADabout");
}

void AboutPAD2(HWND hW) {
	ConfPlugin(PADabout, IDC_LISTPAD2, "PADabout");
}


#define TestPlugin(src, confs, name) \
	void *drv; \
	src conf; \
	int ret = 0; \
	char * pDLL = GetSelDLL(hW, confs); \
	char file[256]; \
	if (pDLL== NULL) return; \
	strcpy(file, Config.PluginsDir); \
	strcat(file, pDLL); \
	drv = SysLoadLibrary(file); \
	if (drv == NULL) return; \
	conf = (src) SysLoadSym(drv, name); \
	if (SysLibError() == NULL) { \
		ret = conf(); \
		if (ret == 0) \
			 SysMessage(_("This plugin reports that should work correctly")); \
		else SysMessage(_("This plugin reports that should not work correctly")); \
	} \
	SysCloseLibrary(drv);

void TestPAD1(HWND hW) {
	TestPlugin(PADtest, IDC_LISTPAD1, "PADtest");
}

void TestPAD2(HWND hW) {
	TestPlugin(PADtest, IDC_LISTPAD2, "PADtest");
}

#include <shlobj.h>

int SelectPath(HWND hW, char *Title, char *Path) {
	LPITEMIDLIST pidl;
	BROWSEINFO bi;
	char Buffer[256];

	bi.hwndOwner = hW;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;
	bi.lpszTitle = Title;
	bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = 0;
	if ((pidl = SHBrowseForFolder(&bi)) != NULL) {
		if (SHGetPathFromIDList(pidl, Path)) {
			int len = strlen(Path);

			if (Path[len - 1] != '\\') { strcat(Path,"\\"); }
			return 0;
		}
	}
	return -1;
}

BOOL CALLBACK ConfigurePluginsDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Configuration"));

			Button_SetText(GetDlgItem(hW, IDOK), _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("Cancel"));
			Static_SetText(GetDlgItem(hW, IDC_GRAPHICS), _("Graphics"));
			Static_SetText(GetDlgItem(hW, IDC_FIRSTCONTROLLER), _("First Controller"));
			Static_SetText(GetDlgItem(hW, IDC_SECONDCONTROLLER), _("Second Controller"));
			Static_SetText(GetDlgItem(hW, IDC_SOUND), _("Sound"));
			Static_SetText(GetDlgItem(hW, IDC_CDROM), _("CD-ROM"));
			Static_SetText(GetDlgItem(hW, IDC_BIOS), _("Bios"));
			Button_SetText(GetDlgItem(hW, IDC_CONFIGGPU), _("Configure..."));
			Button_SetText(GetDlgItem(hW, IDC_TESTGPU), _("Test..."));
			Button_SetText(GetDlgItem(hW, IDC_ABOUTGPU), _("About..."));
			Button_SetText(GetDlgItem(hW, IDC_CONFIGSPU), _("Configure..."));
			Button_SetText(GetDlgItem(hW, IDC_TESTSPU), _("Test..."));
			Button_SetText(GetDlgItem(hW, IDC_ABOUTSPU), _("About..."));
			Button_SetText(GetDlgItem(hW, IDC_CONFIGCDR), _("Configure..."));
			Button_SetText(GetDlgItem(hW, IDC_TESTCDR), _("Test..."));
			Button_SetText(GetDlgItem(hW, IDC_ABOUTCDR), _("About..."));
			Button_SetText(GetDlgItem(hW, IDC_CONFIGPAD1), _("Configure..."));
			Button_SetText(GetDlgItem(hW, IDC_TESTPAD1), _("Test..."));
			Button_SetText(GetDlgItem(hW, IDC_ABOUTPAD1), _("About..."));
			Button_SetText(GetDlgItem(hW, IDC_CONFIGPAD2), _("Configure..."));
			Button_SetText(GetDlgItem(hW, IDC_TESTPAD2), _("Test..."));
			Button_SetText(GetDlgItem(hW, IDC_ABOUTPAD2), _("About..."));

			return OnConfigurePluginsDialog(hW);

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_CONFIGGPU:  ConfigureGPU(hW); return TRUE;
       			case IDC_CONFIGSPU:  ConfigureSPU(hW); return TRUE;
       			case IDC_CONFIGPAD1: ConfigurePAD1(hW); return TRUE;
       			case IDC_CONFIGPAD2: ConfigurePAD2(hW); return TRUE;
				case IDC_TESTPAD1:   TestPAD1(hW);  return TRUE;
				case IDC_TESTPAD2:   TestPAD2(hW);  return TRUE;
    	        case IDC_ABOUTPAD1:  AboutPAD1(hW); return TRUE;
    		    case IDC_ABOUTPAD2:  AboutPAD2(hW); return TRUE;

				case IDCANCEL: 
					OnCancel(hW); 
					if (CancelQuit) {
						SysClose(); exit(1);
					}
					return TRUE;
				case IDOK:     
					OnOK(hW);     
					return TRUE;
			}
	}
	return FALSE;
}


void ConfigurePlugins(HWND hWnd) {
    DialogBox(gApp.hInstance,
              MAKEINTRESOURCE(IDD_CONFIG),
              hWnd,  
              (DLGPROC)ConfigurePluginsDlgProc);
}