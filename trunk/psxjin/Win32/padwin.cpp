#define WINVER 0x0500
#define _WIN32_WINNT WINVER
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "dinput.h"

#include "PSXCommon.h"
#include "plugins.h"
#include "resource.h"


HWND hTargetWnd;

struct 
{
	LPDIRECTINPUT8 pDInput;
	LPDIRECTINPUTDEVICE8 pDKeyboard;
	LPDIRECTINPUTDEVICE8 pDDevice[4];
	LPDIRECTINPUTEFFECT pDEffect[4][2]; /* for Small & Big Motor */
	DIJOYSTATE JoyState[4];
} dx8;


static BOOL CALLBACK EnumAxesCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	LPDIRECTINPUTDEVICE8 pDDevice = (LPDIRECTINPUTDEVICE8)pvRef;
	DIPROPRANGE diprg;
	diprg.diph.dwSize = sizeof (diprg);
	diprg.diph.dwHeaderSize	= sizeof (diprg.diph);
	diprg.diph.dwObj = lpddoi->dwType;
	diprg.diph.dwHow = DIPH_BYID;
	diprg.lMin = -128;
	diprg.lMax = 127;
	pDDevice->SetProperty (DIPROP_RANGE, &diprg.diph);
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumJoysticksCallback (const DIDEVICEINSTANCE* instance, VOID* pContext)
{
	const int devno = Config.PadState.devcnt;
	if (devno >= 4)
		return DIENUM_STOP;
	HRESULT result = dx8.pDInput->CreateDevice (instance->guidInstance, &dx8.pDDevice[devno], NULL);
	if (FAILED (result))
		return DIENUM_CONTINUE;
	Config.PadState.devcnt++;
	return DIENUM_CONTINUE;
}

bool ReleaseDirectInput (void)
{
	int index = 4;
	while (index--)
	{
		if (dx8.pDEffect[index][0])
		{
			dx8.pDEffect[index][0]->Unload();
			dx8.pDEffect[index][0]->Release();
			dx8.pDEffect[index][0] = NULL;
		}
		if (dx8.pDEffect[index][1])
		{
			dx8.pDEffect[index][1]->Unload();
			dx8.pDEffect[index][1]->Release();
			dx8.pDEffect[index][1] = NULL;
		}
		if (dx8.pDDevice[index])
		{
			dx8.pDDevice[index]->Unacquire();
			dx8.pDDevice[index]->Release();
			dx8.pDDevice[index] = NULL;
		}
	}
	if (dx8.pDKeyboard)
	{
		dx8.pDKeyboard->Unacquire();
		dx8.pDKeyboard->Release();
		dx8.pDKeyboard = NULL;
	}
	if (dx8.pDInput)
	{
		dx8.pDInput->Release();
		dx8.pDInput = NULL;
	}
	Config.PadState.devcnt = 0;
	return FALSE;
}

bool InitDirectInput (void)
{
	if (dx8.pDInput)
		return TRUE;
	HRESULT result = DirectInput8Create (gApp.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dx8.pDInput, NULL);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = dx8.pDInput->CreateDevice (GUID_SysKeyboard, &dx8.pDKeyboard, NULL);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = dx8.pDInput->EnumDevices (DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = dx8.pDKeyboard->SetDataFormat (&c_dfDIKeyboard);
	if (FAILED (result))
		return ReleaseDirectInput();
	if (hTargetWnd)
	{
		dx8.pDKeyboard->Unacquire();
		result = dx8.pDKeyboard->SetCooperativeLevel (hTargetWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		if (FAILED (result))
			return ReleaseDirectInput();
	}
	int index = Config.PadState.devcnt;
	while (index--)
	{
		const LPDIRECTINPUTDEVICE8 pDDevice = dx8.pDDevice[index];
		result = pDDevice->SetDataFormat (&c_dfDIJoystick);
		if (FAILED (result))
			return ReleaseDirectInput();
		if (hTargetWnd)
		{
			pDDevice->Unacquire();
			result = pDDevice->SetCooperativeLevel (hTargetWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
			if (FAILED (result))
				return ReleaseDirectInput();
		}
		struct
		{
			DIPROPDWORD dipdw;
			DWORD rgdwAxes[2];
			LONG rglDirection[2];
			DIPERIODIC per;
			DICONSTANTFORCE cf;
			DIEFFECT eff;
		} local;
		memset (&local, 0, sizeof (local));
		local.dipdw.diph.dwSize = sizeof (DIPROPDWORD);
		local.dipdw.diph.dwHeaderSize = sizeof (DIPROPHEADER);
		local.dipdw.diph.dwHow = DIPH_DEVICE;
		local.dipdw.dwData = DIPROPAUTOCENTER_OFF;
		pDDevice->SetProperty (DIPROP_AUTOCENTER, &local.dipdw.diph);
		result = pDDevice->EnumObjects (EnumAxesCallback, pDDevice, DIDFT_AXIS);
		if (FAILED (result))
			return ReleaseDirectInput();

		local.rgdwAxes[0] = DIJOFS_X;
		local.rgdwAxes[1] = DIJOFS_Y;
		local.eff.dwSize = sizeof (DIEFFECT);
		local.eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		local.eff.dwDuration = INFINITE;
		local.eff.dwGain = DI_FFNOMINALMAX;
		local.eff.dwTriggerButton = DIEB_NOTRIGGER;
		local.eff.cAxes = 2;
		local.eff.rgdwAxes = local.rgdwAxes;
		local.eff.rglDirection = local.rglDirection;

		/* Small Motor */
		local.eff.cbTypeSpecificParams = sizeof (DIPERIODIC);
		local.eff.lpvTypeSpecificParams = &local.per;
		result = pDDevice->CreateEffect (GUID_Square , &local.eff, &dx8.pDEffect[index][0], NULL);
		if (FAILED (result))
			dx8.pDEffect[index][0] = NULL;

		/* Big Motor */
		local.eff.cbTypeSpecificParams = sizeof (DICONSTANTFORCE);
		local.eff.lpvTypeSpecificParams = &local.cf;
		result = pDDevice->CreateEffect (GUID_ConstantForce , &local.eff, &dx8.pDEffect[index][1], NULL);
		if (FAILED (result))
			dx8.pDEffect[index][1] = NULL;
 	}
	return TRUE;
}

static bool AcquireDevice (LPDIRECTINPUTDEVICE8 lpDirectInputDevice)
{
	if (FAILED (lpDirectInputDevice->Acquire()))
	{
		HRESULT result = lpDirectInputDevice->Acquire();
		if (result == DIERR_OTHERAPPHASPRIO)
			return FALSE;
		if (FAILED (result))
			return ReleaseDirectInput();
	}
	return TRUE;
}



static bool GetJoyState (const int devno)
{
	InitDirectInput();
	if (dx8.pDDevice[devno] == NULL)
		return FALSE;
	dx8.pDDevice[devno]->Poll();
	if (FAILED (dx8.pDDevice[devno]->GetDeviceState (sizeof (DIJOYSTATE), &dx8.JoyState[devno])))
	{
		AcquireDevice (dx8.pDDevice[devno]);
		return FALSE;
	}
	return TRUE;
}

static bool GetKeyState (u8* keyboard)
{
	InitDirectInput();
	if (dx8.pDKeyboard == NULL)
		return FALSE;
	dx8.pDKeyboard->Poll();
	if (FAILED (dx8.pDKeyboard->GetDeviceState (256, keyboard)))
	{
		AcquireDevice (dx8.pDKeyboard);
		return FALSE;
	}
	return TRUE;
}

int PadFreeze(gzFile f, int Mode) {
	gzfreezel(&Config.PadState);
	return 0;
}


static void SaveConfig (void)
{
	char Str_Tmp[1024];
	char Pad_Tmp[1024];
	for (int j = 0; j < 2; j++)
	{
		sprintf(Pad_Tmp, "PAD%d Type",j);
		sprintf(Str_Tmp, "%d",Config.PadState.padID[j]);
		WritePrivateProfileString("Controllers", Pad_Tmp, Str_Tmp, Config.Conf_File);
		for(int i = 0; i < 21; i++)
		{
			sprintf(Str_Tmp, "%d",Config.KeyConfig.keys[j][i]);
			sprintf(Pad_Tmp, "PAD%d_%d",j, i);
			WritePrivateProfileString("Controllers", Pad_Tmp, Str_Tmp, Config.Conf_File);
		}
	}
	
}

static void LoadConfig (void)
{	
	char Pad_Tmp[1024];
	for (int j = 0; j < 2; j++)
	{
		sprintf(Pad_Tmp, "PAD%d Type",j);
		Config.PadState.padID[j] = (u32)GetPrivateProfileInt("Controllers", Pad_Tmp, 0, Config.Conf_File);
		for(int i = 0; i < 21; i++)
		{			
			sprintf(Pad_Tmp, "PAD%d_%d",j, i);
			Config.KeyConfig.keys[j][i] = (u32)GetPrivateProfileInt("Controllers", Pad_Tmp, 0, Config.Conf_File);
		}
	}	
	Config.PadState.padVibC[0] = Config.PadState.padVibC[1] = -1;
	for (int cnt = 21; cnt--; )
	{
		const int key0 = Config.KeyConfig.keys[0][cnt];
		if (key0 >= 0x1000)
			Config.PadState.padVibC[0] = (key0 & 0xfff) / 0x100;
		const int key1 = Config.KeyConfig.keys[1][cnt];
		if (key1 >= 0x1000)
			Config.PadState.padVibC[1] = (key1 & 0xfff) / 0x100;
	}
}

static void PADsetMode (const int pad, const int mode)
{
	static const u8 padID[] = { 0x41, 0x73, 0x41, 0x73 };
	Config.PadState.padMode1[pad] = mode;
	Config.PadState.padVib0[pad] = 0;
	Config.PadState.padVib1[pad] = 0;
	Config.PadState.padVibF[pad][0] = 0;
	Config.PadState.padVibF[pad][1] = 0;
	Config.PadState.padID[pad] = padID[Config.PadState.padMode2[pad] * 2 + mode];
}

static void KeyPress (const int pad, const int index, const bool press)
{
	if (index < 16)
	{
		if (press)
		{
			Config.PadState.padStat[pad] &= ~(1 << index);
			if (Config.PadState.padPress[pad][index] == 0)
				Config.PadState.padPress[pad][index] = GetTickCount();
		}
		else
		{
			Config.PadState.padStat[pad] |= 1 << index;
			Config.PadState.padPress[pad][index] = 0;
		}
	}
	else
	{
		static bool prev[2] = { FALSE, FALSE };
		if ((prev[pad] != press) && (Config.PadState.padModeF[pad] == 0))
		{
			prev[pad] = press;
			if (press) PADsetMode (pad, !Config.PadState.padMode1[pad]);
		}
	}
}

void UpdateState (const int pad)
{
	static int flag_keyboard;
	static int flag_joypad[4];
	if (pad == 0)
	{
		flag_keyboard = 0;
		flag_joypad[0] = 0;
		flag_joypad[1] = 0;
		flag_joypad[2] = 0;
		flag_joypad[3] = 0;
	}
	static u8 keystate[256];
	for (int index = 17; index--; )
	{
		const int key = Config.KeyConfig.keys[pad][index];
		if (key == 0)
			continue;
		else if (key < 0x100)
		{
			if (flag_keyboard == FALSE)
			{
				flag_keyboard = TRUE;
				if (GetKeyState (keystate) == FALSE)
					return;
			}
			KeyPress (pad, index, keystate[key] & 0x80);
		}
		else
		{
			const int joypad = ((key & 0xfff) / 100);
			if (flag_joypad[joypad] == FALSE)
			{
				flag_joypad[joypad] = TRUE;
				if (GetJoyState (joypad) == FALSE)
					return;
			}
			if (key < 0x2000)
			{
				KeyPress (pad, index, dx8.JoyState[joypad].rgbButtons[key & 0xff]);
			}
			else if (key < 0x3000)
			{
				const int state = ((int*)&dx8.JoyState[joypad].lX)[(key & 0xff) /2];
				switch (key & 1)
				{
				case 0: KeyPress (pad, index, state < -64); break;
				case 1: KeyPress (pad, index, state >= 64); break;
				}
			}
			else
			{
				const u32 state = dx8.JoyState[joypad].rgdwPOV[(key & 0xff) /4];
				switch (key & 3)
				{
				case 0: KeyPress (pad, index, (state >= 0 && state <= 4500) || (state >= 31500 && state <= 36000)); break;
				case 1: KeyPress (pad, index, state >= 4500  && state <= 13500); break;
				case 2: KeyPress (pad, index, state >= 13500 && state <= 22500); break;
				case 3: KeyPress (pad, index, state >= 22500 && state <= 31500); break;
				}
			}
		}
	}

	
}
	


static void set_label (const HWND hWnd, const int pad, const int index)
{
	const int key = Config.KeyConfig.keys[pad][index];
	char buff[64];
	if (key < 0x100)
	{
		if (key == 0)
			strcpy (buff, "NONE");
		else if (GetKeyNameText (key << 16, buff, sizeof (buff)) == 0)
			wsprintf (buff, "Keyboard 0x%02X", key);
	}
	else if (key >= 0x1000 && key < 0x2000)
	{
		wsprintf (buff, "J%d_%d", (key & 0xfff) / 0x100, (key & 0xff) + 1);
	}
	else if (key >= 0x2000 && key < 0x3000)
	{
		static const char name[][4] = { "MIN", "MAX" };
		const int axis = (key & 0xff);
		wsprintf (buff, "J%d_AXIS%d_%s", (key & 0xfff) / 0x100, axis / 2, name[axis % 2]);
		if (index >= 17 && index <= 20)
			buff[strlen (buff) -4] = '\0';
	}
	else if (key >= 0x3000 && key < 0x4000)
	{
		static const char name[][7] = { "FOWARD", "RIGHT", "BACK", "LEFT" };
		const int pov = (key & 0xff);
		wsprintf (buff, "J%d_POV%d_%s", (key & 0xfff) / 0x100, pov /4, name[pov % 4]);
	}
	Button_SetText (GetDlgItem (hWnd, IDC_ESELECT + index), buff);
}



s32 PADopen (HWND hWnd)
{
	if (!IsWindow (hWnd) && !IsBadReadPtr ((u32*)hWnd, 4))
		hWnd = *(HWND*)hWnd;
	if (!IsWindow (hWnd))
		hWnd = NULL;
	else
	{
		while (GetWindowLong (hWnd, GWL_STYLE) & WS_CHILD)
			hWnd = GetParent (hWnd);
	}
	hTargetWnd = hWnd;
	memset (&Config.PadState, 0, sizeof (PadDef));
	Config.PadState.padStat[0] = 0xffff;
	Config.PadState.padStat[1] = 0xffff;
	LoadConfig();
	PADsetMode (0, (int)((Config.PadState.padID[0] & 0xf0) == 0x40));
	PADsetMode (1, (int)((Config.PadState.padID[1] & 0xf0) == 0x40));	
	return 0;
}


u32 CALLBACK PADquery (void)
{
	return 3;
}


static const u8 cmd40[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5a
};
static const u8 cmd41[8] =
{
	0xff, 0x5a, 0xff, 0xff, 0x03, 0x00, 0x00, 0x5a,
};
static const u8 cmd44[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const u8 cmd45[8] =
{
	0xff, 0x5a, 0x00, 0x02, 0x01, 0x02, 0x01, 0x00,
};
static const u8 cmd46[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0a,
};
static const u8 cmd47[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
};
static const u8 cmd4c[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const u8 cmd4d[8] =
{
	0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const u8 cmd4f[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a,
};

static u8 get_analog (const int key)
{
	const int pad = ((key & 0xf00) / 0x100);
	const int pos = ((key & 0x0ff) /2);
	return (u8)(((int*)&dx8.JoyState[pad].lX)[pos] + 128);
}
static u8 buf[20];

void PADstartPoll_SSS(PadDataS *pad)
{
	*(u16*)&buf[2] = pad->buttonStatus;
	buf[4] = pad->rightJoyX;
	buf[5] = pad->rightJoyY;
	buf[6] = pad->leftJoyX;
	buf[7] = pad->leftJoyY;	
}
u8 PADpoll_SSS (u8 value)
{
	const int pad = Config.PadState.curPad;		
	/*if ((value&0xf0) == 0x40)
	{		
		Config.PadState.curByte = 0;
	}*/
	const int cur = Config.PadState.curByte;
	//printf("(%d)", Config.PadState.curByte);
	if (cur == 0)
	{
		Config.PadState.curByte++;
		Config.PadState.curCmd = value;
		switch (value)
		{
		case 0x40:
			Config.PadState.cmdLen = sizeof (cmd40);
			memcpy (buf, cmd40, sizeof (cmd40));
			return 0xf3;
		case 0x41:
			Config.PadState.cmdLen = sizeof (cmd41);
			memcpy (buf, cmd41, sizeof (cmd41));
			return 0xf3;
		case 0x42:
		case 0x43:			
			Config.PadState.cmdLen = 2 + 2 * (Config.PadState.padID[pad] & 0x0f);
			buf[1] = Config.PadState.padModeC[pad] ? 0x00 : 0x5a;			
			if (value == 0x43 && Config.PadState.padModeE[pad])
			{
				buf[4] = 0;
				buf[5] = 0;
				buf[6] = 0;
				buf[7] = 0;
				return 0xf3;
			}			
			return (u8)Config.PadState.padID[pad];
			break;
		case 0x44:
			Config.PadState.cmdLen = sizeof (cmd44);
			memcpy (buf, cmd44, sizeof (cmd44));
			return 0xf3;
		case 0x45:
			Config.PadState.cmdLen = sizeof (cmd45);
			memcpy (buf, cmd45, sizeof (cmd45));
			buf[4] = (u8)Config.PadState.padMode1[pad];
			return 0xf3;
		case 0x46:
			Config.PadState.cmdLen = sizeof (cmd46);
			memcpy (buf, cmd46, sizeof (cmd46));
			return 0xf3;
		case 0x47:
			Config.PadState.cmdLen = sizeof (cmd47);
			memcpy (buf, cmd47, sizeof (cmd47));
			return 0xf3;
		case 0x4c:
			Config.PadState.cmdLen = sizeof (cmd4c);
			memcpy (buf, cmd4c, sizeof (cmd4c));
			return 0xf3;
		case 0x4d:
			Config.PadState.cmdLen = sizeof (cmd4d);
			memcpy (buf, cmd4d, sizeof (cmd4d));
			return 0xf3;
		case 0x4f:
			Config.PadState.padID[pad] = 0x79;
			Config.PadState.padMode2[pad] = 1;
			Config.PadState.cmdLen = sizeof (cmd4f);
			memcpy (buf, cmd4f, sizeof (cmd4f));
			return 0xf3;
		}
	}
	switch (Config.PadState.curCmd)
	{
	case 0x42:
		break;
	case 0x43:
		if (cur == 2)
		{
			Config.PadState.padModeE[pad] = value;
			Config.PadState.padModeC[pad] = 0;
		}
		break;
	case 0x44:
		if (cur == 2)
			PADsetMode (pad, value);
		if (cur == 3)
			Config.PadState.padModeF[pad] = (value == 3);
		break;
	case 0x46:
		if (cur == 2)
		{
			switch(value)
			{
			case 0:
				buf[5] = 0x02;
				buf[6] = 0x00;
				buf[7] = 0x0A;
				break;
			case 1:
				buf[5] = 0x01;
				buf[6] = 0x01;
				buf[7] = 0x14;
				break;
			}
		}
		break;
	case 0x4c:
		if (cur == 2)
		{
			static const u8 buf5[] = { 0x04, 0x07, 0x02, 0x05 };
			buf[5] = buf5[value & 3];
		}
		break;
	case 0x4d:
		if (cur >= 2)
		{
			if (cur == Config.PadState.padVib0[pad])
				buf[cur] = 0x00;
			if (cur == Config.PadState.padVib1[pad])
				buf[cur] = 0x01;
			if (value == 0x00)
			{
				Config.PadState.padVib0[pad] = cur;
				if ((Config.PadState.padID[pad] & 0x0f) < (cur - 1) / 2)
					 Config.PadState.padID[pad] = (Config.PadState.padID[pad] & 0xf0) + (cur - 1) / 2;
			}
			else if (value == 0x01)
			{
				Config.PadState.padVib1[pad] = cur;
				if ((Config.PadState.padID[pad] & 0x0f) < (cur - 1) / 2)
					 Config.PadState.padID[pad] = (Config.PadState.padID[pad] & 0xf0) + (cur - 1) / 2;
			}
		}
		break;
	}
	if (cur >= Config.PadState.cmdLen)
		return 0xff;
	return buf[Config.PadState.curByte++];
}



long PAD1_readPort1(PadDataS* pads)
{		
	memset (pads, 0, sizeof (PadDataS));
	if ((Config.PadState.padID[0] & 0xf0) == 0x40)
		pads->controllerType = 4;
	else
		pads->controllerType = 7;
	pads->buttonStatus = Config.PadState.padStat[0];
	pads->leftJoyX = get_analog (Config.KeyConfig.keys[0][17]);
	pads->leftJoyY = get_analog (Config.KeyConfig.keys[0][18]);
	pads->rightJoyX = get_analog (Config.KeyConfig.keys[0][19]);
	pads->rightJoyY = get_analog (Config.KeyConfig.keys[0][20]);
	pads->moveX = 0;
	pads->moveY = 0;
	return 0;
}

long PAD2_readPort2(PadDataS* pads)
{		
	memset (pads, 0, sizeof (PadDataS));
	if ((Config.PadState.padID[1] & 0xf0) == 0x40)
		pads->controllerType = 4;
	else
		pads->controllerType = 7;
	pads->buttonStatus = Config.PadState.padStat[1];
	pads->leftJoyX = get_analog (Config.KeyConfig.keys[1][17]);
	pads->leftJoyY = get_analog (Config.KeyConfig.keys[1][18]);
	pads->rightJoyX = get_analog (Config.KeyConfig.keys[1][19]);
	pads->rightJoyY = get_analog (Config.KeyConfig.keys[1][20]);
	pads->moveX = 0;
	pads->moveY = 0;
	return 0;
}

keyEvent* CALLBACK PADkeyEvent (void)
{
	static keyEvent ev;
	static u8 state[2][256];
	memcpy (state[0], state[1], sizeof (state[0]));
	GetKeyState (state[1]);
	for (int cnt = 0; cnt < 256; cnt++)
	{
		if (~state[0][cnt] & state[1][cnt] & 0x80)
		{
			ev.event = (state[1][cnt] & 0x80) ? 1 : 2;
			ev.key = MapVirtualKey (cnt, 1);
			return &ev;
		}
	}
	return NULL;
}




LRESULT WINAPI ConfigurePADDlgProc (const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	static BYTE keymaps[2][256];
	static DWORD countdown;
	static int disabled;
	static HWND hTabWnd;
	static int pad;
	int cnt1;
	int cnt2;
	int key;
	switch (msg)
	{
	case WM_INITDIALOG:
		hTargetWnd = hWnd;
		pad = disabled = 0;
		LoadConfig();
		for (cnt1 = 21; cnt1--; )
			set_label (hWnd, pad, cnt1);
		if ((Config.PadState.padID[pad] & 0xf0) == 0x40)
		{
			 CheckDlgButton(hWnd, IDC_DIGITALSELECT, BST_CHECKED);
			 CheckDlgButton(hWnd, IDC_ANALOGSELECT, BST_UNCHECKED);
		}
		else
		{
			 CheckDlgButton(hWnd, IDC_DIGITALSELECT, BST_UNCHECKED);
			 CheckDlgButton(hWnd, IDC_ANALOGSELECT, BST_CHECKED);
		}
		hTabWnd = GetDlgItem (hWnd, IDC_TABC);
		TCITEM tcI;
		tcI.mask = TCIF_TEXT;
		tcI.pszText = "PAD1";
		TabCtrl_InsertItem (hTabWnd, 0, &tcI);
		tcI.mask = TCIF_TEXT;
		tcI.pszText = "PAD2";
		TabCtrl_InsertItem (hTabWnd, 1, &tcI);
		SetTimer (hWnd, 0x80, 50, NULL);
		return TRUE;
	case WM_DESTROY:
		break;
	case WM_NOTIFY:
		if (wParam == IDC_TABC)
		{
			if (disabled)
				EnableWindow (GetDlgItem (hWnd, disabled), TRUE);
			disabled = 0;
			pad = TabCtrl_GetCurSel (hTabWnd);
			for (cnt1 = 21; cnt1--; )
				set_label (hWnd, pad, cnt1);
			if((Config.PadState.padID[pad] & 0xf0) == 0x40)
			{
				 CheckDlgButton(hWnd, IDC_DIGITALSELECT, BST_CHECKED);
				 CheckDlgButton(hWnd, IDC_ANALOGSELECT, BST_UNCHECKED);
			}
			else
			{
				 CheckDlgButton(hWnd, IDC_DIGITALSELECT, BST_UNCHECKED);
				 CheckDlgButton(hWnd, IDC_ANALOGSELECT, BST_CHECKED);
			}
		}
			if (wParam == IDC_DIGITALSELECT || wParam == IDC_ANALOGSELECT)
			{
				if (IsDlgButtonChecked(hWnd,IDC_DIGITALSELECT) == BST_CHECKED) 
			{
				Config.PadState.padID[pad] = 0x41;
			}
			else
			{
				Config.PadState.padID[pad] = 0x73;
			}
		}
		break;
	case WM_COMMAND:
		
		for (cnt1 = 21; cnt1--; )
		{
			if (LOWORD (wParam) == IDC_BSELECT + cnt1)
			{
				if (disabled)
					EnableWindow (GetDlgItem (hWnd, disabled), TRUE);
				EnableWindow (GetDlgItem (hWnd, disabled = wParam), FALSE);
				countdown = GetTickCount();
				GetKeyState (keymaps[0]);
				return TRUE;
			}
		}
		if (LOWORD (wParam) == IDOK)
			EndDialog (hWnd, IDOK);
		else if (LOWORD (wParam) == IDCANCEL)
			EndDialog (hWnd, IDCANCEL);
		break;
	case WM_TIMER:
		if (disabled)
		{
			const int index = disabled - IDC_BSELECT;
			int analog = FALSE;
			UpdateState(0);
			UpdateState(1);
			if ((GetTickCount() - countdown) / 1000 != 10)
			{
				char buff[64];
				wsprintf (buff, "Timeout: %d", 10 - (GetTickCount() - countdown) / 1000);
				SetWindowText (GetDlgItem (hWnd, IDC_ESELECT + index), buff);
			}
			else
			{
				Config.KeyConfig.keys[pad][index] = 0;
				set_label (hWnd, pad, index);
				EnableWindow (GetDlgItem (hWnd, disabled), TRUE);
				disabled = 0;
				break;
			}
			if (GetKeyState (keymaps[1]) == FALSE)
				break;
			for (key = 0x100; key--; )
			{
				if (~keymaps[0][key] & keymaps[1][key] & 0x80)
					break;
			}
			for (cnt1 = Config.PadState.devcnt; cnt1--;)
			{
				if (GetJoyState (cnt1) == FALSE)
					break;

				for (cnt2 = 32; cnt2--; )
				{
					if (dx8.JoyState[cnt1].rgbButtons[cnt2])
						key = 0x1000 + 0x100 * cnt1 + cnt2;
				}
				for (cnt2 = 8; cnt2--; )
				{
					const int now = ((u32*)&dx8.JoyState[cnt1].lX)[cnt2];
					if (now < -64)
					{
						key = 0x2000 + 0x100 * cnt1 + cnt2 * 2 +0;
						analog = TRUE;
					}
					else if (now >= 64)
					{
						key = 0x2000 + 0x100 * cnt1 + cnt2 * 2 +1;
						analog = TRUE;
					}
				}
				for (cnt2 = 4; cnt2--; )
				{
					const u32 now = dx8.JoyState[cnt1].rgdwPOV[cnt2];
					if ((now >= 0 && now < 4500) || (now >= 31500 && now < 36000))
						key = 0x3000 + 0x100 * cnt1 + cnt2 * 4 +0;
					if (now >= 4500 && now < 13500)
						key = 0x3000 + 0x100 * cnt1 + cnt2 * 4 +1;
					if (now >= 13500 && now < 22500)
						key = 0x3000 + 0x100 * cnt1 + cnt2 * 4 +2;
					if (now >= 22500 && now < 31500)
						key = 0x3000 + 0x100 * cnt1 + cnt2 * 4 +3;
				}
			}
			if (index >= 17 && index <= 20 && analog == 0)
				key = 0;
			else if (key > 0)
			{
				if (key != 1)
					Config.KeyConfig.keys[pad][index] = key;
				set_label (hWnd, pad, index);
				EnableWindow (GetDlgItem (hWnd, disabled), TRUE);
				disabled = 0;
			}
		}
	}
	return 0;
}


void PADconfigure (void)
{			
		if (DialogBox (gApp.hInstance, MAKEINTRESOURCE (IDD_CONFIGCONTROL), GetActiveWindow(), (DLGPROC)ConfigurePADDlgProc) == IDOK)
			SaveConfig();		
}
