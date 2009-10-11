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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#if defined (__WIN32__)
#include "Win32\plugin.h"
#elif defined(__LINUX__) || defined(__MACOSX__)
typedef void* HWND;
#define CALLBACK
#include "Linux/Plugin.h"
#elif defined(__DREAMCAST__)
typedef void* HWND;
#define CALLBACK
#include "Dreamcast/Plugin.h"
#endif

#include "PSEmu_Plugin_Defs.h"
#include "PsxCommon.h"
#include "Decode_XA.h"

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
void SPUwriteRegister(unsigned long, unsigned short);	
unsigned short SPUreadRegister(unsigned long);		
void SPUwriteDMA(unsigned short);
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
struct SPUFreeze_t;
long SPUfreeze(unsigned long, SPUFreeze_t *);
void SPUasync(unsigned long);
void SPUstartWav(char*);
void SPUstopWav(void);


START_EXTERN_C

typedef unsigned long (CALLBACK* PSEgetLibType)(void);
typedef unsigned long (CALLBACK* PSEgetLibVersion)(void);
typedef char *(CALLBACK* PSEgetLibName)(void);

///GPU PLUGIN STUFF 
typedef long (CALLBACK* GPUinit)(void);
typedef long (CALLBACK* GPUshutdown)(void);
typedef long (CALLBACK* GPUclose)(void);
typedef void (CALLBACK* GPUwriteStatus)(unsigned long);
typedef void (CALLBACK* GPUwriteData)(unsigned long);
typedef void (CALLBACK* GPUwriteDataMem)(u32 *, int);
typedef unsigned long (CALLBACK* GPUreadStatus)(void);
typedef unsigned long (CALLBACK* GPUreadData)(void);
typedef void (CALLBACK* GPUreadDataMem)(u32 *, int);
typedef long (CALLBACK* GPUdmaChain)(u32 *,unsigned long);
typedef void (CALLBACK* GPUupdateLace)(void);
typedef long (CALLBACK* GPUconfigure)(void);
typedef long (CALLBACK* GPUtest)(void);
typedef void (CALLBACK* GPUabout)(void);
typedef void (CALLBACK* GPUmakeSnapshot)(void);
typedef void (CALLBACK* GPUkeypressed)(int);
typedef void (CALLBACK* GPUdisplayText)(char *);
typedef struct {
	unsigned long ulFreezeVersion;
	unsigned long ulStatus;
	unsigned long ulControl[256];
	unsigned char psxVRam[1024*512*2];
} GPUFreeze_t;
typedef long (CALLBACK* GPUfreeze)(unsigned long, GPUFreeze_t *);
typedef long (CALLBACK* GPUgetScreenPic)(unsigned char *);
typedef long (CALLBACK* GPUshowScreenPic)(unsigned char *);
typedef void (CALLBACK* GPUclearDynarec)(void (CALLBACK *callback)(void));

typedef void (CALLBACK* GPUsetframelimit)(unsigned long);
typedef void (CALLBACK* GPUsetframecounter)(unsigned long, unsigned long);
typedef void (CALLBACK* GPUsetlagcounter)(unsigned long);
typedef void (CALLBACK* GPUinputdisplay)(unsigned long);
typedef void (CALLBACK* GPUupdateframe)(void);
typedef void (CALLBACK* GPUsetcurrentmode)(char);
typedef void (CALLBACK* GPUsetspeedmode)(unsigned long);
typedef void (CALLBACK* GPUshowframecounter)(void);
typedef long (CALLBACK* GPUstartAvi)(char* filename);
typedef long (CALLBACK* GPUstopAvi)(void);
typedef long (CALLBACK* GPUsendFpLuaGui)(void (*fpPCSX_LuaGui)(void *,int,int,int,int));

//plugin stuff From Shadow
// *** walking in the valley of your darking soul i realize that i was alone
//Gpu function pointers
extern GPUupdateLace       GPU_updateLace;
extern GPUinit             GPU_init;
extern GPUshutdown         GPU_shutdown; 
extern GPUconfigure        GPU_configure;
extern GPUtest             GPU_test;
extern GPUabout            GPU_about;
extern GPUopen             GPU_open;
extern GPUclose            GPU_close;
extern GPUreadStatus       GPU_readStatus;
extern GPUreadData         GPU_readData;
extern GPUreadDataMem      GPU_readDataMem;
extern GPUwriteStatus      GPU_writeStatus; 
extern GPUwriteData        GPU_writeData;
extern GPUwriteDataMem     GPU_writeDataMem;
extern GPUdmaChain         GPU_dmaChain;
extern GPUkeypressed       GPU_keypressed;
extern GPUdisplayText      GPU_displayText;
extern GPUmakeSnapshot     GPU_makeSnapshot;
extern GPUfreeze           GPU_freeze;
extern GPUgetScreenPic     GPU_getScreenPic;
extern GPUshowScreenPic    GPU_showScreenPic;
extern GPUclearDynarec     GPU_clearDynarec;
extern GPUsetframelimit    GPU_setframelimit;
extern GPUsetframecounter  GPU_setframecounter;
extern GPUsetlagcounter    GPU_setlagcounter;
extern GPUinputdisplay     GPU_inputdisplay;
extern GPUupdateframe      GPU_updateframe;
extern GPUsetcurrentmode   GPU_setcurrentmode;
extern GPUsetspeedmode     GPU_setspeedmode;
extern GPUshowframecounter GPU_showframecounter;
extern GPUstartAvi         GPU_startAvi;
extern GPUstopAvi          GPU_stopAvi;
extern GPUsendFpLuaGui     GPU_sendFpLuaGui;

//cd rom plugin ;)
typedef long (CALLBACK* CDRinit)(void);
typedef long (CALLBACK* CDRshutdown)(void);
typedef long (CALLBACK* CDRopen)(void);
typedef long (CALLBACK* CDRclose)(void);
typedef long (CALLBACK* CDRgetTN)(unsigned char *);
typedef long (CALLBACK* CDRgetTD)(unsigned char , unsigned char *);
typedef long (CALLBACK* CDRreadTrack)(unsigned char *);
typedef unsigned char * (CALLBACK* CDRgetBuffer)(void);
typedef long (CALLBACK* CDRconfigure)(void);
typedef long (CALLBACK* CDRtest)(void);
typedef void (CALLBACK* CDRabout)(void);
typedef long (CALLBACK* CDRplay)(unsigned char *);
typedef long (CALLBACK* CDRstop)(void);
struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];
};
typedef long (CALLBACK* CDRgetStatus)(struct CdrStat *);
typedef char* (CALLBACK* CDRgetDriveLetter)(void);
struct SubQ {
	char res0[11];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	char res1[72];
};
typedef unsigned char* (CALLBACK* CDRgetBufferSub)(void);

//cd rom function pointers 
extern CDRinit               CDR_init;
extern CDRshutdown           CDR_shutdown;
extern CDRopen               CDR_open;
extern CDRclose              CDR_close; 
extern CDRtest               CDR_test;
extern CDRgetTN              CDR_getTN;
extern CDRgetTD              CDR_getTD;
extern CDRreadTrack          CDR_readTrack;
extern CDRgetBuffer          CDR_getBuffer;
extern CDRplay               CDR_play;
extern CDRstop               CDR_stop;
extern CDRgetStatus          CDR_getStatus;
extern CDRgetDriveLetter     CDR_getDriveLetter;
extern CDRgetBufferSub       CDR_getBufferSub;
extern CDRconfigure          CDR_configure;
extern CDRabout              CDR_about;

typedef long (CALLBACK* PADconfigure)(void);
typedef void (CALLBACK* PADabout)(void);
typedef long (CALLBACK* PADinit)(long);
typedef long (CALLBACK* PADshutdown)(void);	
typedef long (CALLBACK* PADtest)(void);		
typedef long (CALLBACK* PADclose)(void);
typedef long (CALLBACK* PADquery)(void);
typedef long (CALLBACK*	PADreadPort1)(PadDataS*);
typedef long (CALLBACK* PADreadPort2)(PadDataS*);
typedef long (CALLBACK* PADkeypressed)(void);
typedef unsigned char (CALLBACK* PADstartPoll)(int);
typedef unsigned char (CALLBACK* PADpoll)(unsigned char);
typedef void (CALLBACK* PADsetSensitive)(int);

//PAD POINTERS
extern PADconfigure        PAD1_configure;
extern PADabout            PAD1_about;
extern PADinit             PAD1_init;
extern PADshutdown         PAD1_shutdown;
extern PADtest             PAD1_test;
extern PADopen             PAD1_open;
extern PADclose            PAD1_close;
extern PADquery            PAD1_query;
extern PADreadPort1        PAD1_readPort1;
extern PADkeypressed       PAD1_keypressed;
extern PADstartPoll        PAD1_startPoll;
extern PADpoll             PAD1_poll;
extern PADsetSensitive     PAD1_setSensitive;

extern PADconfigure        PAD2_configure;
extern PADabout            PAD2_about;
extern PADinit             PAD2_init;
extern PADshutdown         PAD2_shutdown;
extern PADtest             PAD2_test;
extern PADopen             PAD2_open;
extern PADclose            PAD2_close;
extern PADquery            PAD2_query;
extern PADreadPort2        PAD2_readPort2;
extern PADkeypressed       PAD2_keypressed;
extern PADstartPoll        PAD2_startPoll;
extern PADpoll             PAD2_poll;
extern PADsetSensitive     PAD2_setSensitive;

// NET plugin

typedef long (CALLBACK* NETinit)(void);
typedef long (CALLBACK* NETshutdown)(void);
typedef long (CALLBACK* NETclose)(void);
typedef long (CALLBACK* NETconfigure)(void);
typedef long (CALLBACK* NETtest)(void);
typedef void (CALLBACK* NETabout)(void);
typedef void (CALLBACK* NETpause)(void);
typedef void (CALLBACK* NETresume)(void);
typedef long (CALLBACK* NETqueryPlayer)(void);
typedef long (CALLBACK* NETsendData)(void *, int, int);
typedef long (CALLBACK* NETrecvData)(void *, int, int);
typedef long (CALLBACK* NETsendPadData)(void *, int);
typedef long (CALLBACK* NETrecvPadData)(void *, int);

typedef struct {
	char EmuName[32];
	char CdromID[10];	// ie. 'SCPH12345', no \0 trailing character
	char CdromLabel[33];
	void *psxMem;
	GPUshowScreenPic GPU_showScreenPic;
	GPUdisplayText GPU_displayText;
	PADsetSensitive PAD_setSensitive;
	char GPUpath[256];	// paths must be absolute
	char SPUpath[256];
	char CDRpath[256];
	char MCD1path[256];
	char MCD2path[256];
	char BIOSpath[256];	// 'HLE' for internal bios
	char Unused[1024];
} netInfo;

typedef long (CALLBACK* NETsetInfo)(netInfo *);
typedef long (CALLBACK* NETkeypressed)(int)
;


// NET function pointers 
extern NETinit               NET_init;
extern NETshutdown           NET_shutdown;
extern NETopen               NET_open;
extern NETclose              NET_close; 
extern NETtest               NET_test;
extern NETconfigure          NET_configure;
extern NETabout              NET_about;
extern NETpause              NET_pause;
extern NETresume             NET_resume;
extern NETqueryPlayer        NET_queryPlayer;
extern NETsendData           NET_sendData;
extern NETrecvData           NET_recvData;
extern NETsendPadData        NET_sendPadData;
extern NETrecvPadData        NET_recvPadData;
extern NETsetInfo            NET_setInfo;
extern NETkeypressed         NET_keypressed;

END_EXTERN_C

int LoadCDRplugin(char *CDRdll);
int LoadGPUplugin(char *GPUdll);
int LoadSPUplugin(char *SPUdll);
int LoadPAD1plugin(char *PAD1dll);
int LoadPAD2plugin(char *PAD2dll);
int LoadNETplugin(char *NETdll);

void CALLBACK clearDynarec(void);

#endif /* __PLUGINS_H__ */
