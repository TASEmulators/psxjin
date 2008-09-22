#include "resource.h"
#include "PsxCommon.h"
#include "maphkeys.h"

#ifdef WIN32
#include "Win32.h"
#endif

extern AppData gApp;

HWND hMHkeysDlg = NULL;							// Handle to the MapHotkeys Dialog

struct EMUCMDTABLE EmuCommandTable[]=
{
	{ EMUCMD_PAUSE,VK_PAUSE,0,"Pause", },
	{ EMUCMD_TURBOMODE,VK_TAB,0,"Fast Forward", },
	{ EMUCMD_FRAMEADVANCE,VK_OEM_5,0,"Frame Advance", },
	{ EMUCMD_RWTOGGLE,'8',VK_SHIFT,"Read-Only Toggle", },
	{ EMUCMD_SPEEDDEC,VK_OEM_MINUS,VK_SHIFT,"Decrease Speed", },
	{ EMUCMD_SPEEDINC,VK_OEM_PLUS,VK_SHIFT,"Increase Speed", },
	{ EMUCMD_FRAMECOUNTER,VK_BACK,VK_SHIFT,"Frame Counter", },
	{ EMUCMD_LOADSTATE1,VK_F1,0,"Load State 1", },
	{ EMUCMD_LOADSTATE2,VK_F2,0,"Load State 2", },
	{ EMUCMD_LOADSTATE3,VK_F3,0,"Load State 3", },
	{ EMUCMD_LOADSTATE4,VK_F4,0,"Load State 4", },
	{ EMUCMD_LOADSTATE5,VK_F5,0,"Load State 5", },
	{ EMUCMD_LOADSTATE6,VK_F6,0,"Load State 6", },
	{ EMUCMD_LOADSTATE7,VK_F7,0,"Load State 7", },
	{ EMUCMD_LOADSTATE8,VK_F8,0,"Load State 8", },
	{ EMUCMD_LOADSTATE9,VK_F9,0,"Load State 9", },
	{ EMUCMD_SAVESTATE1,VK_F1,VK_SHIFT,"Save State 1", },
	{ EMUCMD_SAVESTATE2,VK_F2,VK_SHIFT,"Save State 2", },
	{ EMUCMD_SAVESTATE3,VK_F3,VK_SHIFT,"Save State 3", },
	{ EMUCMD_SAVESTATE4,VK_F4,VK_SHIFT,"Save State 4", },
	{ EMUCMD_SAVESTATE5,VK_F5,VK_SHIFT,"Save State 5", },
	{ EMUCMD_SAVESTATE6,VK_F6,VK_SHIFT,"Save State 6", },
	{ EMUCMD_SAVESTATE7,VK_F7,VK_SHIFT,"Save State 7", },
	{ EMUCMD_SAVESTATE8,VK_F8,VK_SHIFT,"Save State 8", },
	{ EMUCMD_SAVESTATE9,VK_F9,VK_SHIFT,"Save State 9", }
};
#define NUM_EMU_CMDS (sizeof(EmuCommandTable)/sizeof(EmuCommandTable[0]))

static HWND hMHkeysList = NULL;
static unsigned char *LastVal = NULL;			// Last input values/defined
static int bLastValDefined = 0;					//

static HHOOK hook;
static int receivingKmap;

static char* RealKeyName(int c)
{
	static char out[255];

	sprintf(out,GAMEDEVICE_KEY,c);
	if((c>='0' && c<='9')||(c>='A' &&c<='Z'))
	{
		sprintf(out,"%c",c);
		return out;
	}
	if( c >= VK_NUMPAD0 && c <= VK_NUMPAD9)
	{
		sprintf(out,GAMEDEVICE_NUMPADPREFIX,'0'+(c-VK_NUMPAD0));
		return out;
	}
	switch(c)
	{
		case VK_TAB:        sprintf(out,GAMEDEVICE_VK_TAB); break;
		case VK_BACK:       sprintf(out,GAMEDEVICE_VK_BACK); break;
		case VK_CLEAR:      sprintf(out,GAMEDEVICE_VK_CLEAR); break;
		case VK_RETURN:     sprintf(out,GAMEDEVICE_VK_RETURN); break;
		case VK_LSHIFT:     sprintf(out,GAMEDEVICE_VK_LSHIFT); break;
		case VK_RSHIFT:     sprintf(out,GAMEDEVICE_VK_RSHIFT); break;
		case VK_LCONTROL:   sprintf(out,GAMEDEVICE_VK_LCONTROL); break;
		case VK_RCONTROL:   sprintf(out,GAMEDEVICE_VK_RCONTROL); break;
		case VK_LMENU:      sprintf(out,GAMEDEVICE_VK_LMENU); break;
		case VK_RMENU:      sprintf(out,GAMEDEVICE_VK_RMENU); break;
		case 3:             sprintf(out,GAMEDEVICE_VK_PAUSE); break;
		case VK_PAUSE:      sprintf(out,GAMEDEVICE_VK_PAUSE); break;
		case VK_CAPITAL:    sprintf(out,GAMEDEVICE_VK_CAPITAL); break;
		case VK_ESCAPE:     sprintf(out,GAMEDEVICE_VK_ESCAPE); break;
		case VK_SPACE:      sprintf(out,GAMEDEVICE_VK_SPACE); break;
		case VK_PRIOR:      sprintf(out,GAMEDEVICE_VK_PRIOR); break;
		case VK_NEXT:       sprintf(out,GAMEDEVICE_VK_NEXT); break;
		case VK_HOME:       sprintf(out,GAMEDEVICE_VK_HOME); break;
		case VK_END:        sprintf(out,GAMEDEVICE_VK_END); break;
		case VK_LEFT:       sprintf(out,GAMEDEVICE_VK_LEFT ); break;
		case VK_RIGHT:      sprintf(out,GAMEDEVICE_VK_RIGHT); break;
		case VK_UP:         sprintf(out,GAMEDEVICE_VK_UP); break;
		case VK_DOWN:       sprintf(out,GAMEDEVICE_VK_DOWN); break;
		case VK_SELECT:     sprintf(out,GAMEDEVICE_VK_SELECT); break;
		case VK_PRINT:      sprintf(out,GAMEDEVICE_VK_PRINT); break;
		case VK_EXECUTE:    sprintf(out,GAMEDEVICE_VK_EXECUTE); break;
		case VK_SNAPSHOT:   sprintf(out,GAMEDEVICE_VK_SNAPSHOT); break;
		case VK_INSERT:     sprintf(out,GAMEDEVICE_VK_INSERT); break;
		case VK_DELETE:     sprintf(out,GAMEDEVICE_VK_DELETE); break;
		case VK_HELP:       sprintf(out,GAMEDEVICE_VK_HELP); break;
		case VK_LWIN:       sprintf(out,GAMEDEVICE_VK_LWIN); break;
		case VK_RWIN:       sprintf(out,GAMEDEVICE_VK_RWIN); break;
		case VK_APPS:       sprintf(out,GAMEDEVICE_VK_APPS); break;
		case VK_MULTIPLY:   sprintf(out,GAMEDEVICE_VK_MULTIPLY); break;
		case VK_ADD:        sprintf(out,GAMEDEVICE_VK_ADD); break;
		case VK_SEPARATOR:  sprintf(out,GAMEDEVICE_VK_SEPARATOR); break;
		case VK_OEM_1:      sprintf(out,GAMEDEVICE_VK_OEM_1); break;
		case VK_OEM_7:      sprintf(out,GAMEDEVICE_VK_OEM_7); break;
		case VK_OEM_COMMA:  sprintf(out,GAMEDEVICE_VK_OEM_COMMA );break;
		case VK_OEM_PERIOD: sprintf(out,GAMEDEVICE_VK_OEM_PERIOD);break;
		case VK_SUBTRACT:   sprintf(out,GAMEDEVICE_VK_SUBTRACT); break;
		case VK_DECIMAL:    sprintf(out,GAMEDEVICE_VK_DECIMAL); break;
		case VK_DIVIDE:     sprintf(out,GAMEDEVICE_VK_DIVIDE); break;
		case VK_NUMLOCK:    sprintf(out,GAMEDEVICE_VK_NUMLOCK); break;
		case VK_SCROLL:     sprintf(out,GAMEDEVICE_VK_SCROLL); break;
		case 189:           sprintf(out,"-"); break;
		case 187:           sprintf(out,"="); break;
		case 16:            sprintf(out,"Shift"); break;
		case 17:            sprintf(out,"Control"); break;
		case 18:            sprintf(out,"Alt"); break;
		case 219:           sprintf(out,"["); break;
		case 221:           sprintf(out,"]"); break;
		case 220:           sprintf(out,"\\"); break;
		case 191:           sprintf(out,"/"); break;
		case 192:           sprintf(out,"`"); break;
		case 112:           sprintf(out,"F1"); break;
		case 113:           sprintf(out,"F2"); break;
		case 114:           sprintf(out,"F3"); break;
		case 115:           sprintf(out,"F4"); break;
		case 116:           sprintf(out,"F5"); break;
		case 117:           sprintf(out,"F6"); break;
		case 118:           sprintf(out,"F7"); break;
		case 119:           sprintf(out,"F8"); break;
		case 120:           sprintf(out,"F9"); break;
		case 121:           sprintf(out,"F10"); break;
		case 122:           sprintf(out,"F11"); break;
		case 123:           sprintf(out,"F12"); break;
	}

	return out;
}

// Update which command is using which key
static int MHkeysUseUpdate()
{
	if (hMHkeysList == NULL) {
		return 1;
	}
	char tempTxt[255];
	unsigned int i;

	// Update the values of all the inputs
	for (i = 0; i < NUM_EMU_CMDS; i++) {
		LVITEM LvItem;

		if (!EmuCommandTable[i].key) {
			continue;
		}

		sprintf(tempTxt, "");
		if(EmuCommandTable[i].keymod == VK_CONTROL)
			sprintf(tempTxt,"Ctrl + ");
		else if(EmuCommandTable[i].keymod == VK_MENU)
			sprintf(tempTxt,"Alt + ");
		else if(EmuCommandTable[i].keymod == VK_SHIFT)
			sprintf(tempTxt,"Shift + ");
	
		sprintf(tempTxt,"%s%s",tempTxt,RealKeyName(EmuCommandTable[i].key));

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 1;
		LvItem.pszText = tempTxt;

		SendMessage(hMHkeysList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
	return 0;
}

static int MHkeysListBegin()
{
	LVCOLUMN LvCol;
	if (hMHkeysList == NULL) {
		return 1;
	}

	// Full row select style:
	SendMessage(hMHkeysList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// Make column headers
	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	LvCol.cx = 0xA3;
	LvCol.pszText = "Command";
	SendMessage(hMHkeysList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.cx = 0xA3;
	LvCol.pszText = "Mapped to";
	SendMessage(hMHkeysList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

	return 0;
}

// Make a list view of the game inputs
int MHkeysListMake(int bBuild)
{
	if (hMHkeysList == NULL) {
		return 1;
	}

	bLastValDefined = 0;
	if (bBuild)	{
		SendMessage(hMHkeysList, LVM_DELETEALLITEMS, 0, 0);
	}

	unsigned int i;
	// Add all the input names to the list
	for (i = 0; i < NUM_EMU_CMDS; i++) {
		LVITEM LvItem;

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT | LVIF_PARAM;
		LvItem.iItem = i;
		LvItem.iSubItem = 0;
		LvItem.pszText = EmuCommandTable[i].name;
		LvItem.lParam = (LPARAM)i;

		SendMessage(hMHkeysList, bBuild ? LVM_INSERTITEM : LVM_SETITEM, 0, (LPARAM)&LvItem);
	}

	MHkeysUseUpdate();

	return 0;
}

static int MHkeysInit()
{
	int nMemLen;

	hMHkeysList = GetDlgItem(hMHkeysDlg, IDC_MHKEYS_LIST);

	// Allocate a last val array for the last input values
	nMemLen = NUM_EMU_CMDS * sizeof(char);
	LastVal = (unsigned char*)malloc(nMemLen);
	if (LastVal == NULL) {
		return 1;
	}
	memset(LastVal, 0, nMemLen);

	MHkeysListBegin();
	MHkeysListMake(1);

	return 0;
}

static int MHkeysExit()
{
	if (LastVal != NULL) {
		free(LastVal);
		LastVal = NULL;
	}
	hMHkeysList = NULL;
	hMHkeysDlg = NULL;

	return 0;
}

static LRESULT CALLBACK KeyMappingHook(int code, WPARAM wParam, LPARAM lParam) {
	if (code < 0) {
		return CallNextHookEx(hook, code, wParam, lParam);
	}
	if(wParam == VK_SHIFT || wParam == VK_MENU || wParam == VK_CONTROL) {
		return CallNextHookEx(hook, code, wParam, lParam);
	}

//	if(wParam == VK_ESCAPE)
//	{
//		TranslateKey(wParam,temp);
//	}
//	else
//	{
	if(GetAsyncKeyState(VK_CONTROL))
		EmuCommandTable[receivingKmap].keymod = VK_CONTROL;
	else if(GetAsyncKeyState(VK_MENU))
		EmuCommandTable[receivingKmap].keymod = VK_MENU;
	else if(GetAsyncKeyState(VK_SHIFT))
		EmuCommandTable[receivingKmap].keymod = VK_SHIFT;
	else
		EmuCommandTable[receivingKmap].keymod = 0;
//	}

	char keyNameBuf[16];
	unsigned short key = wParam;

////	NewKeyMappings[receivingKmap] = key;

	snprintf(keyNameBuf, 16, "0x%02x", key);
//	SetDlgItemText(hwndKeyDlg, ID_LAB_KMAP(receivingKmap), keyNameBuf);
//
//	EnableAll(TRUE);

	static HWND statusText;
	statusText = GetDlgItem(hMHkeysDlg, IDC_HKEYSS_STATIC);
	SetWindowText(statusText, keyNameBuf);
	EmuCommandTable[receivingKmap].key = key;
	MHkeysUseUpdate();

	UnhookWindowsHookEx(hook);
	hook = 0;

	return 1;
}

// List item activated; find out which one
static int ListItemActivate()
{
	char str [256];
	int nSel = SendMessage(hMHkeysList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	static HWND statusText;
	statusText = GetDlgItem(hMHkeysDlg, IDC_HKEYSS_STATIC);

	sprintf(str, "SETTING KEY: %d", nSel);
	SetWindowText(statusText, str);
	receivingKmap = nSel;
	hook = SetWindowsHookEx(WH_KEYBOARD, KeyMappingHook, 0, GetCurrentThreadId());

	return 0;
}

static BOOL CALLBACK DialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_INITDIALOG) {
		hMHkeysDlg = hDlg;
		MHkeysInit();
		
		return TRUE;
	}

	if (Msg == WM_CLOSE) {
		EnableWindow(gApp.hWnd, TRUE);
		DestroyWindow(hMHkeysDlg);
		return 0;
	}

	if (Msg == WM_DESTROY) {
		MHkeysExit();
		return 0;
	}

	if (Msg == WM_COMMAND) {
		int Id = LOWORD(wParam);
		int Notify = HIWORD(wParam);

		if (Id == IDOK && Notify == BN_CLICKED) {
			ListItemActivate();
			return 0;
		}
		if (Id == IDCANCEL && Notify == BN_CLICKED) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return 0;
		}

	}

	if (Msg == WM_NOTIFY && lParam != 0) {
		int Id = LOWORD(wParam);
		NMHDR* pnm = (NMHDR*)lParam;

		if (Id == IDC_MHKEYS_LIST && pnm->code == LVN_ITEMACTIVATE) {
			ListItemActivate();
		}
		if (Id == IDC_MHKEYS_LIST && pnm->code == LVN_KEYDOWN) {
			NMLVKEYDOWN *pnmkd = (NMLVKEYDOWN*)lParam;
			if (pnmkd->wVKey == VK_DELETE) {
//				ListItemDelete();
			}
		}

		return 0;
	}

	return 0;
}

int MHkeysCreate()
{
	DestroyWindow(hMHkeysDlg);										// Make sure exitted

	hMHkeysDlg = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_MHKEYS), gApp.hWnd, DialogProc);
	if (hMHkeysDlg == NULL) {
		return 1;
	}

	ShowWindow(hMHkeysDlg, SW_NORMAL);
	
	return 0;
}
