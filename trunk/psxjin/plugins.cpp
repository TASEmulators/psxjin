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
#include "padwin.h"
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




static unsigned char buf[256];
unsigned char nonepar[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
unsigned char stdpar[10] = { 0x00, 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mousepar[8] = { 0x00, 0x12, 0x5a, 0xff, 0xff, 0xff, 0xff };
unsigned char analogpar[9] = { 0x00, 0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mtappar[34] = { 0x00, 0x80, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
							  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
							  0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static int bufcount, bufc;

PadDataS padd1, padd2;


//Older, simpler version of pad polling - for Standard Pads or MultiTAP
unsigned char _PADstartPoll_old(PadDataS *pad) {
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
		case PSE_PAD_TYPE_NONE:
			memcpy(buf,nonepar,2);
			bufcount = 1;			
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:			
			stdpar[3] = pad->buttonStatus & 0xff;
			stdpar[4] = pad->buttonStatus >> 8;
			memcpy(buf, stdpar, 5);
			bufcount = 4;
			break;
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
				mtappar[3+psize*i+2] = pad[i].buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad[i].buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad[i].rightJoyX;
				mtappar[3+psize*i+5] = pad[i].rightJoyY;
				mtappar[3+psize*i+6] = pad[i].leftJoyX;
				mtappar[3+psize*i+7] = pad[i].leftJoyY;		
				break;
			case PSE_PAD_TYPE_ANALOGPAD: // scph1150
				mtappar[3+psize*i] = 0x73;
				mtappar[3+psize*i+2] = pad[i].buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad[i].buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad[i].rightJoyX;
				mtappar[3+psize*i+5] = pad[i].rightJoyY;
				mtappar[3+psize*i+6] = pad[i].leftJoyX;
				mtappar[3+psize*i+7] = pad[i].leftJoyY;			
				break;
			case PSE_PAD_TYPE_ANALOGJOY: // scph1110
				mtappar[3+psize*i] = 0x53;
				mtappar[3+psize*i+2] = pad[i].buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad[i].buttonStatus >> 8;
				mtappar[3+psize*i+4] = pad[i].rightJoyX;
				mtappar[3+psize*i+5] = pad[i].rightJoyY;
				mtappar[3+psize*i+6] = pad[i].leftJoyX;
				mtappar[3+psize*i+7] = pad[i].leftJoyY;				
				break;
			case PSE_PAD_TYPE_STANDARD:
			default:
				mtappar[3+psize*i] = 0x41;
				mtappar[3+psize*i+2] = pad[i].buttonStatus & 0xff;
				mtappar[3+psize*i+3] = pad[i].buttonStatus >> 8;				
		}
	}
	memcpy(buf, mtappar, 34);
	bufcount = 34;
	return buf[bufc++];
}

unsigned char _PADpoll_old(unsigned char value) {
	if (bufc > bufcount) return 0;
	return buf[bufc++];		
}

unsigned char _PADpoll(unsigned char value) {
	if (!Config.UsingAnalogHack || ((Config.PadState.padID[Config.PadState.curPad] & 0xf0) == 0x40))
	{
		return _PADpoll_old(value);
	}
	else 
	{			
		return PADpoll_SSS(value);				
	}
}

unsigned char _PADstartPoll(PadDataS *pad)
{	
	if (!Config.UsingAnalogHack || (pad->controllerType == PSE_PAD_TYPE_STANDARD))
	{
		
		return _PADstartPoll_old(pad);
	}
	else 
	{		
		Config.PadState.curByte = 0;
		return 0xff;
	}
}

unsigned char PAD1_startPoll(int pad) {
	PadDataS padd; //Pad that is read in
	PadDataS Mpadds[4]; //Place to store all 4 pads data
	Config.PadState.curPad = 0;
	PadDataS epadd; //Set up an empty pad to fill the buffer
	epadd.buttonStatus = 0xffff;
	epadd.leftJoyX = 128;
	epadd.leftJoyY = 128;
	epadd.rightJoyX = 128;
	epadd.rightJoyY = 128;	
	epadd.moveX = 0;
	epadd.moveY = 0;
	epadd.controllerType = Movie.padType1;		
	UpdateState(Config.PadState.curPad);
	PAD1_readPort1(&padd);	
	if (padd.controllerType != PSE_PAD_TYPE_MOUSE)
	{
		padd.moveX = 0;
		padd.moveY = 0;
	}
	if (Movie.mode != 0)	
		padd.controllerType = Movie.padType1;
	memcpy(&PaddInput,&padd,sizeof(padd));
	
	if (Config.enable_extern_analog)
	{
		padd.leftJoyX = Config.PadLeftX;
		padd.leftJoyY = Config.PadLeftY;
		padd.rightJoyX = Config.PadRightX;
		padd.rightJoyY = Config.PadRightY;
	}
	else
	{
		Config.WriteAnalog = true;
		Config.PadLeftX = (padd.leftJoyX);
		Config.PadLeftY = (padd.leftJoyY);
		Config.PadRightX = (padd.rightJoyX);
		Config.PadRightY =(padd.rightJoyY);
	}
	if (Config.EnableAutoFire) 
	{
		
		if (Config.AutoFireFrame) 
		{			
			padd.buttonStatus &= (Config.Pad1AutoFire ^ 0xffff); //Force pad on  and value with zero, force to zero. 
		}
		else
		{
			padd.buttonStatus |= (Config.Pad1AutoFire); //Force pad off, or value with one;
		}
	}
	if (Config.EnableAutoHold)
	{		
		padd.buttonStatus &= (Config.Pad1AutoHold ^ 0xffff); //Force pad on  and value with zero, force to zero. 			
	}
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
				Config.WriteAnalog = true;
				Config.PadLeftX = (padd.leftJoyX);
				Config.PadLeftY = (padd.leftJoyY);
				Config.PadRightX = (padd.rightJoyX);
				Config.PadRightY =(padd.rightJoyY);
			}
			memcpy(&Movie.lastPads1[0],&padd,sizeof(padd));
			iJoysToPoll--;
			return _PADstartPoll(&padd);
		}
		else
		{
			memcpy(&padd,&Movie.lastPads1[0],sizeof(padd));
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
							memcpy(&Mpadds[Player],&epadd,sizeof(padd));
						} 
						else
						{
							MOV_ReadJoy(&Mpadds[Player],Movie.padType1);
						}
					}				
				} else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames) {
					MOV_ReadJoy(&Mpadds[Player],Movie.padType1);
				}
				memcpy(&Movie.lastPads1[Player],&Mpadds[Player],sizeof(padd));				
			}			
			iJoysToPoll--;
			return _xPADstartPoll(&Mpadds[0]);
		}
		else
		{
			memcpy(&Mpadds,&Movie.lastPads1,sizeof(Mpadds));			
			return _xPADstartPoll(&Mpadds[0]);
		}
	}
}


unsigned char PAD1_poll(unsigned char value) {
	return _PADpoll(value);
}

long CALLBACK PAD1__configure(void) { return 0; }
void CALLBACK PAD1__about(void) {}
long CALLBACK PAD1__test(void) { return 0; }
long CALLBACK PAD1__query(void) { return 3; }
long CALLBACK PAD1__keypressed() { return 0; }


unsigned char PAD2_startPoll(int pad) {
	PadDataS padd;
	PadDataS Mpadds[4];
	Config.PadState.curPad = 0;
	PadDataS epadd; //empty pad;
	epadd.buttonStatus = 0xffff;
	epadd.leftJoyX = 128;
	epadd.leftJoyY = 128;
	epadd.rightJoyX = 128;
	epadd.rightJoyY = 128;
	epadd.moveX = 0;
	epadd.moveY = 0;
	epadd.controllerType = Movie.padType2;
	if (Movie.MultiTrack && ((Movie.RecordPlayer >= Movie.P2_Start) || (Movie.RecordPlayer == Movie.NumPlayers+2)))
	{
		PAD2_readPort2(&padd);
		memcpy(&padd,&PaddInput,sizeof(padd));
	}
	else
	{
		PAD2_readPort2(&padd);
	}
	if (padd.controllerType != PSE_PAD_TYPE_MOUSE)
	{
		padd.moveX = 0;
		padd.moveY = 0;
	}
	if (Config.EnableAutoFire) 
	{
		if (Config.AutoFireFrame) 
		{			
			padd.buttonStatus &= (Config.Pad2AutoFire ^ 0xffff); //Force pad on  and value with zero, force to zero. 
		}
		else
		{
			padd.buttonStatus |= (Config.Pad2AutoFire); //Force pad off, or value with one;
		}
	}
	if (Config.EnableAutoHold)
	{		
		padd.buttonStatus &= (Config.Pad2AutoHold ^ 0xffff); //Force pad on  and value with zero, force to zero. 			
	}
	if (Movie.mode != 0)	
		padd.controllerType = Movie.padType2; //Force Controller to match movie file. 
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
						MOV_WriteJoy(&epadd,Movie.padType2);
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
		memcpy(&Movie.lastPads2[0],&padd,sizeof(padd));
		iJoysToPoll--;
		return _PADstartPoll(&padd);
	}
	else
	{
		memcpy(&padd,&Movie.lastPads2[0],sizeof(padd));
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
						memcpy(&Mpadds[Player],&padd,sizeof(padd));
					}
					else 
					{
						if (Movie.currentFrame >= Movie.MaxRecFrames)
						{							
							MOV_WriteJoy(&epadd,Movie.padType2);
							memcpy(&Mpadds[Player],&epadd,sizeof(epadd));
						} 						
						else
						{
							MOV_ReadJoy(&Mpadds[Player],Movie.padType2);
						}
					}				
				} else if (Movie.mode == MOVIEMODE_PLAY && Movie.currentFrame < Movie.totalFrames) {
					MOV_ReadJoy(&Mpadds[Player],Movie.padType2);
				}
				memcpy(&Movie.lastPads2[Player],&Mpadds[Player],sizeof(Mpadds));				
			}
			iJoysToPoll--;
			return _xPADstartPoll(&Mpadds[0]);
		}
		else
		{
			memcpy(&Mpadds,&Movie.lastPads2,sizeof(Mpadds));			
			return _xPADstartPoll(&Mpadds[0]);
		}
	}
}

unsigned char PAD2_poll(unsigned char value) {
	return _PADpoll(value);
}

long CALLBACK PAD2__configure(void) { return 0; }
void CALLBACK PAD2__about(void) {}
long CALLBACK PAD2__test(void) { return 0; }
long CALLBACK PAD2__query(void) { return 3; }
long CALLBACK PAD2__keypressed() { return 0; }



void CALLBACK clearDynarec(void) {
	psxCpu->Reset();
}

int LoadPlugins() {
	int ret;


	ret = CDRinit();
	if (ret < 0) { SysMessage (_("CDRinit error : %d"), ret); return -1; }
	ret = GPUinit();
	if (ret < 0) { SysMessage (_("GPUinit error: %d"), ret); return -1; }
	ret = SPUinit();
	if (ret < 0) { SysMessage (_("SPUinit error: %d"), ret); return -1; }
	InitDirectInput();
	return 0;
}

void ReleasePlugins() {
	GPUshutdown(); //This is the only one that actually does anything. 

}
