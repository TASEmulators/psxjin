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
#include <time.h>
#include <stdio.h>
#include <string>
#include "../plugins.h"
#include "../PsxCommon.h"
#include "../movie.h"
#include "../cheat.h"
#include "../R3000A.h"
#include "movie.h"

#include "Win32.h"
#include "resource.h"
#include "maphkeys.h"
#include "ram_search.h"
#include "ramwatch.h"
#include "../spu/spu.h"
#include "analog.h"

extern HWND LuaConsoleHWnd;
extern INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern int iTurboMode;
int iSpeedMode = 7;

int ShowPic=0;
char Text[255];
int ret;

void PlayMovieFromBeginning()
{
	if (Movie.mode != MOVIEMODE_INACTIVE) {
		MOV_StopMovie();
		WIN32_StartMovieReplay(Movie.movieFilename);
	}
	else
		GPUdisplayText("*PSXjin*: error: No Movie To Play");
	return;
}

void ReadonlyToggle()
{
	Movie.readOnly^=1;
	if (Movie.readOnly)
		GPUdisplayText("*PSXjin*: Read-Only mode");
	else
		GPUdisplayText("*PSXjin*: Read+Write mode");
	return;
}

void CDOpenClose()
{
	if (Movie.mode == MOVIEMODE_RECORD)
	{
		MovieControl.cdCase ^= 1;
		if (cdOpenCase < 0) {
			if (MovieControl.cdCase)
				GPUdisplayText("*PSXjin*: CD Case Will Close On Next Frame");
			else
				GPUdisplayText("*PSXjin*: CD Case Won't Close On Next Frame");
		}
		else {
			if (MovieControl.cdCase)
				GPUdisplayText("*PSXjin*: CD Case Will Open On Next Frame");
			else
				GPUdisplayText("*PSXjin*: CD Case Won't Open On Next Frame");
		}
	}
	else 
	{
		cdOpenCase ^= -1;
		if (cdOpenCase < 0) {
			GPUdisplayText(_("*PSXjin*: CD Case Opened"));
		}
		else {
			GPUdisplayText(_("*PSXjin*: CD Case Closed"));
			CDRclose();
			CDRopen(IsoFile);
			CheckCdrom();
			if (LoadCdrom() == -1)
				SysMessage(_("Could not load Cdrom"));
		}
	}
		return;
}

void gpuShowPic() {
	gzFile f;

	if (!ShowPic) {
		unsigned char *pMem;

		pMem = (unsigned char *) malloc(128*96*3);
		if (pMem == NULL) return;
		sprintf(Text, "sstates\\%10.10s.%3.3d", CdromLabel, StatesC);

		GPUfreeze(2, (GPUFreeze_t *)&StatesC);

		f = gzopen(Text, "rb");
		if (f != NULL) {
			gzseek(f, 32, SEEK_SET); // skip header
			gzread(f, pMem, 128*96*3);
			gzclose(f);
		}
		GPUshowScreenPic(pMem);

		free(pMem);
		ShowPic = 1;
	} else { GPUshowScreenPic(NULL); ShowPic = 0; }
}

void WIN32_SaveState(int newState) {
	StatesC=newState-1;
	if (StatesC == -1) StatesC = 9;
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pjm.%3.3d", szCurrentPath, Movie.movieFilenameMini, StatesC+1);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, StatesC+1);
	GPUfreeze(2, (GPUFreeze_t *)&StatesC);
	ret = SaveState(Text);
	if (ret == 0)
		 sprintf(Text, _("*PSXjin*: Saved State %d"), StatesC+1);
	else sprintf(Text, _("*PSXjin*: Error Saving State %d"), StatesC+1);
	GPUdisplayText(Text);
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
	if (StatesC == -1) StatesC = 9;
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pjm.%3.3d", szCurrentPath, Movie.movieFilenameMini, StatesC+1);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, StatesC+1);
	ret = LoadState(Text);
	if (ret == 0)
		sprintf(Text, _("*PSXjin*: Loaded State %d"), StatesC+1);
	else {
		sprintf(Text, _("*PSXjin*: Error Loading State %d"), StatesC+1);
		Movie.mode = previousMode;
	}
	GPUdisplayText(Text);
	UpdateToolWindows();
}

char *GetSavestateFilename(int newState) {
	if (Movie.mode != MOVIEMODE_INACTIVE)
		sprintf(Text, "%ssstates\\%s.pjm.%3.3d", szCurrentPath, Movie.movieFilenameMini, newState);
	else
		sprintf(Text, "%ssstates\\%10.10s.%3.3d", szCurrentPath, CdromLabel, newState);
	return Text;
}

void PADhandleKey(int key) {
	char tempstr[1024];
	const int Frates[] = {1, 4, 9, 15, 22, 30, 60, 75, 90, 120, 240, 480, 9999};
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
			GPUfreeze(2, (GPUFreeze_t *)&StatesC);
			if (ShowPic) { ShowPic = 0; gpuShowPic(); }
			sprintf(Text, "*PSXjin*: State %d Selected", StatesC+1);
			GPUdisplayText(Text);
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

		GPUfreeze(2, (GPUFreeze_t *)&StatesC);
		if (ShowPic) { ShowPic = 0; gpuShowPic(); }
		sprintf(Text, "*PSXjin*: State %d Selected", StatesC+1);
		GPUdisplayText(Text);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_NEXTSTATE].key
	&& modifiers == EmuCommandTable[EMUCMD_NEXTSTATE].keymod)
	{
		if (StatesC == 9)
			StatesC = 0;
		else
			StatesC++;

		GPUfreeze(2, (GPUFreeze_t *)&StatesC);
		if (ShowPic) { ShowPic = 0; gpuShowPic(); }
		sprintf(Text, "*PSXjin*: State %d Selected", StatesC+1);
		GPUdisplayText(Text);
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

	if(key == EmuCommandTable[EMUCMD_OPENCD].key
	&& modifiers == EmuCommandTable[EMUCMD_OPENCD].keymod)
	{
		SendMessage(gApp.hWnd, WM_COMMAND, (WPARAM)ID_FILE_RUN_CD,(LPARAM)(NULL));
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
		ReadonlyToggle();
	}

	if(key == EmuCommandTable[EMUCMD_LAGCOUNTERRESET].key
	&& modifiers == EmuCommandTable[EMUCMD_LAGCOUNTERRESET].keymod)
	{
		Movie.lagCounter=0;
		GPUsetlagcounter(Movie.lagCounter);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SCREENSHOT].key
	&& modifiers == EmuCommandTable[EMUCMD_SCREENSHOT].keymod)
	{
		GPUmakeSnapshot();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDNORMAL].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDNORMAL].keymod)
	{
		SetEmulationSpeed(EMUSPEED_NORMAL);
		GPUdisplayText("Speed at 100%");
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
		sprintf(tempstr,"Speed set at %d%%",(100 * Frates[iSpeedMode-1])/60);
		GPUdisplayText(tempstr);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_SPEEDINC].key
	&& modifiers == EmuCommandTable[EMUCMD_SPEEDINC].keymod)
	{
		SetEmulationSpeed(EMUSPEED_FASTER);
		sprintf(tempstr,"Speed set at %d%%",(100 * Frates[iSpeedMode-1])/60);
		GPUdisplayText(tempstr);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_FRAMECOUNTER].key
	&& modifiers == EmuCommandTable[EMUCMD_FRAMECOUNTER].keymod)
	{
		GPUshowframecounter();
		return;
	}
	if(key == EmuCommandTable[EMUCMD_INPUTDISPLAY].key
	&& modifiers == EmuCommandTable[EMUCMD_INPUTDISPLAY].keymod)
	{
		GPUshowInput();
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
		if(!RamWatchHWnd)
		{
			RamWatchHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAMWATCH), NULL, (DLGPROC) RamWatchProc);
		}
		else
			SetForegroundWindow(RamWatchHWnd);
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CHEATEDITOR].key
	&& modifiers == EmuCommandTable[EMUCMD_CHEATEDITOR].keymod)
	{
		PSXjinRemoveCheats();
		CreateCheatEditor();
		PSXjinApplyCheats();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_RAMSEARCH].key
	&& modifiers == EmuCommandTable[EMUCMD_RAMSEARCH].keymod)
	{
		if(!RamSearchHWnd)
		{
			reset_address_info();
			RamSearchHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_RAMSEARCH), NULL, (DLGPROC) RamSearchProc);
		}
		else
			SetForegroundWindow(RamSearchHWnd);
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
		GPUconfigure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFSPU].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFSPU].keymod)
	{
		SPUconfigure();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_HOTKEYS].key
	&& modifiers == EmuCommandTable[EMUCMD_HOTKEYS].keymod)
	{
		MHkeysCreate();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CONFPAD].key
	&& modifiers == EmuCommandTable[EMUCMD_CONFPAD].keymod)
	{
		if (!IsoFile[0])
		{
			if (PAD1_configure) PAD1_configure();
			if (strcmp(Config.Pad1, Config.Pad2)) if (PAD2_configure) PAD2_configure();
		}
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
			GPUdisplayText("*PSXjin*: error: Movie Already Active");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_STARTPLAYBACK].key
	&& modifiers == EmuCommandTable[EMUCMD_STARTPLAYBACK].keymod)
	{
		if (Movie.mode == MOVIEMODE_INACTIVE)
			WIN32_StartMovieReplay(0);
		else
			GPUdisplayText("*PSXjin*: error: Movie Already Active");
		return;
	}

	if(key == EmuCommandTable[EMUCMD_PLAYFROMBEGINNING].key
	&& modifiers == EmuCommandTable[EMUCMD_PLAYFROMBEGINNING].keymod)
	{
		PlayMovieFromBeginning();
	}

	if(key == EmuCommandTable[EMUCMD_STOPMOVIE].key
	&& modifiers == EmuCommandTable[EMUCMD_STOPMOVIE].keymod)
	{
		if (Movie.mode != MOVIEMODE_INACTIVE) {
			MOV_StopMovie();
			GPUdisplayText("*PSXjin*: Stop Movie");
			EnableMenuItem(gApp.hMenu,ID_FILE_RECORD_MOVIE,MF_ENABLED);
			EnableMenuItem(gApp.hMenu,ID_FILE_REPLAY_MOVIE,MF_ENABLED);
			EnableMenuItem(gApp.hMenu,ID_FILE_STOP_MOVIE,MF_GRAYED);
		}
		else
			GPUdisplayText("*PSXjin*: error: No Movie To Stop");
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
						GPUdisplayText("*PSXjin*: Cheats Will Activate On Next Frame");
					else
						GPUdisplayText("*PSXjin*: Cheats Won't Activate On Next Frame");
				}
				else {
					if (MovieControl.cheats)
						GPUdisplayText("*PSXjin*: Cheats Will Deactivate On Next Frame");
					else
						GPUdisplayText("*PSXjin*: Cheats Won't Deactivate On Next Frame");
				}
			}
		}
		else {
			cheatsEnabled ^= 1;
			if (cheatsEnabled)
				GPUdisplayText(_("*PSXjin*: Cheats Enabled"));
			else
				GPUdisplayText(_("*PSXjin*: Cheats Disabled"));
			PSXjinApplyCheats();
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
					GPUdisplayText("*PSXjin*: Sio Irq Will Activate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: Sio Irq Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.sioIrq)
					GPUdisplayText("*PSXjin*: Sio Irq Will Deactivate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: Sio Irq Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.Sio ^= 0x1;
			if (Config.Sio)
				GPUdisplayText(_("*PSXjin*: Sio Irq Always Enabled"));
			else
				GPUdisplayText(_("*PSXjin*: Sio Irq Not Always Enabled"));
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
					GPUdisplayText("*PSXjin*: PE2 Fix Will Activate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: PE2 Fix Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.RCntFix)
					GPUdisplayText("*PSXjin*: PE2 Fix Will Deactivate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: PE2 Fix Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.RCntFix ^= 0x1;
			if (Config.RCntFix)
				GPUdisplayText(_("*PSXjin*: Parasite Eve 2 Fix Enabled"));
			else
				GPUdisplayText(_("*PSXjin*: Parasite Eve 2 Fix Disabled"));
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
					GPUdisplayText("*PSXjin*: RE 2/3 Fix Will Activate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: RE 2/3 Fix Won't Activate On Next Frame");
			}
			else {
				if (MovieControl.VSyncWA)
					GPUdisplayText("*PSXjin*: RE 2/3 Fix Will Deactivate On Next Frame");
				else
					GPUdisplayText("*PSXjin*: RE 2/3 Fix Won't Deactivate On Next Frame");
			}
		}
		else {
			Config.VSyncWA ^= 0x1;
			if (Config.VSyncWA)
				GPUdisplayText(_("*PSXjin*: Resident Evil 2/3 Fix Enabled"));
			else
				GPUdisplayText(_("*PSXjin*: Resident Evil 2/3 Fix Disabled"));
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
		PSXjin_LuaStop();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_LUA_RELOAD].key
	&& modifiers == EmuCommandTable[EMUCMD_LUA_RELOAD].keymod)
	{
		PSXjin_ReloadLuaCode();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_VOLUMEUP].key
	&& modifiers == EmuCommandTable[EMUCMD_VOLUMEUP].keymod)
	{
		if (iVolume < 5)
			iVolume++;
		else if (iVolume > 5)
			iVolume = 5;		//Meh, just in case
		sprintf(Text, "Sound volume: %d", 5 - iVolume);
		GPUdisplayText(_(Text));
		return;
	}

	if(key == EmuCommandTable[EMUCMD_VOLUMEDOWN].key
	&& modifiers == EmuCommandTable[EMUCMD_VOLUMEDOWN].keymod)
	{
		if (iVolume > 0)
			iVolume--;
		if (iVolume < 0)
			iVolume = 0;		//Meh, just in case
		sprintf(Text, "Sound volume: %d", 5 - iVolume);
		GPUdisplayText(_(Text));
		return;
	}

	if(key == EmuCommandTable[EMUCMD_ANALOG].key
	&& modifiers == EmuCommandTable[EMUCMD_ANALOG].keymod)
	{
		OpenAnalogControl();
	}

	if(key == EmuCommandTable[EMUCMD_RESET].key
	&& modifiers == EmuCommandTable[EMUCMD_RESET].keymod)
	{
		ResetGame();
		return;
	}

	if(key == EmuCommandTable[EMUCMD_CDCASE].key
	&& modifiers == EmuCommandTable[EMUCMD_CDCASE].keymod)
	{
		CDOpenClose();
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
	
	if(key == EmuCommandTable[EMUCMD_MTRACK].key
	&& modifiers == EmuCommandTable[EMUCMD_MTRACK].keymod)
	{
		if (!IsMovieLoaded())
		{
			Movie.MultiTrack = !Movie.MultiTrack;
			if (Movie.MultiTrack)
			{
				Movie.RecordPlayer = Movie.NumPlayers+1;
				GPUdisplayText("*PSXjin*: MultiTrack Enabled");
			}
			else
				GPUdisplayText("*PSXjin*: MultiTrack Disabled");
		}
		else GPUdisplayText("*PSXjin*: Movie must be loaded to enable multitrack");
		return;
	}
	if(key == EmuCommandTable[EMUCMD_INCPLAYER].key
	&& modifiers == EmuCommandTable[EMUCMD_INCPLAYER].keymod)
	{
		Movie.RecordPlayer += 1;
		Movie.RecordPlayer = (Movie.RecordPlayer > Movie.NumPlayers+2)? (1):(Movie.RecordPlayer);
		return;
	}
	if(key == EmuCommandTable[EMUCMD_DECPLAYER].key
	&& modifiers == EmuCommandTable[EMUCMD_DECPLAYER].keymod)
	{
		Movie.RecordPlayer -= 1;
		Movie.RecordPlayer = (Movie.RecordPlayer == 0)? (Movie.NumPlayers+2):(Movie.RecordPlayer);
		return;
	}
	if(key == EmuCommandTable[EMUCMD_SELECTALL].key
	&& modifiers == EmuCommandTable[EMUCMD_SELECTALL].keymod)
	{
		Movie.RecordPlayer = Movie.NumPlayers+2;		
		return;
	}
	if(key == EmuCommandTable[EMUCMD_SELECTNONE].key
	&& modifiers == EmuCommandTable[EMUCMD_SELECTNONE].keymod)
	{
		Movie.RecordPlayer = Movie.NumPlayers+1;		
		return;
	}

	if(key == EmuCommandTable[EMUCMD_AUTOHOLD].key
	&& modifiers == EmuCommandTable[EMUCMD_AUTOHOLD].keymod)
	{
		Config.GetAutoHold = true;
		return;
	}
	/*if(key == EmuCommandTable[EMUCMD_AUTOFIRE].key
	&& modifiers == EmuCommandTable[EMUCMD_AUTOFIRE].keymod)
	{
		Config.GetAutoFire = true;
		return;
	}*/
	if(key == EmuCommandTable[EMUCMD_AUTOHOLDCLEAR].key
	&& modifiers == EmuCommandTable[EMUCMD_AUTOHOLDCLEAR].keymod)
	{
		Config.Pad1AutoHold = 0;
		Config.Pad2AutoHold = 0;
		Config.EnableAutoHold = false;
		Config.EnableAutoFire = false;
		Config.Pad1AutoFire = 0;
		Config.Pad2AutoFire = 0;
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

long CALLBACK GPUopen(HWND hwndGPU);
bool OpenPlugins(HWND hWnd) {
	int ret;

	//GPUclearDynarec(clearDynarec);

	if(strcmp(IsoFile,""))
	{
		ret = CDRopen(IsoFile);
		if (ret == 2) return false;	//adelikat: using 2 to mean "Do nothing" for when the user cancels the open file dialog
		if (ret < 0) {/* SysMessage (_("Error Opening CDR Plugin"));*/ return false; }
	}

	SetCurrentDirectory(PSXjinDir);
	
	ret = GPUopen(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening GPU Plugin (%d)"), ret); return false; }
	ret = SPUopen(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening SPU Plugin (%d)"), ret); return false; }
	ret = PAD1_open(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening PAD1 Plugin (%d)"), ret); return false; }
	ret = PAD2_open(hWnd);
	if (ret < 0) { SysMessage (_("Error Opening PAD2 Plugin (%d)"), ret); return false; }

	SetCurrentDirectory(PSXjinDir);
//	ShowCursor(FALSE);
	GPUsendFpLuaGui(PSXjin_LuaGui);
	return true;
}

void ClosePlugins() {
	int ret;

	UpdateMenuSlots();
	ret = CDRclose();
	if (ret < 0) { SysMessage (_("Error Closing CDR Plugin")); return; }
	ret = GPUclose();
	if (ret < 0) { SysMessage (_("Error Closing GPU Plugin")); return; }
	ret = PAD1_close();
	if (ret < 0) { SysMessage (_("Error Closing PAD1 Plugin")); return; }
	ret = PAD2_close();
	if (ret < 0) { SysMessage (_("Error Closing PAD2 Plugin")); return; }
}

void ResetPlugins() {
	int ret;

	CDRshutdown();
	GPUshutdown();
	SPUshutdown();
	PAD1_shutdown();
	PAD2_shutdown();

	ret = CDRinit();
	if (ret != 0) { SysMessage (_("CDRinit error: %d"), ret); return; }
	ret = GPUinit();
	if (ret != 0) { SysMessage (_("GPUinit error: %d"), ret); return; }
	ret = SPUinit();
	if (ret != 0) { SysMessage (_("SPUinit error: %d"), ret); return; }
	ret = PAD1_init(1);
	if (ret != 0) { SysMessage (_("PAD1init error: %d"), ret); return; }
	ret = PAD2_init(2);
	if (ret != 0) { SysMessage (_("PAD2init error: %d"), ret); return; }
}

void SetEmulationSpeed(int cmd) {
	if(cmd == EMUSPEED_TURBO)
		GPUsetspeedmode(0);
	else if(cmd == EMUSPEED_MAXIMUM)
		GPUsetspeedmode(13);
	else if(cmd == EMUSPEED_NORMAL)
		GPUsetspeedmode(7);
	else if(cmd == EMUSPEED_FASTEST)
		GPUsetspeedmode(12);
	else if(cmd == EMUSPEED_SLOWEST)
		GPUsetspeedmode(1);
	else if(cmd == EMUSPEED_SLOWER) {
		if (iSpeedMode>1) iSpeedMode--;
		GPUsetspeedmode(iSpeedMode);
	}
	else if(cmd == EMUSPEED_FASTER) {
		if (iSpeedMode<12) iSpeedMode++;
		GPUsetspeedmode(iSpeedMode);
	}
}
