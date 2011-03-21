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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "Win32\plugin.h"
#include "PSEmu_Plugin_Defs.h"
#include "PsxCommon.h"
#include "Decode_XA.h"
#include "CdRom.h"

#include "iso/cdriso.h"

void PlayMovieFromBeginning();
void ReadonlyToggle();
int  LoadPlugins();
void ReleasePlugins();
int  OpenPlugins();
void ClosePlugins();
void ResetPlugins();


// spu plugin
long SPUinit(void);				
long SPUshutdown(void);	
long SPUclose(void);			
void SPUplaySample(unsigned char);		
void SPUstartChannels1(unsigned short);	
void SPUstartChannels2(unsigned short);
void SPUstopChannels1(unsigned short);	
void SPUstopChannels2(unsigned short);	
void SPUputOne(unsigned long,unsigned short);			
unsigned short SPUgetOne(unsigned long);			
void SPUsetAddr(unsigned char, unsigned short);			
void SPUsetPitch(unsigned char, unsigned short);		
void SPUsetVolumeL(unsigned char, short );		
void SPUsetVolumeR(unsigned char, short );		
//psemu pro 2 functions from now..
void SPUwriteRegister(u32, u16);	
u16 SPUreadRegister(u32);		
void SPUwriteDMA(short);
unsigned short SPUreadDMA(void);
void SPUwriteDMAMem(unsigned short *, int);
void SPUreadDMAMem(unsigned short *, int);
void SPUplayADPCMchannel(xa_decode_t *);
//void SPUregisterCallback(void (CALLBACK *callback)(void));
long SPUconfigure(void);
long SPUopen(HWND hwnd);
struct FreezeStub_t {
	unsigned char PluginName[8];
	unsigned long PluginVersion;
	unsigned long Size;
};
void SPUasync(unsigned long);
void SPUstartWav(char*);
void SPUstopWav(void);


START_EXTERN_C

typedef unsigned long (CALLBACK* PSEgetLibType)(void);
typedef unsigned long (CALLBACK* PSEgetLibVersion)(void);
typedef char *(CALLBACK* PSEgetLibName)(void);

END_EXTERN_C

///GPU PLUGIN STUFF 
long CALLBACK  GPUinit(void);
long CALLBACK  GPUshutdown(void);
long CALLBACK  GPUclose(void);
void CALLBACK  GPUwriteStatus(unsigned long);
void CALLBACK  GPUwriteData(unsigned long);
void CALLBACK  GPUwriteDataMem(u32 *, int);
unsigned long CALLBACK  GPUreadStatus(void);
unsigned long CALLBACK  GPUreadData(void);
void CALLBACK  GPUreadDataMem(u32 *, int);
long CALLBACK  GPUdmaChain(u32 *,unsigned long);
void CALLBACK  GPUupdateLace(void);
long CALLBACK  GPUconfigure(void);
long CALLBACK  GPUtest(void);
void CALLBACK  GPUabout(void);
void CALLBACK  GPUmakeSnapshot(void);
void CALLBACK  GPUkeypressed(int);
void CALLBACK  GPUdisplayText(char *);
#pragma pack(push, 1)
struct GPUFreeze_t{
	void* extraData;
	int extraDataSize;
	unsigned long ulFreezeVersion;
	unsigned long ulStatus;
	unsigned long ulControl[256];
	unsigned char psxVRam[1024*512*2];
};
#pragma pack(pop)
long CALLBACK  GPUfreeze(unsigned long, GPUFreeze_t *);
void CALLBACK  GPUgetScreenPic(unsigned char *);
void CALLBACK  GPUshowScreenPic(unsigned char *);
void CALLBACK  GPUclearDynarec(void (CALLBACK *callback)(void));

void CALLBACK  GPUsetframelimit(unsigned long);
void CALLBACK  GPUsetframecounter(unsigned long, unsigned long);
void CALLBACK  GPUsetlagcounter(unsigned long);
void CALLBACK  GPUinputdisplay(unsigned long);
void CALLBACK  GPUupdateframe(void);
void CALLBACK  GPUsetcurrentmode(char);
void CALLBACK  GPUsetspeedmode(unsigned long);
void CALLBACK  GPUshowframecounter(void);
void CALLBACK  GPUshowInput(void);
void CALLBACK  GPUshowAnalog(void);
void CALLBACK  GPUshowALL(void);
void CALLBACK  GPUstartAvi(char* filename);
void CALLBACK  GPUstopAvi(void);
void CALLBACK GPUrestartAVINewRes(void);
void CALLBACK  GPUsendFpLuaGui(void (*fpPSXjin_LuaGui)(void *,int,int,int,int));


//Padwin Exports
//Should probably put these in a separate .h file, but whatever.

//int LoadCDRplugin(char *CDRdll);
//int LoadSPUplugin(char *SPUdll);

void CALLBACK clearDynarec(void);

#endif /* __PLUGINS_H__ */
