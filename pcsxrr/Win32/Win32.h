/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
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

#ifndef __WIN32_H__
#define __WIN32_H__

#define _WIN32_WINDOWS 0x410
#define _WIN32_IE 0x0500

#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <ctype.h>

AppData gApp;
HANDLE hConsole;

long LoadCdBios;
extern int StatesC;
extern int AccBreak;
extern int NeedReset;
extern int ConfPlug;
extern int CancelQuit;
char cfgfile[256];
int Running;
char PcsxDir[256];

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfigureMcdsDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConfigureCpuDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConfigureNetPlayDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ConfigurePlugins(HWND hWnd);

int  Open_File_Proc(char *file);
void Open_Mcd_Proc(HWND hW, int MCDID);
void CreateMainWindow(int nCmdShow);
void RunGui();
void PADhandleKey(int key);

int  LoadConfig();
void SaveConfig();

void UpdateMenuSlots();
void ResetMenuSlots();

void InitLanguages();
char *GetLanguageNext();
void CloseLanguages();
void ChangeLanguage(char *lang);

// maphkeys.c
extern HWND hMHkeysDlg;							// Handle to the MapHotkeys Dialog

int MHkeysUpdate();
int MHkeysCreate();
int MHkeysListMake(int bBuild);

// memwatch.c
void UpdateMemWatch();
void CreateMemWatch();
void AddMemWatch();
extern char * MemWatchDir;

// cheats
void CreateCheatEditor();
void CreateMemSearch();
void UpdateMemSearch();

void PCSX_LoadState(int newState);
void PCSX_SaveState(int newState);
extern int flagSaveState;
extern int flagLoadState;
extern int flagEscPressed;

#endif /* __WIN32_H__ */
