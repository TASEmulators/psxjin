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

#include <string.h>

#include "PsxCommon.h"
#include "Win32/Win32.h"
#include "Win32/ram_search.h"
#include "Win32/ramwatch.h"


// global variables
psxCounter psxCounters[5];
unsigned long psxNextCounter, psxNextsCounter;

static int cnts = 4;

static void psxRcntUpd(unsigned long index) {
	psxCounters[index].sCycle = psxRegs.cycle;
	if (((!(psxCounters[index].mode & 1)) || (index!=2)) &&
		psxCounters[index].mode & 0x30) {
		if (psxCounters[index].mode & 0x10) { // Interrupt on target
			psxCounters[index].Cycle = ((psxCounters[index].target - psxCounters[index].count) * psxCounters[index].rate) / BIAS;
		} else { // Interrupt on 0xffff
			psxCounters[index].Cycle = ((0xffff - psxCounters[index].count) * psxCounters[index].rate) / BIAS;
		}
	} else psxCounters[index].Cycle = 0xffffffff;
//	if (index == 2) SysPrintf("Cycle %x\n", psxCounters[index].Cycle);
}

static void psxRcntReset(unsigned long index) {
//	SysPrintf("psxRcntReset %x (mode=%x)\n", index, psxCounters[index].mode);
	psxCounters[index].count = 0;
	psxRcntUpd(index);

//	if (index == 2) SysPrintf("rcnt2 %x\n", psxCounters[index].mode);
	psxHu32ref(0x1070)|= SWAPu32(psxCounters[index].interrupt);
	if (!(psxCounters[index].mode & 0x40)) { // Only 1 interrupt
		psxCounters[index].Cycle = 0xffffffff;
	} // else Continuos interrupt mode
}

static void psxRcntSet() {
	int i;

	psxNextCounter = 0x7fffffff;
	psxNextsCounter = psxRegs.cycle;

	for (i=0; i<cnts; i++) {
		s32 count;

		if (psxCounters[i].Cycle == 0xffffffff) continue;

		count = psxCounters[i].Cycle - (psxRegs.cycle - psxCounters[i].sCycle);

		if (count < 0) {
			psxNextCounter = 0; break;
		}

		if (count < (s32)psxNextCounter) {
			psxNextCounter = count;
		}
	}
}

void psxRcntInit() {

	memset(psxCounters, 0, sizeof(psxCounters));

	psxCounters[0].rate = 1; psxCounters[0].interrupt = 0x10;
	psxCounters[1].rate = 1; psxCounters[1].interrupt = 0x20;
	psxCounters[2].rate = 1; psxCounters[2].interrupt = 0x40;

	psxCounters[3].interrupt = 1;
	psxCounters[3].mode = 0x58; // The VSync counter mode
	psxCounters[3].target = 1;
	psxUpdateVSyncRate();

	//if (SPUasync != NULL) {
		cnts = 5;

		psxCounters[4].rate = 768 * 1;
		psxCounters[4].target = 1;
		psxCounters[4].mode = 0x58;
	//}
	//else 
	//	cnts = 4;

	psxRcntUpd(0); psxRcntUpd(1); psxRcntUpd(2); psxRcntUpd(3);
	psxRcntSet();
}

void psxUpdateVSyncRate() {
	if (Config.PsxType) // ntsc - 0 | pal - 1
	     psxCounters[3].rate = (PSXCLK / 50);// / BIAS;
	else psxCounters[3].rate = (PSXCLK / 60);// / BIAS;
	psxCounters[3].rate-= (psxCounters[3].rate / 262) * 22;
	if (Config.VSyncWA) psxCounters[3].rate/= 2;
}

void psxUpdateVSyncRateEnd() {
	if (Config.PsxType) // ntsc - 0 | pal - 1
	     psxCounters[3].rate = (PSXCLK / 50);// / BIAS;
	else psxCounters[3].rate = (PSXCLK / 60);// / BIAS;
	psxCounters[3].rate = (psxCounters[3].rate / 262) * 22;
	if (Config.VSyncWA) psxCounters[3].rate/= 2;
}

extern int iVSyncFlag;      //has a VSync already occured? (we can only save just after a VSync+iGpuHasUpdated)
void psxRcntUpdate() {


	if ((psxRegs.cycle - psxCounters[3].sCycle) >= psxCounters[3].Cycle) {
		if (psxCounters[3].mode & 0x10000) { // VSync End (22 hsyncs)
				psxCounters[3].mode&=~0x10000;
				psxUpdateVSyncRate();
				psxRcntUpd(3);
				GPUupdateLace(); // updateGPU
				//SysUpdate();
	#ifdef GTE_LOG
				GTE_LOG("VSync\n");
	#endif

					/* movie stuff start */
// raise VSync flag
iVSyncFlag = 1;

	PadDataS paddtemp;
	unsigned long buttonToSend;
	char modeFlags;
	Config.AutoFireFrame = !Config.AutoFireFrame;
	Movie.currentFrame++;
	//printf("frame: %d\n",Movie.currentFrame);
	if (Movie.currentFrame > Movie.MaxRecFrames) Movie.MaxRecFrames = Movie.currentFrame;

	// update OSD information
	GPUsetframecounter(Movie.currentFrame,Movie.totalFrames);

	// handle movie end while in replay mode
	if (Movie.mode == MOVIEMODE_PLAY) 
	{
		// pause at last movie frame
		if (Movie.currentFrame==Movie.totalFrames && Config.PauseAfterPlayback)
			iPause = 1;
		// stop if we're beyond last frame
		if (Movie.currentFrame>Movie.totalFrames)
		{
			GPUdisplayText("*PSXjin*: Movie End");
			MOV_StopMovie();
		}
	}


	// write/read joypad information for this frame
	if (iJoysToPoll == 2) //if no joypad has been poll for this frame
	{
		if (!Movie.Port1_Mtap)
		{
			if (Movie.mode == MOVIEMODE_RECORD)
			{		
				MOV_WriteJoy(&Movie.lastPads1[0],Movie.padType1);		
			}
			else if (Movie.mode == MOVIEMODE_PLAY)
			{
				MOV_ReadJoy(&paddtemp,Movie.padType1);		
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				if (Movie.mode == MOVIEMODE_RECORD)
				{		
					MOV_WriteJoy(&Movie.lastPads1[i],Movie.padType1);		
				}
				else if (Movie.mode == MOVIEMODE_PLAY)
				{
					MOV_ReadJoy(&paddtemp,Movie.padType1);		
				}	
			}
		}
		if (!Movie.Port2_Mtap)
		{
			if (Movie.mode == MOVIEMODE_RECORD)
			{		
				MOV_WriteJoy(&Movie.lastPads2[0],Movie.padType2);		
			}
			else if (Movie.mode == MOVIEMODE_PLAY)
			{
				MOV_ReadJoy(&paddtemp,Movie.padType2);		
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				if (Movie.mode == MOVIEMODE_RECORD)
				{		
					MOV_WriteJoy(&Movie.lastPads2[i],Movie.padType2);		
				}
				else if (Movie.mode == MOVIEMODE_PLAY)
				{
					MOV_ReadJoy(&paddtemp,Movie.padType2);		
				}	
			}
		}
		Movie.lagCounter++;
		GPUsetlagcounter(Movie.lagCounter);
	}
	else if (iJoysToPoll == 1)
	{ //this should never happen, but one can never be sure (only 1 pad has been polled for this frame)
		printf("CATASTROPHIC FAILURE: CONTACT DARKKOBOLD");
		if (Movie.mode == MOVIEMODE_RECORD)		
			MOV_WriteJoy(&Movie.lastPads2[0],Movie.padType2);
		else if (Movie.mode == MOVIEMODE_PLAY)
			MOV_ReadJoy(&paddtemp,Movie.padType2);
	}

	// write/read control byte for this frame
	if (Movie.mode == MOVIEMODE_RECORD)	
		MOV_WriteControl();
	else if (Movie.mode == MOVIEMODE_PLAY)
		MOV_ReadControl();

	MOV_ProcessControlFlags();

	// write file once in a while to prevent data loss
	if ((Movie.mode == MOVIEMODE_RECORD) && (Movie.currentFrame%1800 == 0))
		MOV_WriteMovieFile();

	buttonToSend = 0;
	buttonToSend = Movie.lastPads1[0].buttonStatus;
	buttonToSend = (buttonToSend ^ (Movie.lastPads2[0].buttonStatus << 16));
	GPUinputdisplay(buttonToSend);

	modeFlags = 0;
	if (iPause)
		modeFlags |= MODE_FLAG_PAUSED;
	if (Movie.mode == MOVIEMODE_RECORD)
		modeFlags |= MODE_FLAG_RECORD;
	if (Movie.mode == MOVIEMODE_PLAY)
		modeFlags |= MODE_FLAG_REPLAY;
	GPUsetcurrentmode(modeFlags);

	//Update tools
	PSXjinApplyCheats();
	UpdateToolWindows();

	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);

					/* movie stuff end */
		} else { // VSync Start (240 hsyncs) 
			CallRegisteredLuaFunctions(LUACALL_BEFOREEMULATION);
			psxCounters[3].mode|= 0x10000;
			psxUpdateVSyncRateEnd();
			psxRcntUpd(3);
			psxHu32ref(0x1070)|= SWAPu32(1);
		}
	}

	if ((psxRegs.cycle - psxCounters[0].sCycle) >= psxCounters[0].Cycle) {
		psxRcntReset(0);
	}

	if ((psxRegs.cycle - psxCounters[1].sCycle) >= psxCounters[1].Cycle) {
		psxRcntReset(1);
	}

	if ((psxRegs.cycle - psxCounters[2].sCycle) >= psxCounters[2].Cycle) {
		psxRcntReset(2);
	}

	if (cnts >= 5) {
		if ((psxRegs.cycle - psxCounters[4].sCycle) >= psxCounters[4].Cycle) {
			SPUasync((psxRegs.cycle - psxCounters[4].sCycle));
			psxRcntReset(4);
		}
	}

	psxRcntSet();
}

void psxRcntWcount(unsigned long index, unsigned long value) {
//	SysPrintf("writeCcount[%d] = %x\n", index, value);
//	PSXCPU_LOG("writeCcount[%d] = %x\n", index, value);
	psxCounters[index].count = value;
	psxRcntUpd(index);
	psxRcntSet();
}

void psxRcntWmode(unsigned long index, unsigned long value)  {
//	SysPrintf("writeCmode[%ld] = %lx\n", index, value);
	psxCounters[index].mode = value;
	psxCounters[index].count = 0;
	if(index == 0) {
		switch (value & 0x300) {
			case 0x100:
				psxCounters[index].rate = ((psxCounters[3].rate /** BIAS*/) / 386) / 262; // seems ok
				break;
			default:
				psxCounters[index].rate = 1;
		}
	}
	else if(index == 1) {
		switch (value & 0x300) {
			case 0x100:
				psxCounters[index].rate = (psxCounters[3].rate /** BIAS*/) / 262; // seems ok
				break;
			default:
				psxCounters[index].rate = 1;
		}
	}
	else if(index == 2) {
		switch (value & 0x300) {
			case 0x200:
				psxCounters[index].rate = 8; // 1/8 speed
				break;
			default:
				psxCounters[index].rate = 1; // normal speed
		}
	}

	// Need to set a rate and target
	psxRcntUpd(index);
	psxRcntSet();
}

void psxRcntWtarget(unsigned long index, unsigned long value) {
//	SysPrintf("writeCtarget[%ld] = %lx\n", index, value);
	psxCounters[index].target = value;
	psxRcntUpd(index);
	psxRcntSet();
}

unsigned long psxRcntRcount(unsigned long index) {
	u32 ret;

//	if ((!(psxCounters[index].mode & 1)) || (index!=2)) {
		if (psxCounters[index].mode & 0x08) { // Wrap at target
			if (Config.RCntFix) { // Parasite Eve 2
				ret = (psxCounters[index].count + /*BIAS **/ ((psxRegs.cycle - psxCounters[index].sCycle) / psxCounters[index].rate)) & 0xffff;
			} else {
				ret = (psxCounters[index].count + BIAS * ((psxRegs.cycle - psxCounters[index].sCycle) / psxCounters[index].rate)) & 0xffff;
			}
		} else { // Wrap at 0xffff
			ret = (psxCounters[index].count + BIAS * (psxRegs.cycle / psxCounters[index].rate)) & 0xffff;
			if (Config.RCntFix) { // Vandal Hearts 1/2
				ret/= 16;
			}
		}
//		return (psxCounters[index].count + BIAS * ((psxRegs.cycle - psxCounters[index].sCycle) / psxCounters[index].rate)) & 0xffff;
//	} else return 0;

//	SysPrintf("readCcount[%ld] = %lx (mode %lx, target %lx, cycle %lx)\n", index, ret, psxCounters[index].mode, psxCounters[index].target, psxRegs.cycle);

	return ret;
}

int psxRcntFreeze(EMUFILE *f, int Mode) {

	gzfreezelarr(psxCounters);
	gzfreezel(&psxNextCounter);
	gzfreezel(&psxNextsCounter);
	gzfreezel(&cnts);

	return 0;
}
