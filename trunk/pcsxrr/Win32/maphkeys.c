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

static char* RealKeyName(int c)
{
	static char szString[64];
	char* szName = "";
	int i;

	for (i = 0; RealKeyNames[i].nCode; i++) {
		if (c == RealKeyNames[i].nCode) {
			if (RealKeyNames[i].szName) {
				szName = RealKeyNames[i].szName;
			}
			break;
		}
	}

	if (szName[0]) {
		sprintf(szString, "%s", szName);
	} else {
		sprintf(szString, "code 0x%.2X", c);
	}

	return szString;
}

// Update which command is using which key
static int MHkeysUseUpdate()
{
	if (hMHkeysList == NULL) {
		return 1;
	}
	unsigned int i;

	// Update the values of all the inputs
	for (i = 0; i < NUM_EMU_CMDS; i++) {
		LVITEM LvItem;

		if (!EmuCommandTable[i].key) {
			continue;
		}

		memset(&LvItem, 0, sizeof(LvItem));
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 1;
		LvItem.pszText = RealKeyName(EmuCommandTable[i].key);

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

// List item activated; find out which one
static int ListItemActivate()
{
	LVITEM LvItem;

	int nSel = SendMessage(hMHkeysList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (nSel < 0) {
		return 1;
	}

	// Get the corresponding input
	LvItem.mask = LVIF_PARAM;
	LvItem.iItem = nSel;
	LvItem.iSubItem = 0;
	SendMessage(hMHkeysList, LVM_GETITEM, 0, (LPARAM)&LvItem);
	nSel = LvItem.lParam;

	if (nSel >= (int)(NUM_EMU_CMDS)) {	// out of range
		return 1;
	}

	if (!EmuCommandTable[nSel].key) {
		return 1;
	}

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
