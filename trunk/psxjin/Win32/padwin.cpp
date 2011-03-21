#define WINVER 0x0500
#define _WIN32_WINNT WINVER
#define DIRECTINPUT_VERSION 0x0800

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
	ConfigKey config;
	int devcnt;
	LPDIRECTINPUT8 pDInput;
	LPDIRECTINPUTDEVICE8 pDKeyboard;
	LPDIRECTINPUTDEVICE8 pDDevice[4];
	LPDIRECTINPUTEFFECT pDEffect[4][2]; /* for Small & Big Motor */
	DIJOYSTATE JoyState[4];
	u16 padStat[2];
	int padID[2];
	int padMode1[2];
	int padMode2[2];
	int padModeE[2];
	int padModeC[2];
	int padModeF[2];
	int padVib0[2];
	int padVib1[2];
	int padVibF[2][4];
	int padVibC[2];
	DWORD padPress[2][16];
	int curPad;
	int curByte;
	int curCmd;
	int cmdLen;
} global;


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
	const int devno = global.devcnt;
	if (devno >= 4)
		return DIENUM_STOP;
	HRESULT result = global.pDInput->CreateDevice (instance->guidInstance, &global.pDDevice[devno], NULL);
	if (FAILED (result))
		return DIENUM_CONTINUE;
	global.devcnt++;
	return DIENUM_CONTINUE;
}

bool ReleaseDirectInput (void)
{
	int index = 4;
	while (index--)
	{
		if (global.pDEffect[index][0])
		{
			global.pDEffect[index][0]->Unload();
			global.pDEffect[index][0]->Release();
			global.pDEffect[index][0] = NULL;
		}
		if (global.pDEffect[index][1])
		{
			global.pDEffect[index][1]->Unload();
			global.pDEffect[index][1]->Release();
			global.pDEffect[index][1] = NULL;
		}
		if (global.pDDevice[index])
		{
			global.pDDevice[index]->Unacquire();
			global.pDDevice[index]->Release();
			global.pDDevice[index] = NULL;
		}
	}
	if (global.pDKeyboard)
	{
		global.pDKeyboard->Unacquire();
		global.pDKeyboard->Release();
		global.pDKeyboard = NULL;
	}
	if (global.pDInput)
	{
		global.pDInput->Release();
		global.pDInput = NULL;
	}
	global.devcnt = 0;
	return FALSE;
}

static bool InitDirectInput (void)
{
	if (global.pDInput)
		return TRUE;
	HRESULT result = DirectInput8Create (gApp.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&global.pDInput, NULL);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = global.pDInput->CreateDevice (GUID_SysKeyboard, &global.pDKeyboard, NULL);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = global.pDInput->EnumDevices (DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
	if (FAILED (result))
		return ReleaseDirectInput();
	result = global.pDKeyboard->SetDataFormat (&c_dfDIKeyboard);
	if (FAILED (result))
		return ReleaseDirectInput();
	if (hTargetWnd)
	{
		global.pDKeyboard->Unacquire();
		result = global.pDKeyboard->SetCooperativeLevel (hTargetWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		if (FAILED (result))
			return ReleaseDirectInput();
	}
	int index = global.devcnt;
	while (index--)
	{
		const LPDIRECTINPUTDEVICE8 pDDevice = global.pDDevice[index];
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
		result = pDDevice->CreateEffect (GUID_Square , &local.eff, &global.pDEffect[index][0], NULL);
		if (FAILED (result))
			global.pDEffect[index][0] = NULL;

		/* Big Motor */
		local.eff.cbTypeSpecificParams = sizeof (DICONSTANTFORCE);
		local.eff.lpvTypeSpecificParams = &local.cf;
		result = pDDevice->CreateEffect (GUID_ConstantForce , &local.eff, &global.pDEffect[index][1], NULL);
		if (FAILED (result))
			global.pDEffect[index][1] = NULL;
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
	if (global.pDDevice[devno] == NULL)
		return FALSE;
	global.pDDevice[devno]->Poll();
	if (FAILED (global.pDDevice[devno]->GetDeviceState (sizeof (DIJOYSTATE), &global.JoyState[devno])))
	{
		AcquireDevice (global.pDDevice[devno]);
		return FALSE;
	}
	return TRUE;
}

static bool GetKeyState (u8* keyboard)
{
	InitDirectInput();
	if (global.pDKeyboard == NULL)
		return FALSE;
	global.pDKeyboard->Poll();
	if (FAILED (global.pDKeyboard->GetDeviceState (256, keyboard)))
	{
		AcquireDevice (global.pDKeyboard);
		return FALSE;
	}
	return TRUE;
}



static void SaveConfig (void)
{
	char Str_Tmp[1024];
	char Pad_Tmp[1024];
	for (int j = 0; j < 2; j++)
	{
		for(int i = 0; i < 21; i++)
		{
			sprintf(Str_Tmp, "%d",global.config.keys[j][i]);
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
		for(int i = 0; i < 21; i++)
		{			
			sprintf(Pad_Tmp, "PAD%d_%d",j, i);
			global.config.keys[j][i] = (u32)GetPrivateProfileInt("Controllers", Pad_Tmp, 0, Config.Conf_File);
		}
	}	
	global.padVibC[0] = global.padVibC[1] = -1;
	for (int cnt = 21; cnt--; )
	{
		const int key0 = global.config.keys[0][cnt];
		if (key0 >= 0x1000)
			global.padVibC[0] = (key0 & 0xfff) / 0x100;
		const int key1 = global.config.keys[1][cnt];
		if (key1 >= 0x1000)
			global.padVibC[1] = (key1 & 0xfff) / 0x100;
	}
}

static void PADsetMode (const int pad, const int mode)
{
	static const u8 padID[] = { 0x41, 0x73, 0x41, 0x79 };
	global.padMode1[pad] = mode;
	global.padVib0[pad] = 0;
	global.padVib1[pad] = 0;
	global.padVibF[pad][0] = 0;
	global.padVibF[pad][1] = 0;
	global.padID[pad] = padID[global.padMode2[pad] * 2 + mode];
}

static void KeyPress (const int pad, const int index, const bool press)
{
	if (index < 16)
	{
		if (press)
		{
			global.padStat[pad] &= ~(1 << index);
			if (global.padPress[pad][index] == 0)
				global.padPress[pad][index] = GetTickCount();
		}
		else
		{
			global.padStat[pad] |= 1 << index;
			global.padPress[pad][index] = 0;
		}
	}
	else
	{
		static bool prev[2] = { FALSE, FALSE };
		if ((prev[pad] != press) && (global.padModeF[pad] == 0))
		{
			prev[pad] = press;
			if (press) PADsetMode (pad, !global.padMode1[pad]);
		}
	}
}

static void UpdateState (const int pad)
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
		const int key = global.config.keys[pad][index];
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
				KeyPress (pad, index, global.JoyState[joypad].rgbButtons[key & 0xff]);
			}
			else if (key < 0x3000)
			{
				const int state = ((int*)&global.JoyState[joypad].lX)[(key & 0xff) /2];
				switch (key & 1)
				{
				case 0: KeyPress (pad, index, state < -64); break;
				case 1: KeyPress (pad, index, state >= 64); break;
				}
			}
			else
			{
				const u32 state = global.JoyState[joypad].rgdwPOV[(key & 0xff) /4];
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
	const int key = global.config.keys[pad][index];
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
	memset (&global, 0, sizeof (global));
	global.padStat[0] = 0xffff;
	global.padStat[1] = 0xffff;
	LoadConfig();
	PADsetMode (0, 0);
	PADsetMode (1, 0);	
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
	0xff, 0x5a, 0x03, 0x02, 0x01, 0x02, 0x01, 0x00,
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
	return (u8)(((int*)&global.JoyState[pad].lX)[pos] + 128);
}




long PAD1_readPort1(PadDataS* pads)
{	
	UpdateState(0);
	memset (pads, 0, sizeof (PadDataS));
	if ((global.padID[0] & 0xf0) == 0x40)
		pads->controllerType = 4;
	else
		pads->controllerType = 7;
	pads->buttonStatus = global.padStat[0];
	pads->leftJoyX = get_analog (global.config.keys[0][17]);
	pads->leftJoyY = get_analog (global.config.keys[0][18]);
	pads->rightJoyX = get_analog (global.config.keys[0][19]);
	pads->rightJoyY = get_analog (global.config.keys[0][20]);
	pads->moveX = 0;
	pads->moveY = 0;
	return 0;
}

long PAD2_readPort2(PadDataS* pads)
{	
	UpdateState(1);
	memset (pads, 0, sizeof (PadDataS));
	if ((global.padID[1] & 0xf0) == 0x40)
		pads->controllerType = 4;
	else
		pads->controllerType = 7;
	pads->buttonStatus = global.padStat[1];
	pads->leftJoyX = get_analog (global.config.keys[1][17]);
	pads->leftJoyY = get_analog (global.config.keys[1][18]);
	pads->rightJoyX = get_analog (global.config.keys[1][19]);
	pads->rightJoyY = get_analog (global.config.keys[1][20]);
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
			if ((GetTickCount() - countdown) / 1000 != 10)
			{
				char buff[64];
				wsprintf (buff, "Timeout: %d", 10 - (GetTickCount() - countdown) / 1000);
				SetWindowText (GetDlgItem (hWnd, IDC_ESELECT + index), buff);
			}
			else
			{
				global.config.keys[pad][index] = 0;
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
			for (cnt1 = global.devcnt; cnt1--;)
			{
				if (GetJoyState (cnt1) == FALSE)
					break;

				for (cnt2 = 32; cnt2--; )
				{
					if (global.JoyState[cnt1].rgbButtons[cnt2])
						key = 0x1000 + 0x100 * cnt1 + cnt2;
				}
				for (cnt2 = 8; cnt2--; )
				{
					const int now = ((u32*)&global.JoyState[cnt1].lX)[cnt2];
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
					const u32 now = global.JoyState[cnt1].rgdwPOV[cnt2];
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
					global.config.keys[pad][index] = key;
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
		memset (&global, 0, sizeof (global));
		if (DialogBox (gApp.hInstance, MAKEINTRESOURCE (IDD_CONFIGCONTROL), GetActiveWindow(), (DLGPROC)ConfigurePADDlgProc) == IDOK)
			SaveConfig();
		ReleaseDirectInput();	
}
