#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <zlib.h>

#include "../cdriso.h"
#include "resource.h"

#include "recentmenu.h"

HINSTANCE hInst;
#define MAXFILENAME 256

unsigned char Zbuf[CD_FRAMESIZE_RAW * 10 * 2];
HWND hDlg;
HWND hProgress;
HWND hIsoFile;
HWND hMethod;
int stop;

extern RecentMenu RecentCDs;
extern RecentMenu RecentMovies;

void SysMessage(char *fmt, ...);
//	va_list list;
//	char tmp[512];
//
//	va_start(list,fmt);
//	vsprintf(tmp,fmt,list);
//	va_end(list);
//	MessageBox(0, tmp, "cdriso Msg", 0);
//}

int _GetFile(char *out) {
	OPENFILENAME ofn;
	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= GetActiveWindow();
    ofn.lpstrFilter			= "Cd Iso Format (*.iso, *.bin, *.img)\0*.iso;*.bin;*.img;\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= MAXFILENAME;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= MAXFILENAME;
    ofn.lpstrTitle			= "PSXjin Open CD...";
    ofn.lpstrDefExt			= "ISO";
	char temp[2048];
	GetModuleFileName(0, temp, 2048);
	ofn.lpstrInitialDir = temp;
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		strcpy(out, szFileName);
		return 1;
	}

	return 0;
}

void CfgOpenFile() {
	if (_GetFile(IsoFile) == 1)
	{
		RecentCDs.UpdateRecentItems(IsoFile);
	}
}

void SysUpdate();

//void SysUpdate() {
//    MSG msg;
//
//	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
//		TranslateMessage(&msg);
//		DispatchMessage(&msg);
//	}
//}