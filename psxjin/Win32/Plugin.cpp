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

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include "../plugins.h"
#include "../PsxCommon.h"
#include "../movie.h"
#include "../cheat.h"
#include "../R3000A.h"

#include "Win32.h"
#include "resource.h"
#include "maphkeys.h"
#include "ram_search.h"
#include "ramwatch.h"

extern HWND LuaConsoleHWnd;
extern INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern int iTurboMode;
int iSpeedMode = 7;

int ShowPic=0;
char Text[255];
int ret;

void gpuShowPic() {
	gzFile f;

	if (!ShowPic) {
		unsigned char *pMem;

		pMem = (unsigned char *) malloc(128*96*3);
		if (pMem == NULL) return;
		sprintf(Text, "sstates\\%10.10s.%3.3d", CdromLabel, StatesC);

		GPU_freeze(2, (GPUFreeze_t *)&StatesC);

		f = gzopen(Text, "rb");
		if (f != NULL) {
			gzseek(f, 32, SEEK_SET); // skip header
			gzread(f, pMem, 128*96*3);
			gzclose(f);
		}
		GPU_showScreenPic(pMem);

		free(pMem);
		ShowPic = 1;
	} else { GPU_showScreenPic(NULL); ShowPic = 0; }
}

void WIN32_SaveState(int newState) {
	StatesC=newState-1;
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pxm.%3.3d", szCurrentPath, Movie.movieFilenameMini, StatesC+1);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, StatesC+1);
	GPU_freeze(2, (GPUFreeze_t *)&StatesC);
	ret = SaveState(Text);
	if (ret == 0)
		 sprintf(Text, _("*PCSX*: Saved State %d"), StatesC+1);
	else sprintf(Text, _("*PCSX*: Error Saving State %d"), StatesC+1);
	GPU_displayText(Text);
	if (ShowPic) { ShowPic = 0; gpuShowPic(); }
}

void WIN32_LoadState(int newState) {
	int previousMode = Movie.mode;
	if (Movie.mode == MOVIEMODE_RECORD) {
		if (Movie.readOnly) {
			MOV_WriteMovieFile();
			Movie.mode = MOVIEMODE_PLAY;
		}
	}
	else if (Movie.mode == MOVIEMODE_PLAY) {
		if (!Movie.readOnly) Movie.mode = MOVIEMODE_RECORD;
	}
	StatesC=newState-1;
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pxm.%3.3d", szCurrentPath, Movie.movieFilenameMini, StatesC+1);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, StatesC+1);
	ret = LoadState(Text);
	if (ret == 0)
		sprintf(Text, _("*PCSX*: Loaded State %d"), StatesC+1);
	else {
		sprintf(Text, _("*PCSX*: Error Loading State %d"), StatesC+1);
		Movie.mode = previousMode;
	}
	GPU_displayText(Text);
	UpdateToolWindows();
}

char *GetSavestateFilename(int newState) {
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pxm.%3.3d", szCurrentPath, Movie.movieFilenameMini, newState);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, newState);
	return Text;
}

void PADhandleKey(int key) {
	int i;
	int modifiers = 0;
	if(GetAsyncKeyState(VK_CONTROL))
		modifiers = VK_CONTROL;
	else if(GetAsyncKeyState(VK_MENU))
		modifiers = VK_MENU;
	else if(GetAsyncKeyState(VK_SHIFT))
		modifiers = VK_SHIFT;

	for (i = EMUCMD_LOADSTATE1; i <= EMUCMD_LOADSTATE1+9; i++) {
		if(key == EmuCommandTable[i].key
		&& modifiers == EmuCommandTable[i].keymod)
		{
			iLoadStateFrom=i-EMUCMD_LOADSTATE1+1;
			return;
		}
	}

	for (i = EMUCMD_SAVESTATE1; i <= EMUCMD_SAVESTATE1+9; i++) {
		if(key == EmuCommandTable[i].key
		&& modifiers == EmuCommandTable[i].keymod)
		{
			iSaveStateTo=i-EMUCMD_SAVESTATE1+1;
			return;
		}
	}

	for (i = EMUCMD_SELECTSTATE1; i <= EMUCMD_SELECTSTATE1+9; i++) {
		if(key == EmuCommandTable[i].key
		&& modifiers == EmuCommandTable[i].keymod)
		{
			StatesC=i-EMUCMD_SELECTSTATE1;
			GPU_freeze(2, (GPUFreeze_t *)&StatesC);
			if (ShowPic) { ShowPic = 0; gpuShowPic(); }
			sprintf(Text, "*PCSX*: State %d Selected", StatesC+1);
			GPU_displayText(Text);
			return;
		}
	}

	if(key == EmuCommandTable[EMUCMD_PREVIOUSSTATE].key
	&& modifiers == EmuCommandTable[EMUCMD_PREVIOUSSTATE].keymod)
	{
		if (StatesC == 0)
			StatesC = 9;
		else
			StatesC--;

		GPU_freeze(2, (GPUFreeze_t *)&StatesC);
		if (ShowPic) { ShowPic = 0; gpuShowPic(); }
		sprintf(Text, "*PCSX*: State %d Selected", StatesC+1);
		GPU_displayText(Text);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_NEXTSTATE].key
	&& modifiers == EmuCommandTable[EMUCMD_NEXTSTATE].keymod)
	{
		if (StatesC == 9)
			StatesC = 0;
		else
			StatesC++;

		GPU_freeze(2, (GPUFreeze_t *)&StatesC);
		if (ShowPic) { ShowPic = 0; gpuShowPic(); }
		sprintf(Text, "*PCSX*: State %d Selected", StatesC+1);
		GPU_displayText(Text);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LOADSTATE].key
	&& modifiers == EmuCommandTable[EMUCMD_LOADSTATE].keymod)
	{
		iLoadStateFrom=StatesC+1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SAVESTATE].key
	&& modifiers == EmuCommandTable[EMUCMD_SAVESTATE].keymod)
	{
		iSaveStateTo=StatesC+1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_MENU].key
	&& modifiers == EmuCommandTable[EMUCMD_MENU].keymod)
	{
		iCallW32Gui=1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_PAUSE].key
	&& modifiers == EmuCommandTable[EMUCMD_PAUSE].keymod)
	{
		if (!iPause)
			iDoPauseAtVSync=1;
		else
			iPause=0;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_FRAMEADVANCE].key
	&& modifiers == EmuCommandTable[EMUCMD_FRAMEADVANCE].keymod)
	{
		iFrameAdvance=1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_VSYNCADVANCE].key
	&& modifiers == EmuCommandTable[EMUCMD_VSYNCADVANCE].keymod)
	{
		iFrameAdvance=1;
		iGpuHasUpdated=1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RWTOGGLE].key
	&& modifiers == EmuCommandTable[EMUCMD_RWTOGGLE].keymod)
	{
		Movie.readOnly^=1;
		if (Movie.readOnly)
			GPU_displayText("*PCSX*: Read-Only mode");
		else
			GPU_displayText("*PCSX*: Read+Write mode");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LAGCOUNTERRESET].key
	&& modifiers == EmuCommandTable[EMUCMD_LAGCOUNTERRESET].keymod)
	{
		Movie.lagCounter=0;
		GPU_setlagcounter(Movie.lagCounter);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SCREENSHOT].key
	&& modifiers == EmuCommandTable[EMUCMD_SCREENSHOT].keymod)
	{
		GPU_makeSnapshot();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDNORMAL].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDNORMAL].keymod)
	{
		SetEmulationSpeed(EMUSPEED_NORMAL);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDTURBO].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDTURBO].keymod)
	{
		SetEmulationSpeed(EMUSPEED_TURBO);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDMAXIMUM].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDMAXIMUM].keymod)
	{
		SetEmulationSpeed(EMUSPEED_MAXIMUM);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_TURBOMODE].key
	&& modifiers == EmuCommandTable[EMUCMD_TURBOMODE].keymod)
	{
		if (!iTurboMode) SetEmulationSpeed(EMUSPEED_TURBO);
		iTurboMode = 1;
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDDEC].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDDEC].keymod)
	{
		SetEmulationSpeed(EMUSPEED_SLOWER);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDINC].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDINC].keymod)
	{
		SetEmulationSpeed(EMUSPEED_FASTER);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_FRAMECOUNTER].key
	&& modifiers == EmuCommandTable[EMUCMD_FRAMECOUNTER].keymod)
	{
		GPU_showframecounter();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFCPU].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFCPU].keymod)
	{
		DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CPUCONF), gApp.hWnd, (DLGPROC)ConfigureCpuDlgProc);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMWATCH].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMWATCH].keymod)
	{
		CreateMemWatch();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CHEATEDITOR].key
	&& modifiers == EmuCommandTable[EMUCMD_CHEATEDITOR].keymod)
	{
		PCSXRemoveCheats();
		CreateCheatEditor();
		PCSXApplyCheats();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMSEARCH].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMSEARCH].keymod)
	{
		CreateMemSearch();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMPOKE].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMPOKE].keymod)
	{
		CreateMemPoke();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFGPU].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFGPU].keymod)
	{
		if (GPU_configure) GPU_configure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFSPU].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFSPU].keymod)
	{
		SPUconfigure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFCDR].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFCDR].keymod)
	{
		CDRconfigure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFPAD].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFPAD].keymod)
	{
		if (PAD1_configure) PAD1_configure();
		if (strcmp(Config.Pad1, Config.Pad2)) if (PAD2_configure) PAD2_configure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_MEMORYCARDS].key
	&& modifiers == EmuCommandTable[EMUCMD_MEMORYCARDS].keymod)
	{
		DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MCDCONF), gApp.hWnd, (DLGPROC)ConfigureMcdsDlgProc);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STARTRECORDING].key
	&& modifiers == EmuCommandTable[EMUCMD_STARTRECORDING].keymod)
	{
		if (Movie.mode == MOVIEMODE_INACTIVE)
			WIN32_StartMovieRecord();
		else
			GPU_displayText("*PCSX*: error: Movie Already Active");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STARTPLAYBACK].key
	&& modifiers == EmuCommandTable[EMUCMD_STARTPLAYBACK].keymod)
	{
		if (Movie.mode == MOVIEMODE_INACTIVE)
			WIN32_StartMovieReplay(0);
		else
			GPU_displayText("*PCSX*: error: Movie Already Active");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_PLAYFROMBEGINNING].key
	&& modifiers == EmuCommandTable[EMUCMD_PLAYFROMBEGINNING].keymod)
	{
		if (Movie.mode != MOVIEMODE_INACTIVE) {
			MOV_StopMovie();
			WIN32_StartMovieReplay(Movie.movieFilename);
		}
		else
			GPU_displayText("*PCSX*: error: No Movie To Play");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STOPMOVIE].key
	&& modifiers == EmuCommandTable[EMUCMD_STOPMOVIE].keymod)
	{
		if (Movie.mode != MOVIEMODE_INACTIVE) {
			MOV_StopMovie();
			GPU_displayText("*PCSX*: Stop Movie");
			EnableMenuItem(gApp.hMenu,ID_FILE_RECORD_MOVIE,MF_ENABLED);
			EnableMenuItem(gApp.hMenu,ID_FILE_REPLAY_MOVIE,MF_ENABLED);
			EnableMenuItem(gApp.hMenu,ID_FILE_STOP_MOVIE,MF_GRAYED);
		}
		else
			GPU_displayText("*PCSX*: error: No Movie To Stop");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CHEATTOGLE].key
	&& modifiers == EmuCommandTable[EMUCMD_CHEATTOGLE].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			if (Movie.cheatListIncluded) {
				MovieControl.cheats ^= 1;
				if (!cheatsEnabled) {
					if (MovieControl.cheats)
						GPU_displayText("*PCSX*: Cheats Will Activate On Next Frame");
					else
						GPU_displayText("*PCSX*: Cheats Won't Activate On Next Frame");
				}
				else {
					if (MovieControl.cheats)
						GPU_displayText("*PCSX*: Cheats Will Deactivate On Next Frame");
					else
						GPU_displayText("*PCSX*: Cheats Won't Deactivate On Next Frame");
				}
			}
		}
		else {
			cheatsEnabled ^= 1;
			if (cheatsEnabled)
				GPU_displayText(_("*PCSX*: Cheats Enabled"));
			else
				GPU_displayText(_("*PCSX*: Cheats Disabled"));
			PCSXApplyCheats();
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SIOIRQ].key
	&& modifiers == EmuCommandTable[EMUCMD_SIOIRQ].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.sioIrq ^= 1;
			if (!Config.Sio) {
				if (MovieControl.sioIrq)
					GPU_displayText("*PCSX*: Sio Irq Will Activate On Next Frame");
				else
					GPU_displayText("*PCSX*: Sio Irq Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.sioIrq)
					GPU_displayText("*PCSX*: Sio Irq Will Deactivate On Next Frame");
				else
					GPU_displayText("*PCSX*: Sio Irq Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.Sio ^= 0x1;
			if (Config.Sio)
				GPU_displayText(_("*PCSX*: Sio Irq Always Enabled"));
			else
				GPU_displayText(_("*PCSX*: Sio Irq Not Always Enabled"));
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RCNTFIX].key
	&& modifiers == EmuCommandTable[EMUCMD_RCNTFIX].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.RCntFix ^= 1;
			if (!Config.RCntFix) {
				if (MovieControl.RCntFix)
					GPU_displayText("*PCSX*: PE2 Fix Will Activate On Next Frame");
				else
					GPU_displayText("*PCSX*: PE2 Fix Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.RCntFix)
					GPU_displayText("*PCSX*: PE2 Fix Will Deactivate On Next Frame");
				else
					GPU_displayText("*PCSX*: PE2 Fix Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.RCntFix ^= 0x1;
			if (Config.RCntFix)
				GPU_displayText(_("*PCSX*: Parasite Eve 2 Fix Enabled"));
			else
				GPU_displayText(_("*PCSX*: Parasite Eve 2 Fix Disabled"));
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_VSYNCWA].key
	&& modifiers == EmuCommandTable[EMUCMD_VSYNCWA].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.VSyncWA ^= 1;
			if (!Config.VSyncWA) {
				if (MovieControl.VSyncWA)
					GPU_displayText("*PCSX*: RE 2/3 Fix Will Activate On Next Frame");
				else
					GPU_displayText("*PCSX*: RE 2/3 Fix Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.VSyncWA)
					GPU_displayText("*PCSX*: RE 2/3 Fix Will Deactivate On Next Frame");
				else
					GPU_displayText("*PCSX*: RE 2/3 Fix Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.VSyncWA ^= 0x1;
			if (Config.VSyncWA)
				GPU_displayText(_("*PCSX*: Resident Evil 2/3 Fix Enabled"));
			else
				GPU_displayText(_("*PCSX*: Resident Evil 2/3 Fix Disabled"));
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LUA_OPEN].key
	&& modifiers == EmuCommandTable[EMUCMD_LUA_OPEN].keymod)
	{
		if(!LuaConsoleHWnd)
		{
			LuaConsoleHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_LUA), NULL, (DLGPROC) DlgLuaScriptDialog);
		}
		else
			SetForegroundWindow(LuaConsoleHWnd);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LUA_STOP].key
	&& modifiers == EmuCommandTable[EMUCMD_LUA_STOP].keymod)
	{
		PCSX_LuaStop();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LUA_RELOAD].key
	&& modifiers == EmuCommandTable[EMUCMD_LUA_RELOAD].keymod)
	{
		PCSX_ReloadLuaCode();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMSEARCH_PERFORM].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMSEARCH_PERFORM].keymod)
	{
		MemSearchCommand(IDC_C_SEARCH);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMSEARCH_REFRESH].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMSEARCH_REFRESH].keymod)
	{
		UpdateMemSearch();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMSEARCH_RESET].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMSEARCH_RESET].keymod)
	{
		MemSearchCommand(IDC_C_RESET);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RESET].key
	&& modifiers == EmuCommandTable[EMUCMD_RESET].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.reset ^= 1;
			if (MovieControl.reset)
				GPU_displayText("*PCSX*: CPU Will Reset On Next Frame");
			else
				GPU_displayText("*PCSX*: CPU Won't Reset On Next Frame");
		}
		else {
			GPU_displayText("*PCSX*: CPU Reset");
			LoadCdBios = 0;
			SysReset();
			NeedReset = 0;
			CheckCdrom();
			if (LoadCdrom() == -1)
				SysMessage(_("Could not load Cdrom"));
			Running = 1;
			psxCpu->Execute();
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CDCASE].key
	&& modifiers == EmuCommandTable[EMUCMD_CDCASE].keymod)
	{
		if (Movie.mode == MOVIEMODE_RECORD) {
			MovieControl.cdCase ^= 1;
			if (cdOpenCase < 0) {
				if (MovieControl.cdCase)
					GPU_displayText("*PCSX*: CD Case Will Close On Next Frame");
				else
					GPU_displayText("*PCSX*: CD Case Won't Close On Next Frame");
			}
			else {
				if (MovieControl.cdCase)
					GPU_displayText("*PCSX*: CD Case Will Open On Next Frame");
				else
					GPU_displayText("*PCSX*: CD Case Won't Open On Next Frame");
			}
		}
		else {
			cdOpenCase ^= -1;
			if (cdOpenCase < 0) {
				GPU_displayText(_("*PCSX*: CD Case Opened"));
			}
			else {
				GPU_displayText(_("*PCSX*: CD Case Closed"));
				CDRclose();
				CDRopen();
				CheckCdrom();
				if (LoadCdrom() == -1)
					SysMessage(_("Could not load Cdrom"));
			}
		}
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STARTAVI].key
	&& modifiers == EmuCommandTable[EMUCMD_STARTAVI].keymod)
	{
		WIN32_StartAviRecord();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STOPAVI].key
	&& modifiers == EmuCommandTable[EMUCMD_STOPAVI].keymod)
	{
		WIN32_StopAviRecord();
		return;
	}
}

char charsTable[4] = { '|', '/', '-', '\\' };

BOOL CALLBACK ConnectDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[256];
	static int waitState;

	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Connecting..."));

			sprintf(str, _("Please wait while connecting... %c\n"), charsTable[waitState]);
			Static_SetText(GetDlgItem(hW, IDC_CONNECTSTR), str);
			SetTimer(hW, 0, 100, NULL);
			return TRUE;

		case WM_TIMER:
			if (++waitState == 4) waitState = 0;
			sprintf(str, _("Please wait while connecting... %c\n"), charsTable[waitState]);
			Static_SetText(GetDlgItem(hW, IDC_CONNECTSTR), str);
			return TRUE;

/*		case WM_COMMAND:
			switch (LOWORD(wParam)) {
       			case IDCANCEL:
					WaitCancel = 1;
					return TRUE;
			}*/
	}

	return FALSE;
}

int NetOpened = 0;

bool OpenPlugins(HWND hWnd) {
	int ret;

	GPU_clearDynarec(clearDynarec);

	ret = CDRopen();
	if (ret == 2) return false;	//adelikat: using 2 to mean "Do nothing" for when the user cancells the open file dialog
	if (ret < 0) { SysMessage (_("Error Opening CDR Plugin")); return false; }

	SetCurrentDirectory(PcsxDir);
	if (Config.UseNet && NetOpened == 0) {
		netInfo info;
		char path[256];

		strcpy(info.EmuName, "PCSX v" PCSX_VERSION);
		strncpy(info.CdromID, CdromId, 9);
		strncpy(info.CdromLabel, CdromLabel, 9);
		info.psxMem = psxM;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.GPU_displayText = GPU_displayText;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.PAD_setSensitive = PAD1_setSensitive;
		sprintf(path, "%s%s", Config.BiosDir, Config.Bios);
		strcpy(info.BIOSpath, path);
		strcpy(info.MCD1path, Config.Mcd1);
		strcpy(info.MCD2path, Config.Mcd2);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Gpu);
		strcpy(info.GPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Spu);
		strcpy(info.SPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Cdr);
		strcpy(info.CDRpath, path);
		NET_setInfo(&info);

		ret = NET_open(hWnd);
		if (ret < 0) Config.UseNet = 0;
		else {
			HWND hW = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_CONNECT), gApp.hWnd, ConnectDlgProc);
			ShowWindow(hW, SW_SHOW);

			if (NET_queryPlayer() == 1) {
				if (SendPcsxInfo() == -1) Config.UseNet = 0;
			} else {
				if (RecvPcsxInfo() == -1) Config.UseNet = 0;
			}

			DestroyWindow(hW);
		}
		NetOpened = 1;
	} else if (Config.UseNet) {
		NET_resume();
	}

	ret = GPU_open(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening GPU Plugin (%d)"), ret); return false; }
	ret = SPUopen(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening SPU Plugin (%d)"), ret); return false; }
	ret = PAD1_open(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening PAD1 Plugin (%d)"), ret); return false; }
	ret = PAD2_open(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening PAD2 Plugin (%d)"), ret); return false; }

	SetCurrentDirectory(PcsxDir);
//	ShowCursor(FALSE);
	GPU_sendFpLuaGui(PCSX_LuaGui);
	return true;
}

void ClosePlugins() {
	int ret;

	UpdateMenuSlots();
	ret = CDRclose();
	if (ret < 0) { SysMessage (_("Error Closing CDR Plugin")); return; }
	ret = GPU_close();
	if (ret < 0) { SysMessage (_("Error Closing GPU Plugin")); return; }
	ret = PAD1_close();
	if (ret < 0) { SysMessage (_("Error Closing PAD1 Plugin")); return; }
	ret = PAD2_close();
	if (ret < 0) { SysMessage (_("Error Closing PAD2 Plugin")); return; }

	if (Config.UseNet) {
		NET_pause();
	}
}

void ResetPlugins() {
	int ret;

	CDRshutdown();
	GPU_shutdown();
	SPUshutdown();
	PAD1_shutdown();
	PAD2_shutdown();
	if (Config.UseNet) NET_shutdown(); 

	ret = CDRinit();
	if (ret != 0) { SysMessage (_("CDRinit error: %d"), ret); return; }
	ret = GPU_init();
	if (ret != 0) { SysMessage (_("GPUinit error: %d"), ret); return; }
	ret = SPUinit();
	if (ret != 0) { SysMessage (_("SPUinit error: %d"), ret); return; }
	ret = PAD1_init(1);
	if (ret != 0) { SysMessage (_("PAD1init error: %d"), ret); return; }
	ret = PAD2_init(2);
	if (ret != 0) { SysMessage (_("PAD2init error: %d"), ret); return; }
	if (Config.UseNet) {
		ret = NET_init();
		if (ret < 0) { SysMessage (_("NETinit error: %d"), ret); return; }
	}

	NetOpened = 0;
}

void SetEmulationSpeed(int cmd) {
	if(cmd == EMUSPEED_TURBO)
		GPU_setspeedmode(0);
	else if(cmd == EMUSPEED_MAXIMUM)
		GPU_setspeedmode(13);
	else if(cmd == EMUSPEED_NORMAL)
		GPU_setspeedmode(7);
	else if(cmd == EMUSPEED_FASTEST)
		GPU_setspeedmode(12);
	else if(cmd == EMUSPEED_SLOWEST)
		GPU_setspeedmode(1);
	else if(cmd == EMUSPEED_SLOWER) {
		if (iSpeedMode>1) iSpeedMode--;
		GPU_setspeedmode(iSpeedMode);
	}
	else if(cmd == EMUSPEED_FASTER) {
		if (iSpeedMode<12) iSpeedMode++;
		GPU_setspeedmode(iSpeedMode);
	}
}
