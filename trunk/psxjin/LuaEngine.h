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

//void PSXjin_LuaWrite(uint32 addr);
void PSXjin_LuaFrameBoundary();
int PSXjin_LoadLuaCode(const char *filename);
void PSXjin_ReloadLuaCode();
void PSXjin_LuaStop();
int PSXjin_LuaRunning();

int PSXjin_LuaUsingJoypad(int);
uint32 PSXjin_LuaReadJoypad(int);
int PSXjin_LuaUsingAnalogJoy(int);
LuaAnalogJoy* PSXjin_LuaReadAnalogJoy(int);
int PSXjin_LuaSpeed();
//int PSXjin_LuaFrameskip();
int PSXjin_LuaRerecordCountSkip();

void PSXjin_LuaGui(void *s, int width, int height, int bpp, int pitch);

void PSXjin_LuaWriteInform();

void PSXjin_LuaClearGui();
void PSXjin_LuaEnableGui(uint8 enabled);

char* PSXjin_GetLuaScriptName();

// Record a command-line argument in lua's global "arg" variable for
// eventual use by the lua script
void PSXjin_LuaAddArgument(char *arg);

#endif
