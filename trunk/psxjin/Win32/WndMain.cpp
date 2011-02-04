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
#include <commctrl.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <string>

#include <fcntl.h>
#include <io.h>

#include "resource.h"
#include "AboutDlg.h"

#include "../version.h"
#include "PsxCommon.h"
#include "plugin.h"
#include "Debug.h"
#include "Win32.h"
#include "../cheat.h"
#include "../movie.h"
#include "moviewin.h"
#include "ram_search.h"
#include "ramwatch.h"
#include "CWindow.h"
#include "memView.h"
#include "../spu/spu.h"	//For iVolume
#include "recentmenu.h"
#include "plugins.h"
#include "analog.h"

const int RECENTCD_START = 65000;
const int RECENTMOVIE_START = 65020;
const int RECENTLUA_START = 65040;

extern HWND LuaConsoleHWnd;
extern INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern void ReadConfig();	//SPU save/load
extern void WriteConfig();

//Prototypes
void RunCD(HWND hWnd);
void SaveIni();
void UpdateMenuHotkeys();
// global variables
AppData gApp;
HANDLE hConsole;
long LoadCdBios;
int Running;
int Continue=0;
char PSXjinDir[256];
bool AVIisCapturing = false;

//TODO: remove me and use the gpu ones!
int dispInput = 0;
int dispFrameCounter = 0;
int dispAllText = 0;
//

int MainWindow_wndx = 0;
int MainWindow_wndy = 0;
int MainWindow_width = 320;		//adelikat Setting width/height to values here is moot since it will set a default value when reading from the config, but hey, why not
int MainWindow_height = 240;
int MainWindow_menubar = 48;
//const int MENUSIZE = 48;

// Recent Menus
RecentMenu RecentCDs;
RecentMenu RecentMovies;
RecentMenu RecentLua;

extern bool OpenPlugins(HWND hWnd);

int iSaveStateTo;
int iLoadStateFrom;
int iCallW32Gui;
char szCurrentPath[256];
char szMovieToLoad[256];

uint32 mousex,mousey;

HWND memPokeHWND = NULL;
char bufPokeAddress[7];
char bufPokeNewval[7];

#ifdef __MINGW32__
#ifndef LVM_GETSELECTIONMARK
#define LVM_GETSELECTIONMARK (LVM_FIRST+66)
#endif
#ifndef ListView_GetSelectionMark
#define ListView_GetSelectionMark(w) (INT)SNDMSG((w),LVM_GETSELECTIONMARK,0,0)
#endif
#endif

#include "maphkeys.h"
int iTurboMode;

int AccBreak=0;
int ConfPlug=0;
int StatesC=0;
int NeedReset=1;
int cdOpenCase=0;
int cheatsEnabled=0;
int CancelQuit=0;
int UseGui = 1;

extern char pConfigFile[MAX_PATH*2];
#define MAXFILENAME 256

int GetMenuSize()
{
	POINT Point={0,0};
	RECT Rect = {0,0};
	ClientToScreen(gApp.hWnd,&Point);
	GetWindowRect(gApp.hWnd,&Rect);
	MainWindow_menubar = Point.y-Rect.top+1;
	return MainWindow_menubar;
}


void strcatz(char *dst, char *src) {
	int len = strlen(dst) + 1;
	strcpy(dst + len, src);
}

void GetCurrentPath()
{
	char szPath[256];
	char szDrive[256];
	char szDirectory[256];
	char szFilename[256];
	char szExt[256];
	szDrive[0] = '\0';
	szDirectory[0] = '\0';
	szFilename[0] = '\0';
	szExt[0] = '\0';
	GetModuleFileName(NULL, szPath, 256);
	_splitpath(szPath, szDrive, szDirectory, szFilename, szExt);

	sprintf(szCurrentPath,"%s%s" + '\0',szDrive, szDirectory);
}

PCHAR*    CommandLineToArgvA( PCHAR CmdLine, int* _argc)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG  len;
	ULONG  argc;
	CHAR   a;
	ULONG  i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED, i + (len+2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while(( a = CmdLine[i] )) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = FALSE;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

int main(int argc, char **argv) {
	TIMECAPS tc;
	DWORD wmTimerRes;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS))== TIMERR_NOERROR)
	{
		wmTimerRes = std::min(std::max(tc.wPeriodMin, (UINT)1), tc.wPeriodMax);
		timeBeginPeriod (wmTimerRes);
	}
	else
	{
		wmTimerRes = 5;
		timeBeginPeriod (wmTimerRes);
	}

	char runexe=0;
	char runcd=0;
	char loadMovie=0;
	int i;
	bool luaLoaded = false;
	printf ("PSXjin\n");

	argv = CommandLineToArgvA(GetCommandLine(), &argc);
	int runcdarg=-2;
	if( argc > 1 )
	for( i=1; i < argc; i++ ) {
		if(!strcmp(argv[i], "-runexe"))
		{ runexe = 1; runcdarg = i; }
		else if (!strcmp(argv[i], "-runcd"))
		{ runcd = 1; runcdarg = i; }
		else if (!strcmp(argv[i], "-play")) {
			loadMovie = 1;
			sprintf(szMovieToLoad,"%s",argv[++i]);
		}
		else if (!strcmp(argv[i], "-dumpavi")) {
			Movie.startAvi = 1;
			sprintf(Movie.aviFilename,"%s",argv[++i]);
		}
		else if (!strcmp(argv[i], "-dumpwav")) {
			Movie.startWav = 1;
			sprintf(Movie.wavFilename,"%s",argv[++i]);
		}
		else if (!strcmp(argv[i], "-lua")) {
			luaLoaded = true;
			RecentLua.UpdateRecentItems(argv[++i]);
			PSXjin_LoadLuaCode(argv[++i]);
		}
		else if (!strcmp(argv[i], "-stopcapture"))
			sscanf (argv[++i],"%lu",&Movie.stopCapture);
		else if (!strcmp(argv[i], "-readonly"))
			Movie.readOnly = 1;
		else if(i==runcdarg+1)
		{
			CDR_iso_fileToOpen = argv[i];
		}
	}

	GetCurrentPath();

	gApp.hInstance = GetModuleHandle(NULL);

	Running=0;

	GetCurrentDirectory(256, PSXjinDir);

	memset(&Config, 0, sizeof(psxjinconfig));
	sprintf(Config.PluginsDir, "%splugins\\", szCurrentPath);
	sprintf(Config.BiosDir, "%sbios\\", szCurrentPath);
	sprintf(Config.SstatesDir, "%ssstates\\", szCurrentPath);
	sprintf(Config.SnapDir, "%ssnap\\", szCurrentPath);
	sprintf(Config.MovieDir, "%smovies\\", szCurrentPath);
	sprintf(Config.MemCardsDir, "%smemcards\\", szCurrentPath);
	sprintf(Config.Conf_File, "%s\\psxjin.ini", szCurrentPath);
	LoadConfig();	//Attempt to load ini, or set default settings
	Config.enable_extern_analog = false;
	Config.WriteAnalog = false;
	strcpy (pConfigFile, Config.Conf_File);

	//If directories don't already exist, create them
	CreateDirectory(Config.SstatesDir, 0);	
	CreateDirectory(Config.SnapDir, 0);
	CreateDirectory(Config.BiosDir, 0);
	CreateDirectory(Config.PluginsDir, 0);
	CreateDirectory(Config.SstatesDir, 0);
	CreateDirectory(Config.MovieDir, 0);
	CreateDirectory(Config.MemCardsDir, 0);

	if (SysInit() == -1) return 1;

	CreateMainWindow(SW_SHOW);

	PSXjinInitCheatData();

	RecentCDs.GetRecentItemsFromIni(Config.Conf_File, "General");
	RecentMovies.GetRecentItemsFromIni(Config.Conf_File, "General");
	RecentLua.GetRecentItemsFromIni(Config.Conf_File, "General");

	char Str[MAX_PATH];
	

	if (!luaLoaded && RecentLua.GetAutoLoad()) //If lua wasn't loaded from command line (commandline should override autoload parameters)
			PSXjin_LoadLuaCode(RecentLua.GetRecentItem(0).c_str());

	//adelikat
	//Set IsoFile, then load movie, then Run Iso, seems messy but it is the easiest way to deal with how these functions are implemented

	if(!runexe)
	{
		if (runcd == 1 || runcd == 2)
			strcpy(IsoFile, CDR_iso_fileToOpen.c_str());
		else if (RecentCDs.GetAutoLoad())
			strcpy(IsoFile, RecentCDs.GetRecentItem(0).c_str());
	}

	if (loadMovie)
	{
		WIN32_StartMovieReplay(szMovieToLoad);
	}
	else if (RecentMovies.GetAutoLoad())
	{
		strcpy(Str, RecentMovies.GetRecentItem(0).c_str());
		WIN32_StartMovieReplay(Str);
	}

	//process some command line options
	if (runcd == 1 || runcd == 2)
	{
		strcpy(IsoFile, CDR_iso_fileToOpen.c_str());
		RunCD(gApp.hWnd);
	}
	else if (RecentCDs.GetAutoLoad())
	{
		strcpy(IsoFile, RecentCDs.GetRecentItem(0).c_str());
		RunCD(gApp.hWnd);
	}

	if(runexe)
	{
		OpenPlugins(gApp.hWnd);
		SysReset();
		NeedReset = 0;
		Load((char*)CDR_iso_fileToOpen.c_str());
		Running = 1;
		psxCpu->Execute();
	}

	if (AutoRWLoad)
		RamWatchHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAMWATCH), NULL, (DLGPROC) RamWatchProc);
	
	RunGui();

	CloseAllToolWindows();

	return 0;
}

void RunMessageLoop() {
	MSG msg;

	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0) > 0)
		{
			if (RamWatchHWnd && IsDialogMessage(RamWatchHWnd, &msg))
			{
				if(msg.message == WM_KEYDOWN) // send keydown messages to the dialog (for accelerators, and also needed for the Alt key to work)
					SendMessage(RamWatchHWnd, msg.message, msg.wParam, msg.lParam);
				continue;
			}
			if (RamSearchHWnd && IsDialogMessage(RamSearchHWnd, &msg))
				continue;

			//if(!TranslateAccelerator(gApp.hWnd,hAccel,&msg)) //adelikat: TODO: get this hooked back up so Ram Watch can use accel keys
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}

void RunGui() {
	Continue = 0;
	for(;;) {
		RunMessageLoop();
		Sleep(1);
		if(Continue)
			break;
	}
	OpenPlugins(gApp.hWnd);
	Continue = 0;
	Running = 1;
}

void SNDDXSetWindow(HWND hwnd);
void RestoreWindow() {
	AccBreak = 1;
	DestroyWindow(gApp.hWnd);
	CreateMainWindow(SW_SHOWNORMAL);
	SNDDXSetWindow(gApp.hWnd);
	ShowCursor(TRUE);
//	SetCursor(LoadCursor(gApp.hInstance, IDC_ARROW));
//	ShowCursor(TRUE);
}

void UpdateToolWindows()
{
	Update_RAM_Search();	//Update_RAM_Watch() is also called; hotkey.cpp - HK_StateLoadSlot & State_Load also call these functions
	RefreshAllToolWindows();
	if (Config.WriteAnalog && AnalogControlHWnd)
		UpdatePositionText(AnalogControlHWnd);
}

int Slots[5] = { -1, -1, -1, -1, -1 };

void ResetMenuSlots() {
	int i;

	for (i=0; i<5; i++) {
		if (Slots[i] == -1)
			EnableMenuItem(GetSubMenu(gApp.hMenu, 0), ID_FILE_STATES_LOAD_SLOT1+i, MF_GRAYED);
		else 
			EnableMenuItem(GetSubMenu(gApp.hMenu, 0), ID_FILE_STATES_LOAD_SLOT1+i, MF_ENABLED);
	}
}

void UpdateMenuSlots() {
	char str[256];
	int i;

	for (i=0; i<5; i++) {
		sprintf(str, "sstates\\%10.10s.%3.3d", CdromLabel, i);
		Slots[i] = CheckState(str);
	}
}


void CloseConsole() {
	FreeConsole(); hConsole = NULL;
}

void States_Load(int num) {
	char Text[256];
	int ret;

	OpenPlugins(gApp.hWnd);
	SysReset();
	NeedReset = 0;

	sprintf (Text, "sstates\\%10.10s.%3.3d", CdromLabel, num);
	ret = LoadState(Text);
	if (ret == 0)
		 sprintf(Text, _("*PSXjin*: Loaded State %d"), num);
	else sprintf(Text, _("*PSXjin*: Error Loading State %d"), num);
	GPUdisplayText(Text);

	Running = 1;
	psxCpu->Execute();
}

void States_Save(int num) {
	char Text[256];
	int ret;

	OpenPlugins(gApp.hWnd);
	if (NeedReset) {
		SysReset();
		NeedReset = 0;
	}
	GPUupdateLace();

	sprintf (Text, "sstates/%10.10s.%3.3d", CdromLabel, num);
	GPUfreeze(2, (GPUFreeze_t *)&num);
	ret = SaveState(Text);
	if (ret == 0)
		 sprintf(Text, _("*PSXjin*: Saved State %d"), num);
	else sprintf(Text, _("*PSXjin*: Error Saving State %d"), num);
	GPUdisplayText(Text);

	Running = 1;
	psxCpu->Execute();
}

void OnStates_LoadOther() {
	OPENFILENAME ofn;
	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];
	char szFilter[256];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	strcpy(szFilter, _("PSXjin State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= MAXFILENAME;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= MAXFILENAME;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		char Text[256];
		int ret;

		OpenPlugins(gApp.hWnd);
		if (NeedReset) {
			SysReset();
			NeedReset = 0;
		}

		ret = LoadState(szFileName);
		if (ret == 0)
			 sprintf(Text, _("*PSXjin*: Loaded State %s"), szFileName);
		else sprintf(Text, _("*PSXjin*: Error Loading State %s"), szFileName);
		GPUdisplayText(Text);

		Running = 1;
		psxCpu->Execute();
	}
} 

void OnStates_Save1() { States_Save(0); } 
void OnStates_Save2() { States_Save(1); } 
void OnStates_Save3() { States_Save(2); } 
void OnStates_Save4() { States_Save(3); } 
void OnStates_Save5() { States_Save(4); } 

void OnStates_SaveOther() {
	OPENFILENAME ofn;

	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];
	char szFilter[256];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	strcpy(szFilter, _("PSXjin State Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= MAXFILENAME;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= MAXFILENAME;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "001";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetSaveFileName ((LPOPENFILENAME)&ofn)) {
		char Text[256];
		int ret;

		ret = SaveState(szFileName);
		if (ret == 0)
			 sprintf(Text, _("*PSXjin*: Saved State %s"), szFileName);
		else sprintf(Text, _("*PSXjin*: Error Saving State %s"), szFileName);
		GPUdisplayText(Text);

		Running = 1;
		psxCpu->Execute();
	}
} 


void ExitPSXjin()
{
	if (AskSave())	//RamWatch ask's to save changes if needed, if the user cancels, don't close
	{
		if (!AccBreak) 
		{
			if (Movie.mode != MOVIEMODE_INACTIVE)
				MOV_StopMovie();
			if (Movie.capture)
				WIN32_StopAviRecord();
			if (Running)
				ClosePlugins();
			SysClose();
			exit(0);
			SaveConfig();	//Various settings	//TODO: A single save config file function!
			SaveIni();		//Main Window settings
		}
		else
			AccBreak = 0;
		return;
	}
}

bool BIOSExists()	//Attempts to open the BIOS, if successful, it returns true
{
	FILE *f = NULL;
	char Bios[256];
	sprintf(Bios, "%s%s", Config.BiosDir, Config.Bios);
	f = fopen(Bios, "rb");
	if (f == NULL)
	{
		return false;
	}
	else
	{
		fclose(f);
		return true;
	}
}

void RunCD(HWND hWnd)
{
	if (!IsoFile[0] == 0)
	{
		if (BIOSExists())
		{
			ClosePlugins();
			if(!OpenPlugins(hWnd)) 
			{
				iCallW32Gui =  1;
				CDRclose();
				return;
			}
			
			SysReset();
			NeedReset = 0;
			CheckCdrom();
			if (LoadCdrom() == -1) {
				ClosePlugins();
				RestoreWindow();
				SysMessage(_("Could not load Cdrom"));
				IsoFile[0] = 0;
				return;
			}
			
			Movie.currentFrame = 0;
			WIN32_StopAviRecord();
			MOV_StopMovie();
			RecentCDs.UpdateRecentItems(IsoFile);
			Running = 1;
			psxCpu->Execute();
		}
		else
			SysMessage("Failure to open BIOS.  Please reconfigure your BIOS settings");
	}
}

void WindowBoundsCheckNoResize(int &windowPosX, int &windowPosY, long windowRight)
{
		if (windowRight < 59) {
		windowPosX = 0;
		}
		if (windowPosY < -18) {
		windowPosY = -18;
		} 
}

void UpdateWindowSizeFromConfig()
{
	MainWindow_width = GetPrivateProfileInt("GPU", "iResX", 320, Config.Conf_File);
	MainWindow_menubar = GetPrivateProfileInt("GPU", "menubar", 48, Config.Conf_File);
	MainWindow_height = GetPrivateProfileInt("GPU", "iResY", 240, Config.Conf_File)+MainWindow_menubar;
}

void SetWindowSize(int dX, int dY)
{
	MainWindow_width = dX;
	MainWindow_height = dY+MainWindow_menubar;
	MoveWindow(gApp.hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
}




void WriteWindowSizeToConfig()
{
	char Str_Tmp[1024];
	sprintf(Str_Tmp, "%d", MainWindow_width);
	WritePrivateProfileString("GPU", "iResX", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", MainWindow_height-GetMenuSize());
	WritePrivateProfileString("GPU", "iResY", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", MainWindow_menubar);
	WritePrivateProfileString("GPU", "menubar", Str_Tmp, Config.Conf_File);
}

void ResetGame()
{
	if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.reset ^= 1;
			if (MovieControl.reset)
				GPUdisplayText("*PSXjin*: CPU Will Reset On Next Frame");
			else
				GPUdisplayText("*PSXjin*: CPU Won't Reset On Next Frame");
		}
		else {
			GPUdisplayText("*PSXjin*: CPU Reset");
			LoadCdBios = 0;
			SysReset();
			NeedReset = 0;
			Movie.currentFrame = 0;
			CheckCdrom();
			if (LoadCdrom() == -1)
				SysMessage(_("Could not load Cdrom"));
			Running = 1;
			psxCpu->Execute();
	}
}

bool IsFileExtension(std::string filename, std::string ext)
{
	if (!(filename.find(ext) == std::string::npos) && (filename.find(ext) == filename.length()-4))
		return true;
	else
		return false;
}

LRESULT WINAPI MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	char File[256];
	char Str_Tmp[1024];
	switch (msg) {
		case WM_DROPFILES:
		{
			UINT len;
			char *ftmp;

			len=DragQueryFile((HDROP)wParam,0,0,0)+1; 
			if((ftmp=(char*)malloc(len))) 
			{
				//adelikat:  Drag and Drop only checks file extension, the internal functions are responsible for file error checking
				DragQueryFile((HDROP)wParam,0,ftmp,len); 
				std::string fileDropped = ftmp;
				
				//-------------------------------------------------------
				//Check if Ram Watch file
				//-------------------------------------------------------
				if (IsFileExtension(fileDropped, ".img") || IsFileExtension(fileDropped, ".bin") || IsFileExtension(fileDropped, ".iso"))
				{
					strcpy(IsoFile, fileDropped.c_str());
					RunCD(hWnd);
				}
				else if (IsFileExtension(fileDropped, ".wch")) 
				{
					SendMessage(hWnd, WM_COMMAND, (WPARAM)ID_RAM_WATCH,(LPARAM)(NULL));
					Load_Watches(true, fileDropped.c_str());
				}
				else if (IsFileExtension(fileDropped, ".pjm"))
				{
					if (!IsoFile[0] == 0)
					{
						Movie.readOnly = 1;
						WIN32_StartMovieReplay(ftmp);
						
					}
					else
					{
						CfgOpenFile();	//If no game selected, ask the user to select one
						WIN32_StartMovieReplay(ftmp);
						RunCD(hWnd);
						
					}
				}
				else if (IsFileExtension(fileDropped, ".pxm"))
				{
					SysMessage("PSXjin-rr movies are not supported in PSXjin");
				}
				else if (IsFileExtension(fileDropped, ".lua"))
				{
					RecentLua.UpdateRecentItems(fileDropped);
					PSXjin_LoadLuaCode(fileDropped.c_str());
				}
				else if (IsFileExtension(fileDropped, ".001") || IsFileExtension(fileDropped, ".002") || IsFileExtension(fileDropped, ".003") || 
					IsFileExtension(fileDropped, ".004") || IsFileExtension(fileDropped, ".005") || IsFileExtension(fileDropped, ".006") ||
					IsFileExtension(fileDropped, ".007") || IsFileExtension(fileDropped, ".008") || IsFileExtension(fileDropped, ".009") ||
					IsFileExtension(fileDropped, ".010"))
				{
					strcpy(Str_Tmp, fileDropped.c_str());
					LoadState(Str_Tmp);
				}
			}
		}

		case WM_ENTERMENULOOP:
			UpdateMenuHotkeys();
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_RESET,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_CLOSE_CD,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_LUA_CLOSE_ALL,MF_GRAYED); //Always grayed until mutiple lua script support
			EnableMenuItem(gApp.hMenu,ID_FILE_RECORD_MOVIE,MF_BYCOMMAND | ( IsMovieLoaded() ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_REPLAY_MOVIE,MF_BYCOMMAND | ( IsMovieLoaded() ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STOP_MOVIE,MF_BYCOMMAND   | (!IsMovieLoaded() ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_PLAYBEGINNING,MF_BYCOMMAND   | (!IsMovieLoaded() ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_CONFIGURATION_CONTROLLERS,MF_BYCOMMAND | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));

			EnableMenuItem(gApp.hMenu,ID_START_CAPTURE,  MF_BYCOMMAND   | (!AVIisCapturing && IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_END_CAPTURE,    MF_BYCOMMAND   | (AVIisCapturing   ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_SCREENSHOT,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));  
			EnableMenuItem(gApp.hMenu,ID_CONFIGURATION,MF_BYCOMMAND | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT0,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT1,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT2,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT3,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT4,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT5,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT6,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT7,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT8,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));   
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_SLOT9,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_LOAD_OTHER,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));

			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT0,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT1,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT2,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT3,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT4,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT5,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT6,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT7,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT8,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_SLOT9,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_FILE_STATES_SAVE_OTHER,MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_CDCLOSE,		MF_BYCOMMAND | (IsoFile[0] ? MF_ENABLED:MF_GRAYED));

			//Disable these when a game is running.  TODO: Find a way to resize the drawing area without having to restart the game
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_1X,MF_BYCOMMAND   | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_2X,MF_BYCOMMAND   | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_3X,MF_BYCOMMAND   | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));
			EnableMenuItem(gApp.hMenu,ID_EMULATOR_4X,MF_BYCOMMAND   | (!IsoFile[0] ? MF_ENABLED:MF_GRAYED));

			CheckMenuItem(gApp.hMenu, ID_FILE_READONLY, MF_BYCOMMAND | (Movie.readOnly ? MF_CHECKED:MF_UNCHECKED));
			CheckMenuItem(gApp.hMenu, ID_EMULATOR_MUTE, MF_BYCOMMAND | ((iVolume == 5) ? MF_CHECKED:MF_UNCHECKED));
			CheckMenuItem(gApp.hMenu, ID_EMULATOR_DISPALL, MF_BYCOMMAND | (dispAllText ? MF_CHECKED:MF_UNCHECKED));
			CheckMenuItem(gApp.hMenu, ID_EMULATOR_DISPFRAMECOUNTER, MF_BYCOMMAND | (dispFrameCounter ? MF_CHECKED:MF_UNCHECKED));
			CheckMenuItem(gApp.hMenu, ID_EMULATOR_DISPINPUT, MF_BYCOMMAND | (dispInput ? MF_CHECKED:MF_UNCHECKED));
			
			return TRUE;
		case WM_MOVE:
		{
			if (!IsIconic(hWnd)) {
			RECT wrect;
			GetWindowRect(hWnd,&wrect);
			MainWindow_wndx = wrect.left;
			MainWindow_wndy = wrect.top;
			WindowBoundsCheckNoResize(MainWindow_wndx,MainWindow_wndy,wrect.right);
			}
		}
		case WM_COMMAND:
			//Recent CDs
			if(wParam >= RECENTCD_START && wParam <= RECENTCD_START + RecentCDs.MAX_RECENT_ITEMS - 1)
			{
				strcpy(IsoFile, RecentCDs.GetRecentItem(wParam - RECENTCD_START).c_str());
				RecentCDs.UpdateRecentItems(IsoFile);
				RunCD(gApp.hWnd);
				return TRUE;
			}
			else if (wParam == RecentCDs.GetClearID())
			{
				RecentCDs.ClearRecentItems();
				return TRUE;
			}
			else if (wParam == RecentCDs.GetAutoloadID())
			{
				RecentCDs.FlipAutoLoad();
				return TRUE;
			}
			//Recent Movies
			if(wParam >= RECENTMOVIE_START && wParam <= RECENTMOVIE_START + RecentMovies.MAX_RECENT_ITEMS - 1)
			{
				strcpy(Str_Tmp, RecentMovies.GetRecentItem(wParam - RECENTMOVIE_START).c_str());
				RecentMovies.UpdateRecentItems(Str_Tmp);
				WIN32_StartMovieReplay(Str_Tmp);
				return TRUE;
			}
			else if (wParam == RecentMovies.GetClearID())
			{
				RecentMovies.ClearRecentItems();
				return TRUE;
			}
			else if (wParam == RecentMovies.GetAutoloadID())
			{
				RecentMovies.FlipAutoLoad();
				return TRUE;
			}
			//Recent Lua
			if(wParam >= RECENTLUA_START && wParam <= RECENTLUA_START + RecentLua.MAX_RECENT_ITEMS - 1)
			{
				PSXjin_LoadLuaCode(RecentLua.GetRecentItem(wParam - RECENTLUA_START).c_str());
				RecentLua.UpdateRecentItems(RecentLua.GetRecentItem(wParam - RECENTLUA_START));
				return TRUE;
			}
			else if (wParam == RecentLua.GetClearID())
			{
				RecentLua.ClearRecentItems();
				return TRUE;
			}
			else if (wParam == RecentLua.GetAutoloadID())
			{
				RecentLua.FlipAutoLoad();
				return TRUE;
			}
			switch (LOWORD(wParam)) {
/*
				case ID_FILE_RUN_EXE:
					if (!Open_File_Proc(File)) return TRUE;
					SetMenu(hWnd, NULL);
					OpenPlugins(hWnd);
					SysReset();
					NeedReset = 0;
					Load(File);
					Running = 1;
					psxCpu->Execute();
					return TRUE;
*/
				case ID_FILE_EXIT:
					ExitPSXjin();
					return TRUE;

				case ID_FILE_RECORD_MOVIE:
					WIN32_StartMovieRecord();
					return TRUE;

				case ID_FILE_REPLAY_MOVIE:
					WIN32_StartMovieReplay(0);
					return TRUE;

				case ID_FILE_STOP_MOVIE:
					MOV_StopMovie();
					return TRUE;

				case ID_FILE_PLAYBEGINNING:
					PlayMovieFromBeginning();
					return TRUE;

				case ID_FILE_READONLY:
					ReadonlyToggle();
					return TRUE;

				case ID_LUA_OPEN:
					if(!LuaConsoleHWnd)
					{
						LuaConsoleHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_LUA), NULL, (DLGPROC) DlgLuaScriptDialog);
					}
					else
						SetForegroundWindow(LuaConsoleHWnd);
					break;

				case ID_LUA_CLOSE_ALL:
					if(LuaConsoleHWnd)
						PostMessage(LuaConsoleHWnd, WM_CLOSE, 0, 0);
					break;

				case ID_START_CAPTURE:
					WIN32_StartAviRecord();
					break;

				case ID_END_CAPTURE:
					WIN32_StopAviRecord();
					break;

				case ID_FILE_RUN_CD:
					LoadCdBios = 0;	//adelikat: In PSXjin we shall not allow CD BIOS files, until someone proves to me otherwise

					CfgOpenFile();	//Open the Open CD dialog, which will set IsoFile if the user chooses one
					RunCD(hWnd);
					return true;
				case ID_FILE_CLOSE_CD:
					strcpy(IsoFile, "");
					iCallW32Gui =  1;
					CDRclose();
					Movie.currentFrame = 0;
					break;

				case ID_FILE_SCREENSHOT:
					GPUmakeSnapshot();
					return TRUE;

				case ID_FILE_STATES_LOAD_SLOT0: States_Load(0); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT1: States_Load(1); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT2: States_Load(2); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT3: States_Load(3); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT4: States_Load(4); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT5: States_Load(5); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT6: States_Load(6); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT7: States_Load(7); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT8: States_Load(8); return TRUE;
				case ID_FILE_STATES_LOAD_SLOT9: States_Load(9); return TRUE;
				case ID_FILE_STATES_LOAD_OTHER: OnStates_LoadOther(); return TRUE;

				case ID_FILE_STATES_SAVE_SLOT0: States_Save(0); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT1: States_Save(1); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT2: States_Save(2); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT3: States_Save(3); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT4: States_Save(4); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT5: States_Save(5); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT6: States_Save(6); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT7: States_Save(7); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT8: States_Save(8); return TRUE;
				case ID_FILE_STATES_SAVE_SLOT9: States_Save(9); return TRUE;
				case ID_FILE_STATES_SAVE_OTHER: OnStates_SaveOther(); return TRUE;

				case ID_EMULATOR_RESET:
					ResetGame();
					return TRUE;
				case ID_EMULATOR_MUTE:
					if (iVolume == 5) 
						iVolume = 0;
					else 
						iVolume = 5;
					return TRUE;
				case ID_EMULATOR_CDCLOSE:
					CDOpenClose();
					return TRUE;
				case ID_CONFIGURATION_GRAPHICS:
					GPUconfigure();
					UpdateWindowSizeFromConfig();
					MoveWindow(hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
					return TRUE;
				case ID_EMULATOR_1X:
					MainWindow_width = 320;
					MainWindow_height = 240+GetMenuSize();
					MoveWindow(hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
					WriteWindowSizeToConfig();
					return TRUE;

				case ID_EMULATOR_2X:
					MainWindow_width = 640;
					MainWindow_height = 480+GetMenuSize();
					MoveWindow(hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
					WriteWindowSizeToConfig();
					return TRUE;
				case ID_EMULATOR_3X:
					MainWindow_width = 960;
					MainWindow_height = 720+GetMenuSize();
					MoveWindow(hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
					WriteWindowSizeToConfig();
					return TRUE;
				case ID_EMULATOR_4X:
					MainWindow_width = 1280;
					MainWindow_height = 960+GetMenuSize();
					MoveWindow(hWnd, MainWindow_wndx, MainWindow_wndy, MainWindow_width, MainWindow_height, true);
					WriteWindowSizeToConfig();
					return TRUE;
				case ID_EMULATOR_DISPALL:
					dispAllText ^= 1;
					GPUshowALL();
					return true;
				case ID_EMULATOR_DISPINPUT:
					dispInput ^= 1;
					GPUshowInput();
					return TRUE;
				case ID_EMULATOR_DISPFRAMECOUNTER:
					dispFrameCounter ^= 1;
					GPUshowframecounter();
					return TRUE;

				case ID_CONFIGURATION_SOUND:
					SPUconfigure();
					return TRUE;

				case ID_CONFIGURATION_CONTROLLERS:
					if (PAD1_configure) PAD1_configure();
					if (strcmp(Config.Pad1, Config.Pad2)) if (PAD2_configure) PAD2_configure();
					return TRUE;

				case ID_CONFIGURATION_MAPHOTKEYS:
						MHkeysCreate();
					return TRUE;

				case ID_RAM_SEARCH:
					if(!RamSearchHWnd)
					{
						reset_address_info();
						RamSearchHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAMSEARCH), NULL, (DLGPROC) RamSearchProc);
					}
					else
						SetForegroundWindow(RamSearchHWnd);
					return TRUE;				

				case ID_RAM_WATCH:
					if(!RamWatchHWnd)
					{
						RamWatchHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAMWATCH), NULL, (DLGPROC) RamWatchProc);
					}
					else
						SetForegroundWindow(RamWatchHWnd);
					return TRUE;

				case IDM_MEMORY:
					if (!RegWndClass("MemView_ViewBox", MemView_ViewBoxProc, 0, sizeof(CMemView*)))
						return 0;

					OpenToolWindow(new CMemView());
					return 0;

				case ID_CONFIGURATION_MEMPOKE:
						CreateMemPoke();
					return TRUE;

				case ID_CONFIGURATION_CHEATS:
						PSXjinRemoveCheats();
						CreateCheatEditor();
						PSXjinApplyCheats();
					return TRUE;
/*
				case ID_CONFIGURATION_NETPLAY:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_NETPLAY), hWnd, (DLGPROC)ConfigureNetPlayDlgProc);
					return TRUE;
*/
				case ID_CONFIGURATION_MEMORYCARDMANAGER:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MCDCONF), hWnd, (DLGPROC)ConfigureMcdsDlgProc);
					return TRUE;

				case ID_CONFIGURATION_CPU:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CPUCONF), hWnd, (DLGPROC)ConfigureCpuDlgProc);
					return TRUE;				

				case ID_EMULATOR_ANALOG:
					OpenAnalogControl();
					return TRUE;
				case ID_CONFIGURATION:
					ConfigurePlugins(hWnd);
					return TRUE;

				case ID_HELP_TUTORIAL:
					ShellExecute(NULL, "open", "http://code.google.com/p/psxjin/wiki/QuickTutorial", NULL, NULL, SW_SHOWNORMAL);
					break;

				case ID_HELP_HELP:
					ShellExecute(NULL, "open", "PSXjin-instructions.txt", NULL, NULL, SW_SHOWNORMAL);
					return TRUE;

				case ID_HELP_ABOUT:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(ABOUT_DIALOG), hWnd, (DLGPROC)AboutDlgProc);
					return TRUE;
			}
			break;

		case WM_SYSKEYDOWN:
			if (wParam != VK_F10)
				return DefWindowProc(hWnd, msg, wParam, lParam);

		case WM_KEYDOWN:
			PADhandleKey(wParam);
			return TRUE;

		case WM_KEYUP: {
			int modifier = 0;int key;
			if (Config.GetAutoHold)
			{				
				PadDataS padd; 
				PAD1_readPort1(&padd);
				Config.Pad1AutoHold |= (padd.buttonStatus ^ 0xffff);				
				PAD2_readPort2(&padd);
				Config.Pad2AutoHold |= (padd.buttonStatus ^ 0xffff);
				if ((Config.Pad1AutoHold > 0) || (Config.Pad2AutoHold > 0))				
					Config.EnableAutoHold = true;				
				Config.GetAutoHold = false;					
			}
			if (Config.GetAutoFire)
			{
				PadDataS padd; 
				PAD1_readPort1(&padd);
				Config.Pad1AutoFire |= (padd.buttonStatus ^ 0xffff);
				PAD2_readPort2(&padd);
				Config.Pad2AutoFire |= (padd.buttonStatus ^ 0xffff);			
				if ((Config.Pad1AutoFire > 0) || (Config.Pad2AutoFire > 0))
					Config.EnableAutoFire = true;				
				Config.GetAutoFire = false;				
			}			
			key = wParam;
			if(GetAsyncKeyState(VK_MENU))
				modifier = VK_MENU;
			else if(GetAsyncKeyState(VK_CONTROL))
				modifier = VK_CONTROL;
			else if(GetAsyncKeyState(VK_SHIFT))
				modifier = VK_SHIFT;
			if(key == EmuCommandTable[EMUCMD_TURBOMODE].key
			&& modifier == EmuCommandTable[EMUCMD_TURBOMODE].keymod)
			{
				SetEmulationSpeed(EMUSPEED_NORMAL);
				iTurboMode = 0;
			}
			return TRUE;
		}
	
		case WM_MOUSEMOVE: {
			mousex=LOWORD(lParam);
			mousey=HIWORD(lParam);
			return TRUE;
		}
	
		case WM_DESTROY:
			ExitPSXjin();
			return TRUE;

		case WM_QUIT:
			ExitPSXjin();
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return FALSE;
}

HWND mcdDlg;
McdBlock Blocks[2][15];
int IconC[2][15];
HIMAGELIST Iiml[2];
HICON eICON;

void CreateListView(int idc) {
	HWND List;
	LV_COLUMN col;

	List = GetDlgItem(mcdDlg, idc);

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt  = LVCFMT_LEFT;
	
	col.pszText  = _("Title");
	col.cx       = 170;
	col.iSubItem = 0;

	ListView_InsertColumn(List, 0, &col);

	col.pszText  = _("Status");
	col.cx       = 50;
	col.iSubItem = 1;

	ListView_InsertColumn(List, 1, &col);

	col.pszText  = _("Game ID");
	col.cx       = 90;
	col.iSubItem = 2;

	ListView_InsertColumn(List, 2, &col);

	col.pszText  = _("Game");
	col.cx       = 80;
	col.iSubItem = 3;

	ListView_InsertColumn(List, 3, &col);
}

int GetRGB() {
    HDC scrDC, memDC;
    HBITMAP oldBmp = NULL; 
    HBITMAP curBmp = NULL;
    COLORREF oldColor;
    COLORREF curColor = RGB(255,255,255);
    int i, R, G, B;

    R = G = B = 1;
 
    scrDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    memDC = CreateCompatibleDC(NULL); 
    curBmp = CreateCompatibleBitmap(scrDC, 1, 1);    
    oldBmp = (HBITMAP)SelectObject(memDC, curBmp);
        
    for (i = 255; i >= 0; --i) {
        oldColor = curColor;
        curColor = SetPixel(memDC, 0, 0, RGB(i, i, i));
        
        if (GetRValue(curColor) < GetRValue(oldColor)) ++R; 
        if (GetGValue(curColor) < GetGValue(oldColor)) ++G;
        if (GetBValue(curColor) < GetBValue(oldColor)) ++B;
    }
 
    DeleteObject(oldBmp);
    DeleteObject(curBmp);
    DeleteDC(scrDC);
    DeleteDC(memDC);
 
    return (R * G * B);
}
 
HICON GetIcon(short *icon) {
    ICONINFO iInfo;
    HDC hDC;
    char mask[16*16];
    int x, y, c, Depth;
  
    hDC = CreateIC("DISPLAY",NULL,NULL,NULL);
    Depth=GetDeviceCaps(hDC, BITSPIXEL);
    DeleteDC(hDC);
 
    if (Depth == 16) {
        if (GetRGB() == (32 * 32 * 32))        
            Depth = 15;
    }

    for (y=0; y<16; y++) {
        for (x=0; x<16; x++) {
            c = icon[y*16+x];
            if (Depth == 15 || Depth == 32)
				c = ((c&0x001f) << 10) | 
					((c&0x7c00) >> 10) | 
					((c&0x03e0)      );
			else
                c = ((c&0x001f) << 11) |
					((c&0x7c00) >>  9) |
					((c&0x03e0) <<  1);

            icon[y*16+x] = c;
        }
    }    

    iInfo.fIcon = TRUE;
    memset(mask, 0, 16*16);
    iInfo.hbmMask  = CreateBitmap(16, 16, 1, 1, mask);
    iInfo.hbmColor = CreateBitmap(16, 16, 1, 16, icon); 
 
    return CreateIconIndirect(&iInfo);
}

HICON hICON[2][3][15];
int aIover[2];                        
int ani[2];
 
void LoadMcdItems(int mcd, int idc) {
    HWND List = GetDlgItem(mcdDlg, idc);
    LV_ITEM item;
    HIMAGELIST iml = Iiml[mcd-1];
    int i, j;
    HICON hIcon;
    McdBlock *Info;
 
    aIover[mcd-1]=0;
    ani[mcd-1]=0;
 
    ListView_DeleteAllItems(List);

    for (i=0; i<15; i++) {
  
        item.mask      = LVIF_TEXT | LVIF_IMAGE;
        item.iItem       = i;
        item.iImage    = i;
        item.pszText  = LPSTR_TEXTCALLBACK;
        item.iSubItem = 0;
 
        IconC[mcd-1][i] = 0;
        Info = &Blocks[mcd-1][i];
 
        if ((Info->Flags & 0xF) == 1 && Info->IconCount != 0) {
            hIcon = GetIcon(Info->Icon);   
 
            if (Info->IconCount > 1) {
                for(j = 0; j < 3; j++)
                    hICON[mcd-1][j][i]=hIcon;
            }
        } else {
            hIcon = eICON; 
        }
 
        ImageList_ReplaceIcon(iml, -1, hIcon);
        ListView_InsertItem(List, &item);
    } 
}

void UpdateMcdItems(int mcd, int idc) {
    HWND List = GetDlgItem(mcdDlg, idc);
    LV_ITEM item;
    HIMAGELIST iml = Iiml[mcd-1];
    int i, j;
    McdBlock *Info;
    HICON hIcon;
 
    aIover[mcd-1]=0;
    ani[mcd-1]=0;
  
    for (i=0; i<15; i++) { 
 
        item.mask     = LVIF_TEXT | LVIF_IMAGE;
        item.iItem    = i;
        item.iImage   = i;
        item.pszText  = LPSTR_TEXTCALLBACK;
        item.iSubItem = 0;
 
        IconC[mcd-1][i] = 0; 
        Info = &Blocks[mcd-1][i];
 
        if ((Info->Flags & 0xF) == 1 && Info->IconCount != 0) {
            hIcon = GetIcon(Info->Icon);   
 
            if (Info->IconCount > 1) { 
                for(j = 0; j < 3; j++)
                    hICON[mcd-1][j][i]=hIcon;
            }
        } else { 
            hIcon = eICON; 
        }
              
        ImageList_ReplaceIcon(iml, i, hIcon);
        ListView_SetItem(List, &item);
    } 
    ListView_Update(List, -1);
}
 
void McdListGetDispInfo(int mcd, int idc, LPNMHDR pnmh) {
	LV_DISPINFO *lpdi = (LV_DISPINFO *)pnmh;
	McdBlock *Info;

	Info = &Blocks[mcd-1][lpdi->item.iItem];

	switch (lpdi->item.iSubItem) {
		case 0:
			switch (Info->Flags & 0xF) {
				case 1:
					lpdi->item.pszText = Info->Title;
					break;
				case 2:
					lpdi->item.pszText = _("mid link block");
					break;
				case 3:
					lpdi->item.pszText = _("terminiting link block");
					break;
			}
			break;
		case 1:
			if ((Info->Flags & 0xF0) == 0xA0) {
				if ((Info->Flags & 0xF) >= 1 &&
					(Info->Flags & 0xF) <= 3) {
					lpdi->item.pszText = _("Deleted");
				} else lpdi->item.pszText = _("Free");
			} else if ((Info->Flags & 0xF0) == 0x50)
				lpdi->item.pszText = _("Used");
			else { lpdi->item.pszText = _("Free"); }
			break;
		case 2:
			if((Info->Flags & 0xF)==1)
				lpdi->item.pszText = Info->ID;
			break;
		case 3:
			if((Info->Flags & 0xF)==1)
				lpdi->item.pszText = Info->Name;
			break;
	}
}

void McdListNotify(int mcd, int idc, LPNMHDR pnmh) {
	switch (pnmh->code) {
		case LVN_GETDISPINFO: McdListGetDispInfo(mcd, idc, pnmh); break;
	}
}

void UpdateMcdDlg() {
	int i;

	for (i=1; i<16; i++) GetMcdBlockInfo(1, i, &Blocks[0][i-1]);
	for (i=1; i<16; i++) GetMcdBlockInfo(2, i, &Blocks[1][i-1]);
	UpdateMcdItems(1, IDC_LIST1);
	UpdateMcdItems(2, IDC_LIST2);
}

void LoadMcdDlg() {
	int i;

	for (i=1; i<16; i++) GetMcdBlockInfo(1, i, &Blocks[0][i-1]);
	for (i=1; i<16; i++) GetMcdBlockInfo(2, i, &Blocks[1][i-1]);
	LoadMcdItems(1, IDC_LIST1);
	LoadMcdItems(2, IDC_LIST2);
}

void UpdateMcdIcon(int mcd, int idc) {
    HWND List = GetDlgItem(mcdDlg, idc);
    HIMAGELIST iml = Iiml[mcd-1];
    int i;
    McdBlock *Info;
    int *count; 
 
    if(!aIover[mcd-1]) {
        ani[mcd-1]++; 
 
        for (i=0; i<15; i++) {
            Info = &Blocks[mcd-1][i];
            count = &IconC[mcd-1][i];
 
            if ((Info->Flags & 0xF) != 1) continue;
            if (Info->IconCount <= 1) continue;
 
            if (*count < Info->IconCount) {
                (*count)++;
                aIover[mcd-1]=0;
 
                if(ani[mcd-1] <= (Info->IconCount-1))  // last frame and below...
                    hICON[mcd-1][ani[mcd-1]][i] = GetIcon(&Info->Icon[(*count)*16*16]);
            } else {
                aIover[mcd-1]=1;
            }
        }

    } else { 
 
        if (ani[mcd-1] > 1) ani[mcd-1] = 0;  // 1st frame
        else ani[mcd-1]++;                       // 2nd, 3rd frame
 
        for(i=0;i<15;i++) {
            Info = &Blocks[mcd-1][i];
 
            if (((Info->Flags & 0xF) == 1) && (Info->IconCount > 1))
                ImageList_ReplaceIcon(iml, i, hICON[mcd-1][ani[mcd-1]][i]);
        }
        InvalidateRect(List,  NULL, FALSE);
    }
}

static int copy = 0, copymcd = 0;
//static int listsel = 0;

BOOL CALLBACK ConfigureMcdsDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[256];
	LPBYTE lpAND, lpXOR;
	LPBYTE lpA, lpX;
	int i, j;

	switch(uMsg) {
		case WM_INITDIALOG:
			mcdDlg = hW;

			SetWindowText(hW, _("Memcard Manager"));

			Button_SetText(GetDlgItem(hW, IDOK),        _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL),    _("Cancel"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL1), _("Select Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_FORMAT1), _("Format Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_RELOAD1), _("Reload Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL2), _("Select Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_FORMAT2), _("Format Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_RELOAD2), _("Reload Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_COPYTO2), _("-> Copy ->"));
			Button_SetText(GetDlgItem(hW, IDC_COPYTO1), _("<- Copy <-"));
			Button_SetText(GetDlgItem(hW, IDC_PASTE),   _("Paste"));
			Button_SetText(GetDlgItem(hW, IDC_DELETE1), _("<- Un/Delete"));
			Button_SetText(GetDlgItem(hW, IDC_DELETE2), _("Un/Delete ->"));
 
			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD1), _("Memory Card 1"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD2), _("Memory Card 2"));

			lpA=lpAND=(LPBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(16*16));
			lpX=lpXOR=(LPBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(16*16));

			for(i=0;i<16;i++)
			{
				for(j=0;j<16;j++)
				{
					*lpA++=0xff;
					*lpX++=0;
				}
			}
			eICON=CreateIcon(gApp.hInstance,16,16,1,1,lpAND,lpXOR);

			HeapFree(GetProcessHeap(),0,lpAND);
			HeapFree(GetProcessHeap(),0,lpXOR);

			if (!strlen(Config.Mcd1)) strcpy(Config.Mcd1, "memcards\\Mcd001.mcr");
			if (!strlen(Config.Mcd2)) strcpy(Config.Mcd2, "memcards\\Mcd002.mcr");
			Edit_SetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1);
			Edit_SetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2);

			CreateListView(IDC_LIST1);
			CreateListView(IDC_LIST2);
 
            Iiml[0] = ImageList_Create(16, 16, ILC_COLOR16, 0, 0);
            Iiml[1] = ImageList_Create(16, 16, ILC_COLOR16, 0, 0);
 
            ListView_SetImageList(GetDlgItem(mcdDlg, IDC_LIST1), Iiml[0], LVSIL_SMALL);
            ListView_SetImageList(GetDlgItem(mcdDlg, IDC_LIST2), Iiml[1], LVSIL_SMALL);

			Button_Enable(GetDlgItem(hW, IDC_PASTE), FALSE);

			LoadMcdDlg();

			SetTimer(hW, 1, 250, NULL);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_COPYTO1:
					copy = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST2));
					copymcd = 1;

					Button_Enable(GetDlgItem(hW, IDC_PASTE), TRUE);
					return TRUE;
				case IDC_COPYTO2:
					copy = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST1));
					copymcd = 2;

					Button_Enable(GetDlgItem(hW, IDC_PASTE), TRUE);
					return TRUE;
				case IDC_PASTE:
					if (MessageBox(hW, _("Are you sure you want to paste this selection?"), _("Confirmation"), MB_YESNO) == IDNO) return TRUE;

					if (copymcd == 1) {
						Edit_GetText(GetDlgItem(hW,IDC_MCD1), str, 256);
						i = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST1));

						// save dir data + save data
						memcpy(Mcd1Data + (i+1) * 128, Mcd2Data + (copy+1) * 128, 128);
						SaveMcd(str, Mcd1Data, (i+1) * 128, 128);
						memcpy(Mcd1Data + (i+1) * 1024 * 8, Mcd2Data + (copy+1) * 1024 * 8, 1024 * 8);
						SaveMcd(str, Mcd1Data, (i+1) * 1024 * 8, 1024 * 8);
					} else { // 2
						Edit_GetText(GetDlgItem(hW,IDC_MCD2), str, 256);
						i = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST2));

						// save dir data + save data
						memcpy(Mcd2Data + (i+1) * 128, Mcd1Data + (copy+1) * 128, 128);
						SaveMcd(str, Mcd2Data, (i+1) * 128, 128);
						memcpy(Mcd2Data + (i+1) * 1024 * 8, Mcd1Data + (copy+1) * 1024 * 8, 1024 * 8);
						SaveMcd(str, Mcd2Data, (i+1) * 1024 * 8, 1024 * 8);
					}

					UpdateMcdDlg();

					return TRUE;
				case IDC_DELETE1:
				{
					McdBlock *Info;
					int mcd = 1;
					int i, xor = 0, j;
					unsigned char *data, *ptr;

					Edit_GetText(GetDlgItem(hW,IDC_MCD1), (LPSTR) str, 256);
					i = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST1));
					data = (unsigned char*) Mcd1Data;

					i++;

					ptr = data + i * 128;

					Info = &Blocks[mcd-1][i-1];

					if ((Info->Flags & 0xF0) == 0xA0) {
						if ((Info->Flags & 0xF) >= 1 &&
							(Info->Flags & 0xF) <= 3) { // deleted
							*ptr = 0x50 | (Info->Flags & 0xF);
						} else return TRUE;
					} else if ((Info->Flags & 0xF0) == 0x50) { // used
						*ptr = 0xA0 | (Info->Flags & 0xF);
					} else { return TRUE; }

					for (j=0; j<127; j++) xor^=*ptr++;
					*ptr = (unsigned) xor;

					SaveMcd(str, (char*) data, i * 128, 128);
					UpdateMcdDlg();
				}

					return TRUE;
				case IDC_DELETE2:
				{
					McdBlock *Info;
					int mcd = 2;
					int i, xor = 0, j;
					unsigned char *data, *ptr;

					Edit_GetText(GetDlgItem(hW,IDC_MCD2), (LPSTR) str, 256);
					i = ListView_GetSelectionMark(GetDlgItem(mcdDlg, IDC_LIST2));
					data = (unsigned char*) Mcd2Data;

					i++;

					ptr = data + i * 128;

					Info = &Blocks[mcd-1][i-1];

					if ((Info->Flags & 0xF0) == 0xA0) {
						if ((Info->Flags & 0xF) >= 1 &&
							(Info->Flags & 0xF) <= 3) { // deleted
							*ptr = 0x50 | (Info->Flags & 0xF);
						} else return TRUE;
					} else if ((Info->Flags & 0xF0) == 0x50) { // used
						*ptr = 0xA0 | (Info->Flags & 0xF);
					} else { return TRUE; }

					for (j=0; j<127; j++) xor^=*ptr++;
					*ptr = (unsigned) xor;

					SaveMcd(str, (char*) data, i * 128, 128);
					UpdateMcdDlg();
				}

					return TRUE;

				case IDC_MCDSEL1: 
					Open_Mcd_Proc(hW, 1); 
					return TRUE;
				case IDC_MCDSEL2: 
					Open_Mcd_Proc(hW, 2); 
					return TRUE;
				case IDC_RELOAD1: 
					Edit_GetText(GetDlgItem(hW,IDC_MCD1), str, 256);
					LoadMcd(1, str);
					UpdateMcdDlg();
					return TRUE;
				case IDC_RELOAD2: 
					Edit_GetText(GetDlgItem(hW,IDC_MCD2), str, 256);
					LoadMcd(2, str);
					UpdateMcdDlg();
					return TRUE;
				case IDC_FORMAT1:
					if (MessageBox(hW, _("Are you sure you want to format this Memory Card?"), _("Confirmation"), MB_YESNO) == IDNO) return TRUE;
					Edit_GetText(GetDlgItem(hW,IDC_MCD1), str, 256);
					CreateMcd(str);
					LoadMcd(1, str);
					UpdateMcdDlg();
					return TRUE;
				case IDC_FORMAT2:
					if (MessageBox(hW, _("Are you sure you want to format this Memory Card?"), _("Confirmation"), MB_YESNO) == IDNO) return TRUE;
					Edit_GetText(GetDlgItem(hW,IDC_MCD2), str, 256);
					CreateMcd(str);
					LoadMcd(2, str);
					UpdateMcdDlg();
					return TRUE;
       			case IDCANCEL:
					LoadMcds(Config.Mcd1, Config.Mcd2);

					EndDialog(hW,FALSE);

					return TRUE;
       			case IDOK:
					Edit_GetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1, 256);
					Edit_GetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2, 256);

					LoadMcds(Config.Mcd1, Config.Mcd2);
					SaveConfig();

					EndDialog(hW,TRUE);

					return TRUE;
			}
		case WM_NOTIFY:
			switch (wParam) {
				case IDC_LIST1: McdListNotify(1, IDC_LIST1, (LPNMHDR)lParam); break;
				case IDC_LIST2: McdListNotify(2, IDC_LIST2, (LPNMHDR)lParam); break;
			}
			return TRUE;
		case WM_TIMER:
			UpdateMcdIcon(1, IDC_LIST1);
			UpdateMcdIcon(2, IDC_LIST2);
			return TRUE;
		case WM_DESTROY:
			DestroyIcon(eICON);
			//KillTimer(hW, 1);
			return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK ConfigureCpuDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	long tmp;

	switch(uMsg) {
		case WM_INITDIALOG:
			Button_SetCheck(GetDlgItem(hW,IDC_XA),      Config.Xa);
			Button_SetCheck(GetDlgItem(hW,IDC_SIO),     Config.Sio);
			Button_SetCheck(GetDlgItem(hW,IDC_MDEC),    Config.Mdec);
			Button_SetCheck(GetDlgItem(hW,IDC_QKEYS),   Config.QKeys);
			Button_SetCheck(GetDlgItem(hW,IDC_CDDA),    Config.Cdda);
			Button_SetCheck(GetDlgItem(hW,IDC_PSXAUTO), Config.PsxAuto);
			//Button_SetCheck(GetDlgItem(hW,IDC_CPU),     Config.Cpu);
			Button_SetCheck(GetDlgItem(hW,IDC_PAUSE),   Config.PauseAfterPlayback);
			Button_SetCheck(GetDlgItem(hW,IDC_PSXOUT),  Config.PsxOut);
			Button_SetCheck(GetDlgItem(hW,IDC_RCNTFIX), Config.RCntFix);
			Button_SetCheck(GetDlgItem(hW,IDC_VSYNCWA), Config.VSyncWA);
			ComboBox_AddString(GetDlgItem(hW,IDC_PSXTYPES),"NTSC");
			ComboBox_AddString(GetDlgItem(hW,IDC_PSXTYPES),"PAL");
			ComboBox_SetCurSel(GetDlgItem(hW,IDC_PSXTYPES),Config.PsxType);

		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDCANCEL: EndDialog(hW,FALSE); return TRUE;
				case IDOK:
					tmp = ComboBox_GetCurSel(GetDlgItem(hW,IDC_PSXTYPES));
					if (tmp == 0) Config.PsxType = 0;
					else Config.PsxType = 1;

					Config.Xa      = Button_GetCheck(GetDlgItem(hW,IDC_XA));
					Config.Sio     = Button_GetCheck(GetDlgItem(hW,IDC_SIO));
					Config.Mdec    = Button_GetCheck(GetDlgItem(hW,IDC_MDEC));
					Config.QKeys   = Button_GetCheck(GetDlgItem(hW,IDC_QKEYS));
					Config.Cdda    = Button_GetCheck(GetDlgItem(hW,IDC_CDDA));
					Config.PsxAuto = Button_GetCheck(GetDlgItem(hW,IDC_PSXAUTO));
					Config.PauseAfterPlayback = Button_GetCheck(GetDlgItem(hW,IDC_PAUSE));
					/*tmp = Config.Cpu;
					Config.Cpu     = Button_GetCheck(GetDlgItem(hW,IDC_CPU));
					if (tmp != Config.Cpu) {
						psxCpu->Shutdown();
						if (Config.Cpu)	
							 psxCpu = &psxInt;
						else psxCpu = &psxRec;
						if (psxCpu->Init() == -1) {
							SysClose();
							exit(1);
						}
						psxCpu->Reset();
					}*/
					Config.PsxOut  = Button_GetCheck(GetDlgItem(hW,IDC_PSXOUT));
					Config.RCntFix = Button_GetCheck(GetDlgItem(hW,IDC_RCNTFIX));
					Config.VSyncWA = Button_GetCheck(GetDlgItem(hW,IDC_VSYNCWA));

					SaveConfig();

					EndDialog(hW,TRUE);

					//if (Config.PsxOut) OpenConsole();
					//else CloseConsole();

					return TRUE;
			}
		}
	}
	return FALSE;
}

void Open_Mcd_Proc(HWND hW, int mcd) {
	OPENFILENAME ofn;
	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];
	char szFilter[1024];
	char *str;

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));
	memset(&szFilter,    0, sizeof(szFilter));

	strcpy(szFilter, _("Psx Mcd Format (*.mcr;*.mc;*.mem;*.vgs;*.mcd;*.gme;*.ddf)"));
	str = szFilter + strlen(szFilter) + 1; 
	strcpy(str, "*.mcr;*.mcd;*.mem;*.gme;*.mc;*.ddf");

	str+= strlen(str) + 1;
	strcpy(str, _("Psx Memory Card (*.mcr;*.mc)"));
	str+= strlen(str) + 1;
	strcpy(str, "*.mcr;0*.mc");

	str+= strlen(str) + 1;
	strcpy(str, _("CVGS Memory Card (*.mem;*.vgs)"));
	str+= strlen(str) + 1;
	strcpy(str, "*.mem;*.vgs");

	str+= strlen(str) + 1;
	strcpy(str, _("Bleem Memory Card (*.mcd)"));
	str+= strlen(str) + 1;
	strcpy(str, "*.mcd");

	str+= strlen(str) + 1;
	strcpy(str, _("DexDrive Memory Card (*.gme)"));
	str+= strlen(str) + 1;
	strcpy(str, "*.gme");

	str+= strlen(str) + 1;
	strcpy(str, _("DataDeck Memory Card (*.ddf)"));
	str+= strlen(str) + 1;
	strcpy(str, "*.ddf");

	str+= strlen(str) + 1;
	strcpy(str, _("All Files"));
	str+= strlen(str) + 1;
	strcpy(str, "*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hW;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= MAXFILENAME;
    ofn.lpstrInitialDir		= "memcards";
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= MAXFILENAME;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "MCR";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		Edit_SetText(GetDlgItem(hW,mcd == 1 ? IDC_MCD1 : IDC_MCD2), szFileName);
		LoadMcd(mcd, szFileName);
		UpdateMcdDlg();
	}
}

int Open_File_Proc(char *file) {
	OPENFILENAME ofn;
	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];
	char szFilter[256];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= gApp.hWnd;

	strcpy(szFilter, _("Psx Exe Format"));
	strcatz(szFilter, "*.*;*.*");

    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= MAXFILENAME;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= MAXFILENAME;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "EXE";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		strcpy(file, szFileName);
		return 1;
	} else
		return 0;
}

#define _ADDSUBMENU(menu, menun, string) \
	submenu[menun] = CreatePopupMenu(); \
	AppendMenu(menu, MF_STRING | MF_POPUP, (UINT)submenu[menun], string);

#define ADDSUBMENU(menun, string) \
	_ADDSUBMENU(gApp.hMenu, menun, string);

#define ADDSUBMENUS(submn, menun, string) \
	submenu[menun] = CreatePopupMenu(); \
	InsertMenu(submenu[submn], 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)submenu[menun], string);

#define ADDMENUITEM(menun, string, id) \
	item.fType = MFT_STRING; \
	item.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID; \
	item.fState = MFS_ENABLED; \
	item.wID = id; \
	sprintf(buf, string); \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

#define ADDMENUITEMDISABLED(menun, string, id) \
	item.fType = MFT_STRING; \
	item.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID; \
	item.fState = MFS_DISABLED; \
	item.wID = id; \
	sprintf(buf, string); \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);


#define ADDMENUITEMC(menun, string, id) \
	item.fType = MFT_STRING; \
	item.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID; \
	item.fState = MFS_ENABLED | MFS_CHECKED; \
	item.wID = id; \
	sprintf(buf, string); \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

#define ADDSEPARATOR(menun) \
	item.fMask = MIIM_TYPE; \
	item.fType = MFT_SEPARATOR; \
	InsertMenuItem(submenu[menun], 0, TRUE, &item);

void CreateMainMenu() {
	MENUITEMINFO item;
	HMENU submenu[256];
	char buf[256];

	item.cbSize = sizeof(MENUITEMINFO);
	item.dwTypeData = buf;
	item.cch = 256;

	gApp.hMenu = CreateMenu();

	ADDSUBMENU(0, _("&File"));
	ADDMENUITEM(0, _("E&xit"), ID_FILE_EXIT);
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("Stop AVI"), ID_END_CAPTURE);
	ADDMENUITEM(0, _("Record &AVI"), ID_START_CAPTURE);
	ADDSEPARATOR(0);
	
	ADDMENUITEM(0, _("Screenshot"), ID_FILE_SCREENSHOT);

	ADDSUBMENUS(0, 3, _("Loadstate"));
	ADDMENUITEM(3, _("Loadstate From..."), ID_FILE_STATES_LOAD_OTHER);
	ADDSEPARATOR(3);
	ADDMENUITEM(3, _("Loadstate Slot 0"), ID_FILE_STATES_LOAD_SLOT0);
	ADDMENUITEM(3, _("Loadstate Slot 9"), ID_FILE_STATES_LOAD_SLOT9);
	ADDMENUITEM(3, _("Loadstate Slot 8"), ID_FILE_STATES_LOAD_SLOT8);
	ADDMENUITEM(3, _("Loadstate Slot 7"), ID_FILE_STATES_LOAD_SLOT7);
	ADDMENUITEM(3, _("Loadstate Slot 6"), ID_FILE_STATES_LOAD_SLOT6);
	ADDMENUITEM(3, _("Loadstate Slot 5"), ID_FILE_STATES_LOAD_SLOT5);
	ADDMENUITEM(3, _("Loadstate Slot 4"), ID_FILE_STATES_LOAD_SLOT4);
	ADDMENUITEM(3, _("Loadstate Slot 3"), ID_FILE_STATES_LOAD_SLOT3);
	ADDMENUITEM(3, _("Loadstate Slot 2"), ID_FILE_STATES_LOAD_SLOT2);
	ADDMENUITEM(3, _("Loadstate Slot 1"), ID_FILE_STATES_LOAD_SLOT1);
	
	ADDSUBMENUS(0, 4, _("Savestate"));
	ADDMENUITEM(4, _("Savestate As..."), ID_FILE_STATES_SAVE_OTHER);
	ADDSEPARATOR(4);
	ADDMENUITEM(4, _("Savestate Slot 0"), ID_FILE_STATES_SAVE_SLOT0);
	ADDMENUITEM(4, _("Savestate Slot 9"), ID_FILE_STATES_SAVE_SLOT9);
	ADDMENUITEM(4, _("Savestate Slot 8"), ID_FILE_STATES_SAVE_SLOT8);
	ADDMENUITEM(4, _("Savestate Slot 7"), ID_FILE_STATES_SAVE_SLOT7);
	ADDMENUITEM(4, _("Savestate Slot 6"), ID_FILE_STATES_SAVE_SLOT6);
	ADDMENUITEM(4, _("Savestate Slot 5"), ID_FILE_STATES_SAVE_SLOT5);
	ADDMENUITEM(4, _("Savestate Slot 4"), ID_FILE_STATES_SAVE_SLOT4);
	ADDMENUITEM(4, _("Savestate Slot 3"), ID_FILE_STATES_SAVE_SLOT3);
	ADDMENUITEM(4, _("Savestate Slot 2"), ID_FILE_STATES_SAVE_SLOT2);
	ADDMENUITEM(4, _("Savestate Slot 1"), ID_FILE_STATES_SAVE_SLOT1);

	ADDSUBMENUS(0, 2, _("&Lua Scripting"));
	ADDMENUITEM(2, _("&Close All Script Windows"), ID_LUA_CLOSE_ALL);
	ADDMENUITEM(2, _("&New Lua Script Window..."), ID_LUA_OPEN);
	ADDMENUITEM(2, _("Recent"), ID_FILE_RECENT_LUA);
	ADDSUBMENUS(0, 1, _("&Movie"));
	ADDMENUITEM(1, _("Read-only"), ID_FILE_READONLY);
	ADDMENUITEM(1, _("Play from &Beginning"), ID_FILE_PLAYBEGINNING);
	ADDMENUITEM(1, _("S&top Movie"), ID_FILE_STOP_MOVIE);
	ADDMENUITEM(1, _("Start &Playback..."), ID_FILE_REPLAY_MOVIE);
	ADDMENUITEM(1, _("Start &Recording..."), ID_FILE_RECORD_MOVIE);
	ADDMENUITEM(1, _("Recent"), ID_FILE_RECENT_MOVIE);
	ADDSEPARATOR(0);
	//ADDMENUITEM(0, _("Run &EXE"), ID_FILE_RUN_EXE); //adelikat: For running SDK made games, disabling in favor of a commandline argument instead
	ADDMENUITEM(0, _("Close CD"), ID_FILE_CLOSE_CD);
	ADDMENUITEM(0, _("Recent"), ID_FILE_RECENT_CD);
	ADDMENUITEM(0, _("Open &CD"), ID_FILE_RUN_CD);

	ADDSUBMENU(0, _("&Emulator"));
	ADDMENUITEM(0, _("Display Input"), ID_EMULATOR_DISPINPUT);
	ADDMENUITEM(0, _("Display Frame counter"), ID_EMULATOR_DISPFRAMECOUNTER);
	ADDMENUITEM(0, _("Display All Text"), ID_EMULATOR_DISPALL);
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("4x"), ID_EMULATOR_4X);
	ADDMENUITEM(0, _("3x"), ID_EMULATOR_3X);
	ADDMENUITEM(0, _("2x"), ID_EMULATOR_2X);
	ADDMENUITEM(0, _("1x"), ID_EMULATOR_1X);
	ADDSEPARATOR(0);	
	ADDMENUITEM(0, _("Open Analog Control"), ID_EMULATOR_ANALOG);
	ADDMENUITEM(0, _("Mute Sound"), ID_EMULATOR_MUTE);
	ADDMENUITEM(0, _("&CD Case Open/Close"), ID_EMULATOR_CDCLOSE);
	ADDMENUITEM(0, _("&Reset"), ID_EMULATOR_RESET);

	ADDSUBMENU(0, _("&Configuration"));
	ADDMENUITEM(0, _("&Options"), ID_CONFIGURATION_CPU);
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("Map &Hotkeys"), ID_CONFIGURATION_MAPHOTKEYS);
	ADDMENUITEM(0, _("&Memory Cards"), ID_CONFIGURATION_MEMORYCARDMANAGER);
	ADDMENUITEM(0, _("&Controllers"), ID_CONFIGURATION_CONTROLLERS);
	ADDMENUITEM(0, _("&Sound"), ID_CONFIGURATION_SOUND);
	ADDMENUITEM(0, _("&Graphics"), ID_CONFIGURATION_GRAPHICS);
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("&Plugins && Bios"), ID_CONFIGURATION);


	ADDSUBMENU(0, _("&Tools"));
	ADDMENUITEM(0, _("&Cheat Editor"), ID_CONFIGURATION_CHEATS);
	ADDMENUITEM(0, _("View Memory"), IDM_MEMORY);
	ADDMENUITEM(0, _("RAM &Poke"), ID_CONFIGURATION_MEMPOKE);
	ADDMENUITEM(0, _("RAM &Watch"), ID_RAM_WATCH);
	ADDMENUITEM(0, _("RAM &Search"), ID_RAM_SEARCH);

	ADDSUBMENU(0, _("&Help"));
	ADDMENUITEM(0, _("&About"), ID_HELP_ABOUT);
	ADDSEPARATOR(0);
	ADDMENUITEM(0, _("&Help File"), ID_HELP_HELP);
	ADDMENUITEM(0, _("&Online Tutorial"), ID_HELP_TUTORIAL);
}

void gpu_ReadConfig(void);

void CreateMainWindow(int nCmdShow) {
	WNDCLASS wc;
	HWND hWnd;
	HBRUSH hBrush;
	COLORREF crBkg = RGB(14,90,193);
	hBrush=CreateSolidBrush(crBkg);

	wc.lpszClassName = "PSXjin Main";
	wc.lpfnWndProc = MainWndProc;
	wc.style = 0;
	wc.hInstance = gApp.hInstance;
	wc.hIcon = LoadIcon(gApp.hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	RegisterClass(&wc);
	gpu_ReadConfig();
	ReadConfig();
	UpdateWindowSizeFromConfig();

	hWnd = CreateWindow("PSXjin Main",
						PCSXRR_NAME_AND_VERSION,
						WS_CAPTION | WS_POPUPWINDOW | WS_MINIMIZEBOX,
						MainWindow_wndx,
						MainWindow_wndy,
						MainWindow_width,
						MainWindow_height,
						NULL,
						NULL,
						gApp.hInstance,
						NULL);

	gApp.hWnd = hWnd;
	ResetMenuSlots();

	CreateMainMenu();
	SetMenu(gApp.hWnd, gApp.hMenu);
	
	RecentCDs.SetGUI_hWnd(gApp.hWnd);
	RecentCDs.SetID(RECENTCD_START);
	RecentCDs.SetMenuID(ID_FILE_RECENT_CD);
	RecentCDs.SetType("CD");
	RecentCDs.MakeRecentMenu(gApp.hInstance);
	RecentCDs.GetRecentItemsFromIni(Config.Conf_File, "General");
	
	RecentMovies.SetGUI_hWnd(gApp.hWnd);
	RecentMovies.SetID(RECENTMOVIE_START);
	RecentMovies.SetMenuID(ID_FILE_RECENT_MOVIE);
	RecentMovies.SetType("Movie");
	RecentMovies.MakeRecentMenu(gApp.hInstance);
	RecentMovies.GetRecentItemsFromIni(Config.Conf_File, "General");
	
	RecentLua.SetGUI_hWnd(gApp.hWnd);
	RecentLua.SetID(RECENTLUA_START);
	RecentLua.SetMenuID(ID_FILE_RECENT_LUA);
	RecentLua.SetType("Lua");
	RecentLua.MakeRecentMenu(gApp.hInstance);
	RecentLua.GetRecentItemsFromIni(Config.Conf_File, "General");

	DragAcceptFiles(hWnd, 1);

	ShowWindow(hWnd, nCmdShow);
}

WIN32_FIND_DATA lFindData;
HANDLE lFind;
int lFirst;

void SaveIni()
{
	char Str_Tmp[1024];

	sprintf(Str_Tmp, "%d", MainWindow_wndx);
	WritePrivateProfileString("General", "Main_x", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", MainWindow_wndy);
	WritePrivateProfileString("General", "Main_y", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", dispFrameCounter);
	WritePrivateProfileString("General", "FrameCounter", Str_Tmp, Config.Conf_File);

	sprintf(Str_Tmp, "%d", AutoRWLoad);
	WritePrivateProfileString("RamWatch", "AutoLoad", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", RWSaveWindowPos);
	WritePrivateProfileString("RamWatch", "RWSaveWindowPos", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", ramw_x);
	WritePrivateProfileString("RamWatch", "ramw_x", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", ramw_y);
	WritePrivateProfileString("RamWatch", "ramw_y", Str_Tmp, Config.Conf_File);

	sprintf(Str_Tmp, "%d", dispAllText );
	WritePrivateProfileString("General", "AllDisplay", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", dispFrameCounter);
	WritePrivateProfileString("General", "FrameCounter", Str_Tmp, Config.Conf_File);
	sprintf(Str_Tmp, "%d", dispInput );
	WritePrivateProfileString("General", "InputDisplay", Str_Tmp, Config.Conf_File);


	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		WritePrivateProfileString("Watches", str, &rw_recent_files[i][0], Config.Conf_File);	
	}

	RecentCDs.SaveRecentItemsToIni(Config.Conf_File, "General");
	RecentMovies.SaveRecentItemsToIni(Config.Conf_File, "General");
	RecentLua.SaveRecentItemsToIni(Config.Conf_File, "General");
	
	WriteConfig();	//Save SPU Settings
}

void LoadIni()
{
	AutoRWLoad = GetPrivateProfileInt("RamWatch", "AutoLoad", 0, Config.Conf_File);
	RWSaveWindowPos = GetPrivateProfileInt("RamWatch", "RWSaveWindowPos", 0, Config.Conf_File);
	ramw_x = GetPrivateProfileInt("RamWatch", "ramw_x", 0, Config.Conf_File);
	ramw_y = GetPrivateProfileInt("RamWatch", "ramw_y", 0, Config.Conf_File);
	MainWindow_wndx = GetPrivateProfileInt("General", "main_x", 0, Config.Conf_File);
	MainWindow_wndy = GetPrivateProfileInt("General", "main_y", 0, Config.Conf_File);
	dispAllText = GetPrivateProfileInt("General", "AllDisplay", 1, Config.Conf_File);
	dispFrameCounter = GetPrivateProfileInt("General", "FrameCounter", 0, Config.Conf_File);
	dispInput = GetPrivateProfileInt("General", "InputDisplay", 0, Config.Conf_File);
	

	if (MainWindow_wndx < -320) MainWindow_wndx = 0;	//Just in case, sometimes windows likes to save -32000 and other odd things
	if (MainWindow_wndy < -240) MainWindow_wndy = 0;	//Just in case, sometimes windows likes to save -32000 and other odd things

	for(int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		char str[256];
		sprintf(str, "Recent Watch %d", i+1);
		GetPrivateProfileString("Watches", str, "", &rw_recent_files[i][0], 1024, Config.Conf_File);
	}

	ReadConfig();	//Load SPU settings
}

int SysInit() {
	//if (Config.PsxOut) 
		//OpenConsole();

	if (psxInit() == -1) return -1;

	#ifdef GTE_DUMP
	gteLog = fopen("gteLog.txt","w");
	#endif

#ifdef EMU_LOG
	emuLog = fopen("emuLog.txt","w");
	setvbuf(emuLog, NULL,  _IONBF, 0);
#endif

	LoadPlugins();
	LoadMcds(Config.Mcd1, Config.Mcd2);
	LoadIni();
	if (dispAllText)
		GPUshowALL();
	if (dispFrameCounter)
		GPUshowframecounter();
	if (dispInput)
		GPUshowInput();
	return 0;
}

void SysReset() 
{
	//Let's do a BIOS check here, if the code gets any further without it, it will be disaster.  This code won't prevent that, but at least it will inform the user!
	//This should have been already checked before the game is even loaded, so in theory this will never appear for the user.
	if (!BIOSExists()) 
	{
		SysMessage ("Could not open bios. This program will likely crash after this box disappears, if it doesn't close immediately then configure your BIOS settings\n");
		return;	//Put the responsiblity of this on the calling function
	}
	psxReset();
}

void SysClose() {
	psxShutdown();
	ReleasePlugins();
	SaveIni();
	if (Config.PsxOut) CloseConsole();

	if (emuLog != NULL) fclose(emuLog);
	#ifdef GTE_DUMP
	if (gteLog != NULL) fclose(gteLog);
	#endif
}

void SysPrintf(char *fmt, ...) {
	va_list list;
	char msg[512];
	DWORD tmp;

	if (!hConsole) return;

	va_start(list,fmt);
	vsprintf(msg,fmt,list);
	va_end(list);

	WriteConsole(hConsole, msg, (DWORD)strlen(msg), &tmp, 0);
#ifdef EMU_LOG
#ifndef LOG_STDOUT
	fprintf(emuLog, "%s", msg);
#endif
#endif
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, _("PSXJIN Message"), 0);
}

static char *err = N_("Error Loading Symbol");
static int errval;

void *SysLoadLibrary(char *lib) {
	return LoadLibrary(lib);
}

void *SysLoadSym(void *lib, char *sym) {
	void *tmp = GetProcAddress((HINSTANCE)lib, sym);
	if (tmp == NULL) errval = 1;
	else errval = 0;
	return tmp;
}

const char *SysLibError() {
	if (errval) { errval = 0; return err; }
	return NULL;
}

void SysCloseLibrary(void *lib) {
	FreeLibrary((HINSTANCE)lib);
}

void SysUpdate() {
	RunMessageLoop();
}

void SysRunGui() {
	SPUmute();
	RestoreWindow();
	RunGui();
	SPUunMute();
}


void WIN32_StartMovieReplay(char* szFilename)
{
	int nRet=1;
	CheckCdrom(); //get CdromId
	if (szFilename != '\0') {
		if (! MOV_ReadMovieFile(szFilename,&Movie) ) {
			char errorMessage[300];
			sprintf(errorMessage, "Movie file \"%s\" doesn't exist.",szFilename);
			MessageBox(NULL, errorMessage, NULL, MB_ICONERROR);
			return;
		}
		GetMovieFilenameMini(Movie.movieFilename);
		Movie.readOnly = 1;
	}
	else
		nRet = MOV_W32_StartReplayDialog();
	if (nRet) {
		if (IsoFile[0] == 0) CfgOpenFile();	//If no game loaded, ask the user for a game
		
		//If still no ISO file selected (because the user cancelled), don't attempt to run the movie
		if (!IsoFile[0] == 0) //adelikat:  Having CfgOpenFile() be value returning is more elegant but this gets the job done
		{
			LoadCdBios = 0;
			OpenPlugins(gApp.hWnd);
			SysReset();
			NeedReset = 0;
			CheckCdrom();
			if (LoadCdrom() == -1) {
				ClosePlugins();
				RestoreWindow();
				SysMessage(_("Could not load Cdrom"));
				return;
			}
			Running = 1;
			RecentMovies.UpdateRecentItems(Movie.movieFilename);
			MOV_StartMovie(MOVIEMODE_PLAY);
			psxCpu->Execute();
		}
	}
}

void WIN32_StartMovieRecord()
{
	int nRet;
	CheckCdrom(); //get CdromId
	nRet = MOV_W32_StartRecordDialog();
	if (nRet) 
	{
		if (IsoFile[0] == 0) CfgOpenFile();	//If no game loaded, ask the user for a game

		//If still no ISO file selected (because the user cancelled), don't attempt to run the movie
		if (!IsoFile[0] == 0) //adelikat:  Having CfgOpenFile() be value returning is more elegant but this gets the job done
		{
			if (Movie.saveStateIncluded)
			{
				SetMenu(gApp.hWnd, NULL);
				OpenPlugins(gApp.hWnd);
				Running = 1;
				MOV_StartMovie(MOVIEMODE_RECORD);
				psxCpu->Execute();
				return;
			}
			LoadCdBios = 0;
			OpenPlugins(gApp.hWnd);
			SysReset();
			NeedReset = 0;
			CheckCdrom();
			if (LoadCdrom() == -1)
			{
				ClosePlugins();
				RestoreWindow();
				SysMessage(_("Could not load Cdrom"));
				return;
			}
			Running = 1;
			RecentMovies.UpdateRecentItems(Movie.movieFilename);
			MOV_StartMovie(MOVIEMODE_RECORD);
			psxCpu->Execute();
		}
	}
}

void WIN32_StartAviRecord()
{
	char nameo[256];
	const char filter[]="AVI Files (*.avi)\0*.avi\0";
	OPENFILENAME ofn;
	char fszDrive[256];
	char fszDirectory[256];
	char fszFilename[256];
	char fszExt[256];

	if (Movie.capture)
		return;

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=gApp.hInstance;
	ofn.lpstrTitle="Save AVI as...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=".\\";
	if(GetSaveFileName(&ofn)) {
		int i;
		i = strlen(nameo);

		//add .avi if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && (i < (256-8))) {
			nameo[i] = '.';
			nameo[i+1] = 'a';
			nameo[i+2] = 'v';
			nameo[i+3] = 'i';
			nameo[i+4] = 0;
		}
	}
	else
		return;

	Movie.capture = 1;
	AVIisCapturing = true;
	fszDrive[0] = '\0';
	fszDirectory[0] = '\0';
	fszFilename[0] = '\0';
	fszExt[0] = '\0';
	_splitpath(nameo, fszDrive, fszDirectory, fszFilename, fszExt);
	strcpy(Movie.AviDrive,fszDrive);
	strcpy(Movie.AviDirectory,fszDirectory);
	strcpy(Movie.AviFnameShort,fszFilename);
	Movie.AviCount = 0;
	sprintf(Movie.aviFilename, "%s%s%s.avi",fszDrive,fszDirectory,fszFilename);		 
	sprintf(Movie.wavFilename, "%s%s%s.wav",fszDrive,fszDirectory,fszFilename);
	OpenPlugins(gApp.hWnd);
	Config.CurWinX = MainWindow_width;
	Config.CurWinY = MainWindow_height-MainWindow_menubar;
	GPUstartAvi(Movie.aviFilename);
	SPUstartWav(Movie.wavFilename);
	if (NeedReset) { SysReset(); NeedReset = 0; }
	Running = 1;
	psxCpu->Execute();
}

void WIN32_SplitAvi()
{
	if (Movie.capture)
	{
		GPUstopAvi();
		Movie.AviCount++;
		sprintf(Movie.aviFilename, "%s%s%03d_%s-%dx%d.avi",Movie.AviDrive,Movie.AviDirectory,Movie.AviCount,Movie.AviFnameShort,MainWindow_width,MainWindow_height-MainWindow_menubar);
		Config.CurWinX = MainWindow_width;
		Config.CurWinY = MainWindow_height-MainWindow_menubar;
		GPUstartAvi(Movie.aviFilename);	
	}
}

void WIN32_StopAviRecord()
{
	if (!Movie.capture)
		return;
	GPUstopAvi();
	SPUstopWav();
	Movie.capture = 0;
	AVIisCapturing = false;
}


INT_PTR CALLBACK DlgMemPoke(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG: {
			SetDlgItemText(hDlg, IDC_NC_ADDRESS, bufPokeAddress);
			SetDlgItemText(hDlg, IDC_NC_NEWVAL, bufPokeNewval);
		}
		return TRUE;
		case WM_QUIT:
		case WM_CLOSE:
		case WM_DESTROY: {
			memPokeHWND = NULL;
			GetDlgItemText(hDlg, IDC_NC_ADDRESS, bufPokeAddress, 7);
			GetDlgItemText(hDlg, IDC_NC_NEWVAL, bufPokeNewval, 7);
			DestroyWindow(hDlg);
			return TRUE;
		}
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {

				case IDC_C_ADD: {
					uint32 address;
					uint8 newval;
					unsigned int scanned;
					GetDlgItemText(hDlg, IDC_NC_ADDRESS, bufPokeAddress, 7);
					GetDlgItemText(hDlg, IDC_NC_NEWVAL, bufPokeNewval, 7);
					ScanAddress(bufPokeAddress,&address);
					sscanf(bufPokeNewval, "%d", &scanned);
					newval = (uint8)(scanned & 0xff);
					psxMemWrite8(address,newval);
					return TRUE;
				}

				case IDCANCEL:
					DestroyWindow(hDlg);
					return TRUE;

				default:
					break;
			}
		}
		default:
			return FALSE;
	}
}

void CreateMemPoke()
{
	if(!memPokeHWND) { // create and show non-modal cheat search window
		memPokeHWND = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAM_POKE), NULL, DlgMemPoke);
		ShowWindow(memPokeHWND, SW_SHOW);
	}
	else // already open so just reactivate the window
		SetActiveWindow(memPokeHWND);
}

void ChangeMenuItemText(int menuitem, std::string text)
{
	MENUITEMINFO moo;
	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_TYPE;
	moo.cch = NULL;
	GetMenuItemInfo(gApp.hMenu, menuitem, FALSE, &moo);
	moo.dwTypeData = (LPSTR)text.c_str();
	SetMenuItemInfo(gApp.hMenu, menuitem, FALSE, &moo);
}

extern char* RealKeyName(int c);
void MakeMenuName(int i, std::string menuname, int menuid)
{
	int mod = EmuCommandTable[i].keymod;
	int key = EmuCommandTable[i].key;

	if (key)
		menuname.append("\t");

	if (mod)
	{
		if(mod == VK_CONTROL)
			menuname.append("Ctrl + ");
		else if(mod == VK_MENU)
			menuname.append("Alt + ");
		else if(mod == VK_SHIFT)
			menuname.append("Shift + ");
	}
	
	if (key)
		menuname.append(RealKeyName(key));

	ChangeMenuItemText(menuid, menuname);
}

void UpdateMenuHotkeys()
{
	//Update all menu items that can be called from a hotkey to include the current hotkey assignment
	
	//File-------------------------------------------------------
	MakeMenuName(EMUCMD_OPENCD, "Open &CD", ID_FILE_RUN_CD);
	MakeMenuName(EMUCMD_SCREENSHOT, "Screenshot", ID_FILE_SCREENSHOT);
	MakeMenuName(EMUCMD_STOPMOVIE, "S&top Movie", ID_FILE_STOP_MOVIE);
	MakeMenuName(EMUCMD_STOPMOVIE, "S&top Movie", ID_FILE_STOP_MOVIE);
	MakeMenuName(EMUCMD_PLAYFROMBEGINNING, "Play from &Beginning", ID_FILE_PLAYBEGINNING);
	MakeMenuName(EMUCMD_RWTOGGLE, "Read-only", ID_FILE_READONLY);
	MakeMenuName(EMUCMD_LOADSTATE10, "Loadstate Slot 0", ID_FILE_STATES_LOAD_SLOT0);
	MakeMenuName(EMUCMD_LOADSTATE1, "Loadstate Slot 1", ID_FILE_STATES_LOAD_SLOT1);
	MakeMenuName(EMUCMD_LOADSTATE2, "Loadstate Slot 2", ID_FILE_STATES_LOAD_SLOT2);
	MakeMenuName(EMUCMD_LOADSTATE3, "Loadstate Slot 3", ID_FILE_STATES_LOAD_SLOT3);
	MakeMenuName(EMUCMD_LOADSTATE4, "Loadstate Slot 4", ID_FILE_STATES_LOAD_SLOT4);
	MakeMenuName(EMUCMD_LOADSTATE5, "Loadstate Slot 5", ID_FILE_STATES_LOAD_SLOT5);
	MakeMenuName(EMUCMD_LOADSTATE6, "Loadstate Slot 6", ID_FILE_STATES_LOAD_SLOT6);
	MakeMenuName(EMUCMD_LOADSTATE7, "Loadstate Slot 7", ID_FILE_STATES_LOAD_SLOT7);
	MakeMenuName(EMUCMD_LOADSTATE8, "Loadstate Slot 8", ID_FILE_STATES_LOAD_SLOT8);
	MakeMenuName(EMUCMD_LOADSTATE9, "Loadstate Slot 9", ID_FILE_STATES_LOAD_SLOT9);
	MakeMenuName(EMUCMD_SAVESTATE10, "Savestate Slot 0", ID_FILE_STATES_SAVE_SLOT0);
	MakeMenuName(EMUCMD_SAVESTATE1, "Savestate Slot 1", ID_FILE_STATES_SAVE_SLOT1);
	MakeMenuName(EMUCMD_SAVESTATE2, "Savestate Slot 2", ID_FILE_STATES_SAVE_SLOT2);
	MakeMenuName(EMUCMD_SAVESTATE3, "Savestate Slot 3", ID_FILE_STATES_SAVE_SLOT3);
	MakeMenuName(EMUCMD_SAVESTATE4, "Savestate Slot 4", ID_FILE_STATES_SAVE_SLOT4);
	MakeMenuName(EMUCMD_SAVESTATE5, "Savestate Slot 5", ID_FILE_STATES_SAVE_SLOT5);
	MakeMenuName(EMUCMD_SAVESTATE6, "Savestate Slot 6", ID_FILE_STATES_SAVE_SLOT6);
	MakeMenuName(EMUCMD_SAVESTATE7, "Savestate Slot 7", ID_FILE_STATES_SAVE_SLOT7);
	MakeMenuName(EMUCMD_SAVESTATE8, "Savestate Slot 8", ID_FILE_STATES_SAVE_SLOT8);
	MakeMenuName(EMUCMD_SAVESTATE9, "Savestate Slot 9", ID_FILE_STATES_SAVE_SLOT9);

	MakeMenuName(EMUCMD_STARTPLAYBACK, "Start &Playback...", ID_FILE_REPLAY_MOVIE);
	MakeMenuName(EMUCMD_STARTRECORDING, "Start &Recording...", ID_FILE_RECORD_MOVIE);
	MakeMenuName(EMUCMD_STARTAVI, "Record &AVI", ID_START_CAPTURE);
	MakeMenuName(EMUCMD_STOPAVI, "Stop AVI", ID_END_CAPTURE);
	MakeMenuName(EMUCMD_LUA_OPEN, "&New Lua Script Window...", ID_LUA_OPEN);

	//Emulator---------------------------------------------------
	MakeMenuName(EMUCMD_RESET, "&Reset", ID_EMULATOR_RESET);
	MakeMenuName(EMUCMD_FRAMECOUNTER, "Display Frame counter", ID_EMULATOR_DISPFRAMECOUNTER);
	MakeMenuName(EMUCMD_INPUTDISPLAY, "Display Input", ID_EMULATOR_DISPINPUT);
	
	//Configuration----------------------------------------------
	MakeMenuName(EMUCMD_CONFGPU, "&Graphics", ID_CONFIGURATION_GRAPHICS);
	MakeMenuName(EMUCMD_CONFSPU, "&Sound", ID_CONFIGURATION_SOUND);
	MakeMenuName(EMUCMD_CONFPAD, "&Controllers", ID_CONFIGURATION_CONTROLLERS);
	MakeMenuName(EMUCMD_MEMORYCARDS, "&Memory Cards", ID_CONFIGURATION_MEMORYCARDMANAGER);
	MakeMenuName(EMUCMD_HOTKEYS, "Map &Hotkeys",ID_CONFIGURATION_MAPHOTKEYS);
	MakeMenuName(EMUCMD_CONFCPU, "&Options", ID_CONFIGURATION_CPU);

	//Tools------------------------------------------------------
	MakeMenuName(EMUCMD_RAMSEARCH, "RAM &Search", ID_RAM_SEARCH);
	MakeMenuName(EMUCMD_RAMWATCH, "RAM &Watch", ID_RAM_WATCH);
	MakeMenuName(EMUCMD_RAMPOKE, "RAM &Poke", ID_CONFIGURATION_MEMPOKE);
	MakeMenuName(EMUCMD_CHEATTOGLE, "&Cheat Editor", ID_CONFIGURATION_CHEATS);
}