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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PsxCommon.h"

#ifdef _MSC_VER_
#pragma warning(disable:4244)
#endif

#define CheckErr(func) \
    err = SysLibError(); \
    if (err != NULL) { SysMessage(_("Error loading %s: %s"), func, err); return -1; }

#if defined (__MACOSX__)
#define LoadSym(dest, src, name, checkerr) \
    dest = (src) SysLoadSym(drv, "_" name); \
    if(!checkerr) { SysLibError(); /*clean error*/ } \
    if (checkerr == 1) CheckErr(name); \
    if (checkerr == 2) { err = SysLibError(); if (err != NULL) errval = 1; }
#else
#define LoadSym(dest, src, name, checkerr) \
    dest = (src) SysLoadSym(drv, name); if (checkerr == 1) CheckErr(name); \
    if (checkerr == 2) { err = SysLibError(); if (err != NULL) errval = 1; }
#endif

START_EXTERN_C
//
////Gpu function pointers
//GPUupdateLace       GPU_updateLace;
//GPUinit             GPU_init;
//GPUshutdown         GPU_shutdown; 
//GPUconfigure        GPU_configure;
//GPUtest             GPU_test;
//GPUabout            GPU_about;
//GPUopen             GPU_open;
//GPUclose            GPU_close;
//GPUreadStatus       GPU_readStatus;
//GPUreadData         GPU_readData;
//GPUreadDataMem      GPU_readDataMem;
//GPUwriteStatus      GPU_writeStatus; 
//GPUwriteData        GPU_writeData;
//GPUwriteDataMem     GPU_writeDataMem;
//GPUdmaChain         GPU_dmaChain;
//GPUkeypressed       GPU_keypressed;
//GPUdisplayText      GPUdisplayText;
//GPUmakeSnapshot     GPU_makeSnapshot;
//GPUfreeze           GPU_freeze;
//GPUgetScreenPic     GPU_getScreenPic;
//GPUshowScreenPic    GPU_showScreenPic;
//GPUclearDynarec     GPU_clearDynarec;
//GPUsetframelimit    GPU_setframelimit;
//GPUsetframecounter  GPU_setframecounter;
//GPUsetlagcounter    GPU_setlagcounter;
//GPUinputdisplay     GPU_inputdisplay;
//GPUupdateframe      GPU_updateframe;
//GPUsetcurrentmode   GPUsetcurrentmode;
//GPUsetspeedmode     GPU_setspeedmode;
//GPUshowframecounter GPU_showframecounter;
//GPUshowInput		GPU_showInput;
//GPUstartAvi         GPU_startAvi;
//GPUstopAvi          GPU_stopAvi;
//GPUsendFpLuaGui     GPU_sendFpLuaGui;

//cd rom function pointers 
//CDRinit               CDR_init;
//CDRshutdown           CDR_shutdown;
//CDRopen               CDR_open;
//CDRclose              CDR_close; 
//CDRtest               CDR_test;
//CDRgetTN              CDR_getTN;
//CDRgetTD              CDR_getTD;
//CDRreadTrack          CDR_readTrack;
//CDRgetBuffer          CDR_getBuffer;
//CDRplay               CDR_play;
//CDRstop               CDR_stop;
//CDRgetStatus          CDR_getStatus;
//CDRgetDriveLetter     CDR_getDriveLetter;
//CDRgetBufferSub       CDR_getBufferSub;
//CDRconfigure          CDR_configure;
//CDRabout              CDR_about;

//PAD POINTERS
PADconfigure        PAD1_configure;
PADabout            PAD1_about;
PADinit             PAD1_init;
PADshutdown         PAD1_shutdown;
PADtest             PAD1_test;
PADopen             PAD1_open;
PADclose            PAD1_close;
PADquery            PAD1_query;
PADreadPort1        PAD1_readPort1;
PADkeypressed       PAD1_keypressed;
PADstartPoll        PAD1_startPoll;
PADpoll             PAD1_poll;
PADsetSensitive     PAD1_setSensitive;
PADconfigure        PAD2_configure;
PADabout            PAD2_about;
PADinit             PAD2_init;
PADshutdown         PAD2_shutdown;
PADtest             PAD2_test;
PADopen             PAD2_open;
PADclose            PAD2_close;
PADquery            PAD2_query;
PADreadPort2        PAD2_readPort2;
PADkeypressed       PAD2_keypressed;
PADstartPoll        PAD2_startPoll;
PADpoll             PAD2_poll;
PADsetSensitive     PAD2_setSensitive;


END_EXTERN_C

static const char *err;
static int errval;
PadDataS PaddInput;
void ConfigurePlugins();

void CALLBACK GPU__readDataMem(unsigned long *pMem, int iSize) {
	while (iSize > 0) {
		*pMem = GPUreadData();
		iSize--;
		pMem++;
	}		
}

void CALLBACK GPU__writeDataMem(unsigned long *pMem, int iSize) {
	while (iSize > 0) {
		GPUwriteData(*pMem);
		iSize--;
		pMem++;
	}
}

void CALLBACK GPU__displayText(char *pText) {
	SysPrintf("%s\n", pText);
}

long CALLBACK GPU__freeze(unsigned long ulGetFreezeData, GPUFreeze_t *pF) {
	pF->ulFreezeVersion = 1;
	if (ulGetFreezeData == 0) {
		int val;

		val = GPUreadStatus();
		val = 0x04000000 | ((val >> 29) & 0x3);
		GPUwriteStatus(0x04000003);
		GPUwriteStatus(0x01000000);
		GPUwriteData(0xa0000000);
		GPUwriteData(0x00000000);
		GPUwriteData(0x02000400);
		GPUwriteDataMem((u32*)pF->psxVRam, 0x100000/4);
		GPUwriteStatus(val);

		val = pF->ulStatus;
		GPUwriteStatus(0x00000000);
		GPUwriteData(0x01000000);
		GPUwriteStatus(0x01000000);
		GPUwriteStatus(0x03000000 | ((val>>24)&0x1));
		GPUwriteStatus(0x04000000 | ((val>>29)&0x3));
		GPUwriteStatus(0x08000000 | ((val>>17)&0x3f) | ((val>>10)&0x40));
		GPUwriteData(0xe1000000 | (val&0x7ff));
		GPUwriteData(0xe6000000 | ((val>>11)&3));

/*		GPU_writeData(0xe3000000 | pF->ulControl[0] & 0xffffff);
		GPU_writeData(0xe4000000 | pF->ulControl[1] & 0xffffff);
		GPU_writeData(0xe5000000 | pF->ulControl[2] & 0xffffff);*/
		return 1;
	}
	if (ulGetFreezeData == 1) {
		int val;

		val = GPUreadStatus();
		val = 0x04000000 | ((val >> 29) & 0x3);
		GPUwriteStatus(0x04000003);
		GPUwriteStatus(0x01000000);
		GPUwriteData(0xc0000000);
		GPUwriteData(0x00000000);
		GPUwriteData(0x02000400);
		GPUreadDataMem((u32*)pF->psxVRam, 0x100000/4);
		GPUwriteStatus(val);

		pF->ulStatus = GPUreadStatus();

/*		GPU_writeStatus(0x10000003);
		pF->ulControl[0] = GPU_readData();
		GPU_writeStatus(0x10000004);
		pF->ulControl[1] = GPU_readData();
		GPU_writeStatus(0x10000005);
		pF->ulControl[2] = GPU_readData();*/
		return 1;
	}
	if(ulGetFreezeData==2) {
		long lSlotNum=*((long *)pF);
		char Text[32];

		sprintf (Text, "*PSXjin*: Selected State %ld", lSlotNum+1);
		GPUdisplayText(Text);
		return 1;
	}
	return 0;
}

long CALLBACK GPU__configure(void) { return 0; }
long CALLBACK GPU__test(void) { return 0; }
void CALLBACK GPU__about(void) {}
void CALLBACK GPU__makeSnapshot(void) {}
void CALLBACK GPU__keypressed(int key) {}
long CALLBACK GPU__getScreenPic(unsigned char *pMem) { return -1; }
long CALLBACK GPU__showScreenPic(unsigned char *pMem) { return -1; }
void CALLBACK GPU__clearDynarec(void (CALLBACK *callback)(void)) { }

void CALLBACK GPU__setframelimit(unsigned long option) {}
void CALLBACK GPU__setframecounter(unsigned long newCurrentFrame,unsigned long newTotalFrames) {}
void CALLBACK GPU__setlagcounter(unsigned long newCurrentLag) {}
void CALLBACK GPU__inputdisplay(unsigned long newCurrentInput) {}
void CALLBACK GPU__updateframe(void) {}
void CALLBACK GPU__setcurrentmode(char newModeFlags) {}
void CALLBACK GPU__setspeedmode(unsigned long newSpeedMode) {}
void CALLBACK GPU__showframecounter(void) {}
void CALLBACK GPU__showInput(void) {}
void CALLBACK GPU__startAvi(char* filename) {}
void CALLBACK GPU__stopAvi(void) {}
void CALLBACK GPU__sendFpLuaGui(void (*fpPSXjin_LuaGui)(void *,int,int,int,int)) {}

#define LoadGpuSym1(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, 1);

#define LoadGpuSym0(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, 0); \
	if (GPU_##dest == NULL) GPU_##dest = (GPU##dest) GPU__##dest;

#define LoadGpuSymN(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, 0);


void *hCDRDriver;

long CDR__play(unsigned char *sector) { return 0; }
long CDR__stop(void) { return 0; }

long CDRgetStatus(struct CdrStat *stat) {
//long CDR__getStatus(struct CdrStat *stat) {
	if (cdOpenCase < 0)
		stat->Status = 0x10;
	else
		stat->Status = 0;

	return 0;
}

unsigned long CDR_fakeStatus() {
	if (cdOpenCase < 0)
		return 0x10;
	else
		return 0;
}

char* CDR__getDriveLetter(void) { return NULL; }
//unsigned char* CDR__getBufferSub(void) { return NULL; }
unsigned char* CDRgetBufferSub(void) { return NULL; }
long CDR__configure(void) { return 0; }
long CDR__test(void) { return 0; }
void CDR__about(void) {}

#define LoadCdrSym1(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, 1);

#define LoadCdrSym0(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, 0); \
	if (CDR_##dest == NULL) CDR_##dest = (CDR##dest) CDR__##dest;

#define LoadCdrSymN(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, 0);

//int LoadCDRplugin(char *CDRdll) {
//	void *drv;
//
//	hCDRDriver = SysLoadLibrary(CDRdll);
//	if (hCDRDriver == NULL) {
//		CDR_configure = NULL;
//		SysMessage (_("Could Not load CDR plugin %s"), CDRdll);  return -1;
//	}
//	drv = hCDRDriver;
//	LoadCdrSym1(init, "CDRinit");
//	LoadCdrSym1(shutdown, "CDRshutdown");
//	LoadCdrSym1(open, "CDRopen");
//	LoadCdrSym1(close, "CDRclose");
//	LoadCdrSym1(getTN, "CDRgetTN");
//	LoadCdrSym1(getTD, "CDRgetTD");
//	LoadCdrSym1(readTrack, "CDRreadTrack");
//	LoadCdrSym1(getBuffer, "CDRgetBuffer");
//	LoadCdrSym0(play, "CDRplay");
//	LoadCdrSym0(stop, "CDRstop");
//	LoadCdrSym0(getStatus, "CDRgetStatus");
//	LoadCdrSym0(getDriveLetter, "CDRgetDriveLetter");
//	LoadCdrSym0(getBufferSub, "CDRgetBufferSub");
//	LoadCdrSym0(configure, "CDRconfigure");
//	LoadCdrSym0(test, "CDRtest");
//	LoadCdrSym0(about, "CDRabout");
//
//	return 0;
//}

//int LoadSPUplugin(char *SPUdll) {
//	void *drv;
//
//	hSPUDriver = SysLoadLibrary(SPUdll);
//	if (hSPUDriver == NULL) {
//		SPU_configure = NULL;
//		SysMessage (_("Could not open SPU plugin %s"), SPUdll); return -1;
//	}
//	drv = hSPUDriver;
//	LoadSpuSym1(init, "SPUinit");
//	LoadSpuSym1(shutdown, "SPUshutdown");
//	LoadSpuSym1(open, "SPUopen");
//	LoadSpuSym1(close, "SPUclose");
//	LoadSpuSym0(configure, "SPUconfigure");
//	LoadSpuSym0(about, "SPUabout");
//	LoadSpuSym0(test, "SPUtest");
//	errval = 0;
//	LoadSpuSym2(startChannels1, "SPUstartChannels1");
//	LoadSpuSym2(startChannels2, "SPUstartChannels2");
//	LoadSpuSym2(stopChannels1, "SPUstopChannels1");
//	LoadSpuSym2(stopChannels2, "SPUstopChannels2");
//	LoadSpuSym2(putOne, "SPUputOne");
//	LoadSpuSym2(getOne, "SPUgetOne");
//	LoadSpuSym2(setAddr, "SPUsetAddr");
//	LoadSpuSym2(setPitch, "SPUsetPitch");
//	LoadSpuSym2(setVolumeL, "SPUsetVolumeL");
//	LoadSpuSym2(setVolumeR, "SPUsetVolumeR");
//	LoadSpuSymE(writeRegister, "SPUwriteRegister");
//	LoadSpuSymE(readRegister, "SPUreadRegister");		
//	LoadSpuSymE(writeDMA, "SPUwriteDMA");
//	LoadSpuSymE(readDMA, "SPUreadDMA");
//	LoadSpuSym0(writeDMAMem, "SPUwriteDMAMem");
//	LoadSpuSym0(readDMAMem, "SPUreadDMAMem");
//	LoadSpuSym0(playADPCMchannel, "SPUplayADPCMchannel");
//	LoadSpuSym0(freeze, "SPUfreeze");
//	LoadSpuSym0(registerCallback, "SPUregisterCallback");
//	LoadSpuSymN(async, "SPUasync");
//	LoadSpuSymN(startWav, "SPUstartWav");
//	LoadSpuSymN(stopWav, "SPUstopWav");
//
//	return 0;
//}


void *hPAD1Driver;
void *hPAD2Driver;

static unsigned char buf[256];
unsigned char stdpar[10] = { 0x00, 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mousepar[8] = { 0x00, 0x12, 0x5a, 0xff, 0xff, 0xff, 0xff };
unsigned char analogpar[9] = { 0x00, 0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mtappar[34] = { 0x00, 0x80, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
							  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
							  0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static int bufcount, bufc;

PadDataS padd1, padd2;

unsigned char _PADstartPoll(PadDataS *pad) {
	bufc = 0;

	switch (pad->controllerType) {
		case PSE_PAD_TYPE_MOUSE:
			mousepar[3] = pad->buttonStatus & 0xff;
			mousepar[4] = pad->buttonStatus >> 8;
			mousepar[5] = pad->moveX;
			mousepar[6] = pad->moveY;
			memcpy(buf, mousepar, 7);
			bufcount = 6;
			break;
		case PSE_PAD_TYPE_NEGCON: // npc101/npc104(slph00001/slph00069)
			analogpar[1] = 0x23;
			analogpar[3] = pad->buttonStatus & 0xff;
			analogpar[4] = pad->buttonStatus >> 8;
			analogpar[5] = pad->rightJoyX;
			analogpar[6] = pad->rightJoyY;
			analogpar[7] = pad->leftJoyX;
			analogpar[8] = pad->leftJoyY;

			memcpy(buf, analogpar, 9);
			bufcount = 8;
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			analogpar[1] = 0x73;
			analogpar[3] = pad->buttonStatus & 0xff;
			analogpar[4] = pad->buttonStatus >> 8;
			analogpar[5] = pad->rightJoyX;
			analogpar[6] = pad->rightJoyY;
			analogpar[7] = pad->leftJoyX;
			analogpar[8] = pad->leftJoyY;

			memcpy(buf, analogpar, 9);
			bufcount = 8;
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			analogpar[1] = 0x53;
			analogpar[3] = pad->buttonStatus & 0xff;
			analogpar[4] = pad->buttonStatus >> 8;
			analogpar[5] = pad->rightJoyX;
			analogpar[6] = pad->rightJoyY;
			analogpar[7] = pad->leftJoyX;
			analogpar[8] = pad->leftJoyY;

			memcpy(buf, analogpar, 9);
			bufcount = 8;
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			stdpar[3] = pad->buttonStatus & 0xff;
			stdpar[4] = pad->buttonStatus >> 8;

			memcpy(buf, stdpar, 5);
			bufcount = 4;
	}

	return buf[bufc++];
}

unsigned char _xPADstartPoll(PadDataS *padd) {
	bufc = 0;
	PadDataS pad[4];
	memcpy(&pad,padd,sizeof(pad));	
	int psize = 8;
	for (int i = 0; i < 4; i++)
	{
		mtappar[3+psize*i+1] = 0x5A;
		switch (pad->controllerType) {
			case PSE_PAD_TYPE_MOUSE:
				mtappar[3+psize*i] = 0x12;
				mtappar[3+psize*i+2] = pad[i].buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad[i].buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad[i].moveX;
				mtappar[3+psize*i+5] = pad[i].moveY;								
				break;
			case PSE_PAD_TYPE_NEGCON: // npc101/npc104(slph00001/slph00069)
				mtappar[3+psize*i] = 0x23;
				mtappar[3+psize*i+2] = pad->buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad->buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad->rightJoyX;
				mtappar[3+psize*i+5] = pad->rightJoyY;
				mtappar[3+psize*i+6] = pad->leftJoyX;
				mtappar[3+psize*i+7] = pad->leftJoyY;	
				break;
			case PSE_PAD_TYPE_ANALOGPAD: // scph1150
				mtappar[3+psize*i] = 0x73;
				mtappar[3+psize*i+2] = pad->buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad->buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad->rightJoyX;
				mtappar[3+psize*i+5] = pad->rightJoyY;
				mtappar[3+psize*i+6] = pad->leftJoyX;
				mtappar[3+psize*i+7] = pad->leftJoyY;	
				break;
			case PSE_PAD_TYPE_ANALOGJOY: // scph1110
				mtappar[3+psize*i] = 0x53;
				mtappar[3+psize*i+2] = pad->buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad->buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad->rightJoyX;
				mtappar[3+psize*i+5] = pad->rightJoyY;
				mtappar[3+psize*i+6] = pad->leftJoyX;
				mtappar[3+psize*i+7] = pad->leftJoyY;				
				break;
			case PSE_PAD_TYPE_STANDARD:
			default:
				mtappar[3+psize*i] = 0x41;
				mtappar[3+psize*i+2] = pad->buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad->buttonStatus >> 8;				
		}
	}
	memcpy(buf, mtappar, 34);
	bufcount = 34;
	return buf[bufc++];
}

unsigned char _PADpoll(unsigned char value) {
	if (bufc > bufcount) return 0;
	return buf[bufc++];
}

unsigned char CALLBACK PAD1__startPoll(int pad) {
	PadDataS padd;
	PadDataS Mpadds[4];
	PadDataS epadd; //empty pad;
	epadd.buttonStatus = 0xffff;
	epadd.leftJoyX = 128;
	epadd.leftJoyY = 128;
	epadd.rightJoyX = 128;
	epadd.rightJoyY = 128;
	PAD1_readPort1(&padd);
	memcpy(&PaddInput,&padd,sizeof(padd));
	if(PSXjin_LuaUsingJoypad(0)) padd.buttonStatus = PSXjin_LuaReadJoypad(0)^0xffff;
	LuaAnalogJoy* luaAnalogJoy = PSXjin_LuaReadAnalogJoy(0);
	if (luaAnalogJoy != NULL) {
		padd.leftJoyX = luaAnalogJoy->xleft;
		padd.leftJoyY = luaAnalogJoy->yleft;
		padd.rightJoyX = luaAnalogJoy->xright;
		padd.rightJoyY = luaAnalogJoy->yright;
	}  


/* movie stuff start */
	if (!Movie.Port1_Mtap) {
		if (iJoysToPoll == 2) { // only poll once each frame
			if (Movie.mode == MOVIEMODE_RECORD)
			{
				if (Movie.MultiTrack) 
				{
					if ((Movie.RecordPlayer == 1) || (Movie.RecordPlayer == 4)) 
					{
						MOV_WriteJoy(&padd,Movie.padType1);
					}
					else 
					{
						if (Movie.currentFrame >= Movie.MaxRecFrames)
						{						
							MOV_WriteJoy(&epadd,Movie.padType1);
						} 
						else
						{
							MOV_ReadJoy(&padd,Movie.padType1);
						}
					}
				}
				else
				{
					MOV_WriteJoy(&padd,Movie.padType1);
				}
			}
			else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames)
			{
				MOV_ReadJoy(&padd,Movie.padType1);
			}
			memcpy(&Movie.lastPad1,&padd,sizeof(padd));
			iJoysToPoll--;
			return _PADstartPoll(&padd);
		}
		else
		{
			memcpy(&padd,&Movie.lastPad1,sizeof(Movie.lastPad1));
			return _PADstartPoll(&padd);
		}
	} else {
		if (iJoysToPoll == 2) { // only poll once each frame		
			for (int Player = 0; Player < 4; Player++)		
			{		
				if (Movie.mode == MOVIEMODE_RECORD) {
					if ((Movie.RecordPlayer == Player+1) || (Movie.RecordPlayer == Movie.NumPlayers+2)) 
					{
						MOV_WriteJoy(&padd,Movie.padType1);
						memcpy(&Mpadds[Player],&padd,sizeof(padd));
					}
					else 
					{
						if (Movie.currentFrame >= Movie.MaxRecFrames)
						{							
							MOV_WriteJoy(&epadd,Movie.padType1);
						} 
						else
						{
							MOV_ReadJoy(&Mpadds[Player],Movie.padType1);
						}
					}				
				} else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames) {
					MOV_ReadJoy(&Mpadds[Player],Movie.padType1);
				}
				memcpy(&Movie.lastPads1,&Mpadds[Player],sizeof(Mpadds));
				memcpy(&Movie.lastPad1,&padd,sizeof(padd));
			}
			iJoysToPoll--;
			return _xPADstartPoll(&Mpadds[0]);
		}
		else
		{
			memcpy(&Mpadds,&Movie.lastPads1,sizeof(Mpadds));
			memcpy(&Movie.lastPad2,&padd,sizeof(padd));
			return _xPADstartPoll(&Mpadds[0]);
		}
	}
}


unsigned char CALLBACK PAD1__poll(unsigned char value) {
	return _PADpoll(value);
}

long CALLBACK PAD1__configure(void) { return 0; }
void CALLBACK PAD1__about(void) {}
long CALLBACK PAD1__test(void) { return 0; }
long CALLBACK PAD1__query(void) { return 3; }
long CALLBACK PAD1__keypressed() { return 0; }

#define LoadPad1Sym1(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, 1);

#define LoadPad1SymN(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, 0);

#define LoadPad1Sym0(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, 0); \
	if (PAD1_##dest == NULL) PAD1_##dest = (PAD##dest) PAD1__##dest;

int LoadPAD1plugin(char *PAD1dll) {
	void *drv;

	hPAD1Driver = SysLoadLibrary(PAD1dll);
	if (hPAD1Driver == NULL) {
		PAD1_configure = NULL;
		SysMessage (_("Could Not load PAD1 plugin %s"), PAD1dll); return -1;
	}
	drv = hPAD1Driver;
	LoadPad1Sym1(init, "PADinit");
	LoadPad1Sym1(shutdown, "PADshutdown");
	LoadPad1Sym1(open, "PADopen");
	LoadPad1Sym1(close, "PADclose");
	LoadPad1Sym0(query, "PADquery");
	LoadPad1Sym1(readPort1, "PADreadPort1");
	LoadPad1Sym0(configure, "PADconfigure");
	LoadPad1Sym0(test, "PADtest");
	LoadPad1Sym0(about, "PADabout");
	LoadPad1Sym0(keypressed, "PADkeypressed");
	//LoadPad1Sym0(startPoll, "PADstartPoll");
	//LoadPad1Sym0(poll, "PADpoll");
	PAD1_startPoll = PAD1__startPoll;
	PAD1_poll = PAD1__poll;
	LoadPad1SymN(setSensitive, "PADsetSensitive");

	return 0;
}

unsigned char CALLBACK PAD2__startPoll(int pad) {
	PadDataS padd;
	PadDataS Mpadds[4];
	if (Movie.MultiTrack && ((Movie.RecordPlayer >= Movie.P2_Start) || (Movie.RecordPlayer == Movie.NumPlayers+2)))
	{
		PAD2_readPort2(&padd);
		memcpy(&padd,&PaddInput,sizeof(padd));
	}
	else
	{
		PAD2_readPort2(&padd);
	}
	if(PSXjin_LuaUsingJoypad(1)) padd.buttonStatus = PSXjin_LuaReadJoypad(1)^0xffff;
	LuaAnalogJoy* luaAnalogJoy = PSXjin_LuaReadAnalogJoy(1);
	if (luaAnalogJoy != NULL) {
		padd.leftJoyX = luaAnalogJoy->xleft;
		padd.leftJoyY = luaAnalogJoy->yleft;
		padd.rightJoyX = luaAnalogJoy->xright;
		padd.rightJoyY = luaAnalogJoy->yright;
	}

/* movie stuff start */
if (!Movie.Port2_Mtap) {
	if (iJoysToPoll == 1) { // only poll once each frame
		if (Movie.mode == MOVIEMODE_RECORD)
		{
			if (Movie.MultiTrack) 
			{
				if ((Movie.RecordPlayer == Movie.P2_Start) || (Movie.RecordPlayer == Movie.NumPlayers+2)) 
				{
					MOV_WriteJoy(&padd,Movie.padType2);
				}
				else 
				{
					if (Movie.currentFrame >= Movie.MaxRecFrames)
					{
						padd.buttonStatus = 0xffff;
						padd.leftJoyX = 128;
						padd.leftJoyY = 128;
						padd.rightJoyX = 128;
						padd.rightJoyY = 128;
						MOV_WriteJoy(&padd,Movie.padType2);
					} 
					else
					{
						MOV_ReadJoy(&padd,Movie.padType2);
					}
				}
			}
			else
			{
				MOV_WriteJoy(&padd,Movie.padType2);
			}
		}
		else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames)
			MOV_ReadJoy(&padd,Movie.padType2);
		memcpy(&Movie.lastPad2,&padd,sizeof(padd));
		iJoysToPoll--;
		return _PADstartPoll(&padd);
	}
	else
	{
		memcpy(&padd,&Movie.lastPad2,sizeof(Movie.lastPad2));
		return _PADstartPoll(&padd);
	}
	} else {
		if (iJoysToPoll == 1) { // only poll once each frame		
			for (int Player = 0; Player < 4; Player++)		
			{		
				if (Movie.mode == MOVIEMODE_RECORD) {
					if ((Movie.RecordPlayer == Movie.P2_Start+Player) || (Movie.RecordPlayer == Movie.NumPlayers+2)) 
					{
						MOV_WriteJoy(&Mpadds[Player],Movie.padType2);
					}
					else 
					{
						if (Movie.currentFrame >= Movie.MaxRecFrames)
						{
							padd.buttonStatus = 0xffff;
							padd.leftJoyX = 128;
							padd.leftJoyY = 128;
							padd.rightJoyX = 128;
							padd.rightJoyY = 128;
							MOV_WriteJoy(&Mpadds[Player],Movie.padType2);
						} 
						else
						{
							MOV_ReadJoy(&Mpadds[Player],Movie.padType2);
						}
					}				
				} else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames) {
					MOV_ReadJoy(&Mpadds[Player],Movie.padType2);
				}
				memcpy(&Movie.lastPads2,&Mpadds[Player],sizeof(Mpadds));
				memcpy(&Movie.lastPad2,&padd,sizeof(padd));
			}
			iJoysToPoll--;
			return _xPADstartPoll(&Mpadds[0]);
		}
		else
		{
			memcpy(&Mpadds,&Movie.lastPads2,sizeof(Mpadds));
			memcpy(&Movie.lastPad1,&padd,sizeof(padd));
			return _xPADstartPoll(&Mpadds[0]);
		}
	}
}

unsigned char CALLBACK PAD2__poll(unsigned char value) {
	return _PADpoll(value);
}

long CALLBACK PAD2__configure(void) { return 0; }
void CALLBACK PAD2__about(void) {}
long CALLBACK PAD2__test(void) { return 0; }
long CALLBACK PAD2__query(void) { return 3; }
long CALLBACK PAD2__keypressed() { return 0; }

#define LoadPad2Sym1(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, 1);

#define LoadPad2Sym0(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, 0); \
	if (PAD2_##dest == NULL) PAD2_##dest = (PAD##dest) PAD2__##dest;

#define LoadPad2SymN(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, 0);

int LoadPAD2plugin(char *PAD2dll) {
	void *drv;

	hPAD2Driver = SysLoadLibrary(PAD2dll);
	if (hPAD2Driver == NULL) {
		PAD2_configure = NULL;
		SysMessage (_("Could Not load PAD plugin %s"), PAD2dll); return -1;
	}
	drv = hPAD2Driver;
	LoadPad2Sym1(init, "PADinit");
	LoadPad2Sym1(shutdown, "PADshutdown");
	LoadPad2Sym1(open, "PADopen");
	LoadPad2Sym1(close, "PADclose");
	LoadPad2Sym0(query, "PADquery");
	LoadPad2Sym1(readPort2, "PADreadPort2");
	LoadPad2Sym0(configure, "PADconfigure");
	LoadPad2Sym0(test, "PADtest");
	LoadPad2Sym0(about, "PADabout");
	LoadPad2Sym0(keypressed, "PADkeypressed");
	//LoadPad2Sym0(startPoll, "PADstartPoll");
	//LoadPad2Sym0(poll, "PADpoll");
	PAD2_startPoll = PAD2__startPoll;
	PAD2_poll = PAD2__poll;
	LoadPad2SymN(setSensitive, "PADsetSensitive");

	return 0;
}


void CALLBACK clearDynarec(void) {
	psxCpu->Reset();
}

int LoadPlugins() {
	int ret;
	char Plugin[256];

	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.Pad1);
	if (LoadPAD1plugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.Pad2);
	if (LoadPAD2plugin(Plugin) == -1) return -1;

	ret = CDRinit();
	if (ret < 0) { SysMessage (_("CDRinit error : %d"), ret); return -1; }
	ret = GPUinit();
	if (ret < 0) { SysMessage (_("GPUinit error: %d"), ret); return -1; }
	ret = SPUinit();
	if (ret < 0) { SysMessage (_("SPUinit error: %d"), ret); return -1; }
	ret = PAD1_init(1);
	if (ret < 0) { SysMessage (_("PAD1init error: %d"), ret); return -1; }
	ret = PAD2_init(2);
	if (ret < 0) { SysMessage (_("PAD2init error: %d"), ret); return -1; }

	return 0;
}

void ReleasePlugins() {
	if (hCDRDriver  == NULL ||
		hPAD1Driver == NULL || hPAD2Driver == NULL) return;

	CDRshutdown();
	GPUshutdown();
	SPUshutdown();
	PAD1_shutdown();
	PAD2_shutdown();

	SysCloseLibrary(hPAD1Driver); hPAD1Driver = NULL;
	SysCloseLibrary(hPAD2Driver); hPAD2Driver = NULL;
}
