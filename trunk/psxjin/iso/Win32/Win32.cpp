#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <zlib.h>
#include <bzlib.h>

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

void UpdZmode() {
	if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_METHOD)) == 1)
		 Zmode = 1;
	else Zmode = 2;
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

void OnCompress() {
	struct stat buf;
	FILE *f;
	FILE *Z;
	unsigned long c=0, p=0, s;
	char table[256];
	int ret, blocks;

	Edit_GetText(hIsoFile, IsoFile, 256);

	cdHandle = fopen(IsoFile, "rb");
	if (cdHandle == NULL) {
		return;
	}
	stat(IsoFile, &buf);
	s = buf.st_size / CD_FRAMESIZE_RAW;

	UpdZmode();

	if (Zmode == 1) strcat(IsoFile, ".Z");
	else strcat(IsoFile, ".bz");
	Z = fopen(IsoFile, "wb");
	if (Z == NULL) {
		return;
	}

	strcpy(table, IsoFile);
	if (Zmode == 1) strcat(table, ".table");
	else strcat(table, ".index");

	f = fopen(table, "wb");
	if (f == NULL) {
		return;
	}

	if (Zmode == 1) {
		blocks = 1;
	} else {
		blocks = 10;
	}

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), FALSE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), FALSE);
	stop=0;

	while ((ret = fread(cdbuffer, 1, CD_FRAMESIZE_RAW * blocks, cdHandle))) {
		unsigned long size;
		long per;

		size = CD_FRAMESIZE_RAW * blocks * 2;
		if (Zmode == 1) compress(Zbuf, &size, cdbuffer, ret);
		else BZ2_bzBuffToBuffCompress((char*)Zbuf, (unsigned int*)&size, (char*)cdbuffer, ret, 1, 0, 30);

		fwrite(&c, 1, 4, f);
		if (Zmode == 1) fwrite(&size, 1, 2, f);

		fwrite(Zbuf, 1, size, Z);

		c+=size;
		p+=ret / CD_FRAMESIZE_RAW;

		per = ((p * 100) / s);
		SendMessage(hProgress, PBM_SETPOS, per, 0);
		SysUpdate();
		if (stop) break;
	}
	if (Zmode == 2) fwrite(&c, 1, 4, f);

	if (!stop) Edit_SetText(hIsoFile, IsoFile);

	fclose(cdHandle); cdHandle = NULL;
	fclose(f);
	fclose(Z);

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), TRUE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), TRUE);

	if (!stop) SysMessage("Iso Image Comompressed OK");
}

void OnDecompress() {
	struct stat buf;
	FILE *f;
	unsigned long c=0, p=0, s;
	char table[256];
	int blocks;

	Edit_GetText(hIsoFile, IsoFile, 256);

	UpdateZmode();
	if (Zmode == 0) Zmode = 2;

	strcpy(table, IsoFile);
	if (Zmode == 1) strcat(table, ".table");
	else strcat(table, ".index");

	if (stat(table, &buf) == -1) {
		return;
	}
	if (Zmode == 1) c = s = buf.st_size / 6;
	else c = s = (buf.st_size / 4) - 1;
	f = fopen(table, "rb");
	Ztable = (char*)malloc(buf.st_size);
	fread(Ztable, 1, buf.st_size, f);
	fclose(f);

	cdHandle = fopen(IsoFile, "rb");
	if (cdHandle == NULL) {
		return;
	}

	if (Zmode == 1) IsoFile[strlen(IsoFile) - 2] = 0;
	else IsoFile[strlen(IsoFile) - 3] = 0;

	f = fopen(IsoFile, "wb");
	if (f == NULL) {
		return;
	}

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), FALSE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), FALSE);
	stop=0;

	if (Zmode == 1) {
		blocks = 1;
	} else {
		blocks = 10;
	}

	while (c--) {
		unsigned long size, pos, ssize;
		long per;

		if (Zmode == 1) {
			pos = *(unsigned long*)&Ztable[p * 6];
			fseek(cdHandle, pos, SEEK_SET);

			ssize = *(unsigned short*)&Ztable[p * 6 + 4];
			fread(Zbuf, 1, ssize, cdHandle);
		} else {
			pos = *(unsigned long*)&Ztable[p * 4];
			fseek(cdHandle, pos, SEEK_SET);

			ssize = *(unsigned long*)&Ztable[p * 4 + 4] - pos;
			fread(Zbuf, 1, ssize, cdHandle);
		}

		size = CD_FRAMESIZE_RAW * blocks;
		if (Zmode == 1) uncompress(cdbuffer, &size, Zbuf, ssize);
		else BZ2_bzBuffToBuffDecompress((char*)cdbuffer, (unsigned int*)&size, (char*)Zbuf, ssize, 0, 0);

		fwrite(cdbuffer, 1, size, f);

		p++;

		per = ((p * 100) / s);
		SendMessage(hProgress, PBM_SETPOS, per, 0);
		SysUpdate();
		if (stop) break;
	}
	if (!stop) Edit_SetText(hIsoFile, IsoFile);

	fclose(f);
	fclose(cdHandle); cdHandle = NULL;
	free(Ztable); Ztable = NULL;

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), TRUE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), TRUE);

	if (!stop) SysMessage("Iso Image Decompressed OK");
}

static BOOL CALLBACK IsoConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int i;

	switch(uMsg) {
		case WM_INITDIALOG:
			hDlg = hW;
			hProgress = GetDlgItem(hW, IDC_PROGRESS);
			hIsoFile  = GetDlgItem(hW, IDC_ISOFILE);
			hMethod   = GetDlgItem(hW, IDC_METHOD);

			for (i=0; i<2; i++)
				ComboBox_AddString(hMethod, methods[i]);

			Edit_SetText(hIsoFile, IsoFile);
			if (strstr(IsoFile, ".Z") != NULL)
				 ComboBox_SetCurSel(hMethod, 1);
			else ComboBox_SetCurSel(hMethod, 0);

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_SELECTISO:
					if (_GetFile(IsoFile) == 1)
					{
						Edit_SetText(hIsoFile, IsoFile);
						RecentCDs.UpdateRecentItems(IsoFile);
					}
					return TRUE;

				case IDC_COMPRESSISO:
					OnCompress();
					return TRUE;

				case IDC_DECOMPRESSISO:
					OnDecompress();
					return TRUE;

				case IDC_STOP:
					stop = 1;
					return TRUE;

				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					Edit_GetText(hIsoFile, IsoFile, 256);
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

long CDRconfigure() {
/*
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ISOCONFIG),
              GetActiveWindow(),  
              (DLGPROC)IsoConfigureDlgProc);
*/
	return 0;
}
