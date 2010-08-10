#ifndef _LUAENGINE_H
#define _LUAENGINE_H

typedef struct StructLuaAnalogJoy {
	unsigned char xleft;
	unsigned char yleft;
	unsigned char xright;
	unsigned char yright;
} LuaAnalogJoy;

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_BEFOREEXIT,

	LUACALL_COUNT
};
void CallRegisteredLuaFunctions(int calltype);

//void PCSX_LuaWrite(uint32 addr);
void PCSX_LuaFrameBoundary();
int PCSX_LoadLuaCode(const char *filename);
void PCSX_ReloadLuaCode();
void PCSX_LuaStop();
int PCSX_LuaRunning();

int PCSX_LuaUsingJoypad(int);
uint32 PCSX_LuaReadJoypad(int);
int PCSX_LuaUsingAnalogJoy(int);
LuaAnalogJoy* PCSX_LuaReadAnalogJoy(int);
int PCSX_LuaSpeed();
//int PCSX_LuaFrameskip();
int PCSX_LuaRerecordCountSkip();

void PCSX_LuaGui(void *s, int width, int height, int bpp, int pitch);

void PCSX_LuaWriteInform();

void PCSX_LuaClearGui();
void PCSX_LuaEnableGui(uint8 enabled);

char* PCSX_GetLuaScriptName();

#endif
