#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include "../PsxCommon.h"
#include "resource.h"
#include "Win32.h"
#include "../movie.h"

//------------------------------------------------------

char szFilter[1024];
char szChoice[256];
OPENFILENAME ofn;
extern AppData gApp;

static void MakeOfn(char* pszFilter)
{
	sprintf(pszFilter, "%s Input Recording Files", "PSXJIN");
	memcpy(pszFilter + strlen(pszFilter), " (*.pjm)\0*.pjm\0\0", 14 * sizeof(char));

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gApp.hWnd;
	ofn.lpstrFilter = pszFilter;
	ofn.lpstrFile = szChoice;
	ofn.nMaxFile = sizeof(szChoice) / sizeof(char);
	ofn.lpstrInitialDir = Config.MovieDir;
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pjm";

	return;
}

void GetMovieFilenameMini(char* filenameMini)
{
	char fszDrive[256];
	char fszDirectory[256];
	char fszFilename[256];
	char fszExt[256];
	fszDrive[0] = '\0';
	fszDirectory[0] = '\0';
	fszFilename[0] = '\0';
	fszExt[0] = '\0';
	_splitpath(filenameMini, fszDrive, fszDirectory, fszFilename, fszExt);

	strncpy(Movie.movieFilenameMini, fszFilename, 256);
}

static char* GetRecordingPath(char* szPath)
{
	//this function makes sure a relative path has the "movies\" path prepended to it
	char szDrive[256];
	char szDirectory[256];
	char szFilename[256];
	char szExt[256];
	szDrive[0] = '\0';
	szDirectory[0] = '\0';
	szFilename[0] = '\0';
	szExt[0] = '\0';
	_splitpath(szPath, szDrive, szDirectory, szFilename, szExt);
	if (szDrive[0] == '\0' && szDirectory[0] == '\0') {
		char szTmpPath[256];
		sprintf(szTmpPath, "%smovies\\", szCurrentPath);
		strncpy(szTmpPath + strlen(szTmpPath), szPath, 256 - strlen(szTmpPath));
		szTmpPath[255] = '\0';
		strncpy(szPath, szTmpPath, 256);
	}

	return szPath;
}

static void DisplayReplayProperties(HWND hDlg, int bClear)
{
	struct MovieType dataMovie;
	long lCount;
	long lIndex;
	long lStringLength;
	int nFPS;
	int nSeconds;
	int nMinutes;
	int nHours;
	char szFramesString[32];
	char szLengthString[32];
	char szUndoCountString[32];
	char szPSXjinVersion[32];
	char szStartFrom[128];
	char szUsedCdrom[10];
	char szExtras[128];

	// set default values
	SetDlgItemTextA(hDlg, IDC_LENGTH, "");
	SetDlgItemTextA(hDlg, IDC_FRAMES, "");
	SetDlgItemTextA(hDlg, IDC_UNDO, "");
	SetDlgItemTextA(hDlg, IDC_EMUVERSION, "");
	SetDlgItemTextA(hDlg, IDC_REPLAYRESET, "");
	SetDlgItemTextA(hDlg, IDC_USEDCDROM, "");
	SetDlgItemTextA(hDlg, IDC_CURRENTCDROM, "");
	SetDlgItemTextA(hDlg, IDC_PADTYPE1, "");
	SetDlgItemTextA(hDlg, IDC_PADTYPE2, "");
	SetDlgItemTextA(hDlg, IDC_USECHEATS, "");
	SetDlgItemTextA(hDlg, IDC_METADATA, "");
	EnableWindow(GetDlgItem(hDlg, IDC_READONLY), FALSE);
	SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_UNCHECKED, 0);
	EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
	if(bClear)
		return;

	lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
	lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
	if (lIndex == CB_ERR)
		return;

	if (lIndex == lCount - 1) {                   // Last item is "Browse..."
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE); // Browse is selectable
		return;
	}

	lStringLength = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETLBTEXTLEN, (WPARAM)lIndex, 0);
	if(lStringLength + 1 > 256)
		return;

	// save movie filename in szChoice
	SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)szChoice);

	// ensure a relative path has the "movies\" path in prepended to it
	GetRecordingPath(szChoice);

	if (! MOV_ReadMovieFile(szChoice,&dataMovie) ) {
		return;
	}

//	EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
//	return;

	GetMovieFilenameMini(dataMovie.movieFilename);

	if (_access(szChoice, W_OK)) // is the file read-only?
		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_CHECKED, 0);
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_READONLY), TRUE);
//		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, (dataMovie.readOnly) ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_CHECKED, 0);
	}

	// file exists and it has correct format
	// so enable the "Ok" button
	EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);

	// turn totalFrames into a length string
	if (dataMovie.palTiming)
		nFPS = 50;
	else
		nFPS = 60;
	nSeconds = dataMovie.totalFrames / nFPS;
	nMinutes = nSeconds / 60;
	nHours = nSeconds / 3600;

	//format strings
	sprintf(szFramesString, "%lu", dataMovie.totalFrames);
	sprintf(szLengthString, "%02d:%02d:%02d", nHours, nMinutes % 60, nSeconds % 60);
	sprintf(szUndoCountString, "%lu", dataMovie.rerecordCount);
	sprintf(szPSXjinVersion,"%3.3lu",dataMovie.emuVersion);
	sprintf(szPSXjinVersion,"PSXJIN v%c.%c.%c",szPSXjinVersion[0],szPSXjinVersion[1],szPSXjinVersion[2]);

	//write strings to dialog
	SetDlgItemTextA(hDlg, IDC_LENGTH, szLengthString);
	SetDlgItemTextA(hDlg, IDC_FRAMES, szFramesString);
	SetDlgItemTextA(hDlg, IDC_UNDO, szUndoCountString);
	SetDlgItemTextA(hDlg, IDC_EMUVERSION, szPSXjinVersion);
	SetDlgItemTextA(hDlg, IDC_METADATA, dataMovie.authorInfo);

	//start from?
	if (!dataMovie.saveStateIncluded)
		_snprintf(szStartFrom, 128, "Power-On");
	else
		_snprintf(szStartFrom, 128, "Savestate");
	if (dataMovie.memoryCardIncluded)
		_snprintf(szStartFrom, 128, "%s + Memory Cards",szStartFrom);
	SetDlgItemTextA(hDlg, IDC_REPLAYRESET, szStartFrom);

	//cd-rom ids
	sprintf(szUsedCdrom, "%9.9s", dataMovie.CdromIds);
	SetDlgItemTextA(hDlg, IDC_USEDCDROM, szUsedCdrom);

	SetDlgItemTextA(hDlg, IDC_CURRENTCDROM, CdromId);

	//cheats? hacks?
	if (dataMovie.cheatListIncluded && dataMovie.irqHacksIncluded)
		_snprintf(szExtras, 128, "Cheats + Emulation Hacks");
	else if (dataMovie.cheatListIncluded)
		_snprintf(szExtras, 128, "Cheats");
	else if (dataMovie.irqHacksIncluded)
		_snprintf(szExtras, 128, "Emulation Hacks");
	else
		_snprintf(szExtras, 128, "None");
	SetDlgItemTextA(hDlg, IDC_USECHEATS, szExtras);

	switch (dataMovie.padType1) {
		case PSE_PAD_TYPE_NONE:
			SetDlgItemTextA(hDlg, IDC_PADTYPE1, "None");
			break;
		case PSE_PAD_TYPE_MOUSE:
			SetDlgItemTextA(hDlg, IDC_PADTYPE1, "Mouse");
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			SetDlgItemTextA(hDlg, IDC_PADTYPE1, "Dual Analog");
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			SetDlgItemTextA(hDlg, IDC_PADTYPE1, "Analog Joystick");
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			SetDlgItemTextA(hDlg, IDC_PADTYPE1, "Standard");
	}
	switch (dataMovie.padType2) {
		case PSE_PAD_TYPE_NONE:
			SetDlgItemTextA(hDlg, IDC_PADTYPE2, "None");
			break;
		case PSE_PAD_TYPE_MOUSE:
			SetDlgItemTextA(hDlg, IDC_PADTYPE2, "Mouse");
			break;
		case PSE_PAD_TYPE_ANALOGPAD: // scph1150
			SetDlgItemTextA(hDlg, IDC_PADTYPE2, "Dual Analog");
			break;
		case PSE_PAD_TYPE_ANALOGJOY: // scph1110
			SetDlgItemTextA(hDlg, IDC_PADTYPE2, "Analog Joystick");
			break;
		case PSE_PAD_TYPE_STANDARD:
		default:
			SetDlgItemTextA(hDlg, IDC_PADTYPE2, "Standard");
	}
}

static BOOL CALLBACK ReplayDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int nRet;
	if (Msg == WM_INITDIALOG) {
		char szFindPath[256];
		WIN32_FIND_DATA wfd;
		HANDLE hFind;
		int i = 0;

		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_UNCHECKED, 0);

		memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
//		if (bDrvOkay) {
//			_stprintf(szFindPath, _T("movies\\%.8s*.pjm"), BurnDrvGetText(DRV_NAME));
//		}
		sprintf(szFindPath, "%smovies\\*.pjm", szCurrentPath);

		hFind = FindFirstFile(szFindPath, &wfd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do
			{
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_INSERTSTRING, i++, (LPARAM)wfd.cFileName);

			} while(FindNextFile(hFind, &wfd));
			FindClose(hFind);
		}
		SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_SETCURSEL, i-1, 0);
		SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_INSERTSTRING, i++, (LPARAM)"Browse...");

		if (i>1)
			DisplayReplayProperties(hDlg, 0);

		SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_LIST));
		return FALSE;
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			LONG lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR)
				DisplayReplayProperties(hDlg, (lIndex == lCount - 1));		// Selecting "Browse..." will clear the replay properties display
		} else if (HIWORD(wParam) == CBN_CLOSEUP) {
			LONG lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR) {
				if (lIndex == lCount - 1) // send an OK notification to open the file browser
					SendMessage(hDlg, WM_COMMAND, (WPARAM)IDOK, 0);
			}
		} else {
			int wID = LOWORD(wParam);
			switch (wID) {
				case IDOK:
					{
						LONG lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
						LONG lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
						if (lIndex != CB_ERR) {
							if (lIndex == lCount - 1) {
								MakeOfn(szFilter);
								ofn.lpstrTitle = "Replay Input from File";
								ofn.Flags &= ~OFN_HIDEREADONLY;

								nRet = GetOpenFileName(&ofn);
								if (nRet != 0) {
									LONG lOtherIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_FINDSTRING, (WPARAM)-1, (LPARAM)szChoice);
									if (lOtherIndex != CB_ERR) {
										// select already existing string
										SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_SETCURSEL, lOtherIndex, 0);
									} else {
										SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_INSERTSTRING, lIndex, (LPARAM)szChoice);
										SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_SETCURSEL, lIndex, 0);
									}
									// restore focus to the dialog
									SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_LIST));
									DisplayReplayProperties(hDlg, 0);
									if (ofn.Flags & OFN_READONLY) {
										SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_CHECKED, 0);
									} else {
										SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_UNCHECKED, 0);
									}
								}
							} else {
								MOV_ReadMovieFile(szChoice,&Movie);
								// get readonly status
								Movie.readOnly = 0;
								if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_READONLY, BM_GETCHECK, 0, 0))
									Movie.readOnly = 1;
								EndDialog(hDlg, 1);					// only allow OK if a valid selection was made
							}
						}
					}
					return TRUE;
				case IDCANCEL:
					szChoice[0] = '\0';
					EndDialog(hDlg, 0);
					return TRUE;
			}
		}
	}

	return FALSE;
}

int MOV_W32_StartReplayDialog()
{
	return DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_REPLAYINP), gApp.hWnd, ReplayDialogProc);
}

static int VerifyRecordingAccessMode(char* szFilename, int mode)
{
	GetRecordingPath(szFilename);
	if(_access(szFilename, mode))
		return 0; // not writeable, return failure

	return 1;
}

static void VerifyRecordingFilename(HWND hDlg)
{
	char szFilename[256];
	GetDlgItemText(hDlg, IDC_FILENAME, szFilename, 256);

	// if filename null, or, file exists and is not writeable
	// then disable the dialog controls
	if(szFilename[0] == '\0' ||
		(VerifyRecordingAccessMode(szFilename, 0) != 0 && VerifyRecordingAccessMode(szFilename, W_OK) == 0)) {
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_METADATA), FALSE);
	} else {
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_METADATA), TRUE);
	}
}

//adelikat: This is probably more useful in Wndmain + header
//Strips path and extension off IsoFile and returns the result
std::string MakeMovieName()
{
	std::string str;
	if (IsoFile[0])
	{
		str = IsoFile;
		int x = str.find_last_of('\\') + 1;
		str = str.substr(x, str.length() - x - 4);
	}
	else
	{
		str = "movie";
	}

	return str;
}
char defaultFilename[1024];
static BOOL CALLBACK RecordDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int nRet;
	LONG lIndex;
	
	if (Msg == WM_INITDIALOG) {
		// come up with a unique name
		char szPath[256];
		char szFilename[256];
		int i = 0;
		strncpy(defaultFilename, MakeMovieName().c_str(), 1024);
		sprintf(szFilename, "%.252s.pjm", defaultFilename);
		strncpy(szPath, szFilename, 256);
		while(VerifyRecordingAccessMode(szPath, 0) == 1) {
			sprintf(szFilename, "%.252s-%d.pjm", defaultFilename, ++i);
			strncpy(szPath, szFilename, 256);
		}

		SetDlgItemText(hDlg, IDC_FILENAME, szFilename);
		SetDlgItemText(hDlg, IDC_METADATA, "");
//		CheckDlgButton(hDlg, IDC_REPLAYRESET, BST_CHECKED);

		SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_INSERTSTRING, -1, (LPARAM)"Power-On");
		SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_INSERTSTRING, -1, (LPARAM)"Power-On + Current Memory Cards");
		SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_INSERTSTRING, -1, (LPARAM)"Current State");
		SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_INSERTSTRING, -1, (LPARAM)"Current State + Current Memory Cards");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Standard");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Dual Analog");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Mouse");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"None");

		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Standard");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Dual Analog");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Mouse");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"None");
		
		SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_SETCURSEL, 0, 0);

		VerifyRecordingFilename(hDlg);

		SetFocus(GetDlgItem(hDlg, IDC_METADATA));
		return FALSE;
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE) {
			VerifyRecordingFilename(hDlg);
		}
		else {
			int wID = LOWORD(wParam);
			switch (wID) {
				case IDC_BROWSE:
				{
					sprintf(szChoice, "%.64s", defaultFilename);
					MakeOfn(szFilter);
					ofn.lpstrTitle = "Record Input to File";
					ofn.Flags |= OFN_OVERWRITEPROMPT;
					nRet = GetSaveFileName(&ofn);
					if (nRet != 0) //this triggers an EN_CHANGE message
						SetDlgItemText(hDlg, IDC_FILENAME, szChoice);
					return TRUE;
				}
				case IDOK:
				{
					//save movie filename stuff
					GetDlgItemText(hDlg, IDC_FILENAME, szChoice, 256);
					strncpy(Movie.movieFilename,GetRecordingPath(szChoice),256);
					GetMovieFilenameMini(Movie.movieFilename);

					//save author info
					GetDlgItemText(hDlg, IDC_METADATA, Movie.authorInfo, MOVIE_MAX_METADATA);
					Movie.authorInfo[MOVIE_MAX_METADATA-1] = '\0';					
					Movie.NumPlayers = 2;
					Movie.P2_Start = 2;
					if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_USE_BINARY, BM_GETCHECK, 0, 0))
						Movie.isText = 0;
					else
						Movie.isText = 1;
					if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_PORT1_MTAP, BM_GETCHECK, 0, 0))
					{					
						Movie.Port1_Mtap = 1;
						Movie.NumPlayers +=3;
						Movie.P2_Start += 3;
						Movie.MultiTrack =1;						
					}
					else
						Movie.Port1_Mtap = 0;
					if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_PORT2_MTAP, BM_GETCHECK, 0, 0))
					{						
						Movie.Port2_Mtap = 1;
						Movie.NumPlayers +=3;
						Movie.MultiTrack =1;
					}
					else
						Movie.Port2_Mtap = 0;

					//save cheat list checkbox
					Movie.cheatListIncluded = 0;
					if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_USECHEATS, BM_GETCHECK, 0, 0))
						Movie.cheatListIncluded = 1;					

					//save "start from" option list
					lIndex = SendDlgItemMessage(hDlg, IDC_REPLAYRESET, CB_GETCURSEL, 0, 0);
					switch (lIndex) {
						case 3:
							Movie.saveStateIncluded = 1;
							Movie.memoryCardIncluded = 1;
							break;
						case 2:
							Movie.saveStateIncluded = 1;
							Movie.memoryCardIncluded = 0;
							break;
						case 1:
							Movie.saveStateIncluded = 0;
							Movie.memoryCardIncluded = 1;
							break;
						case 0:
						default:
							Movie.saveStateIncluded = 0;
							Movie.memoryCardIncluded = 0;
					}

					//save "joypad type" option lists
					lIndex = SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_GETCURSEL, 0, 0);
					switch (lIndex) {
						case 3:											
							Movie.padType1 = PSE_PAD_TYPE_NONE;
							Movie.NumPlayers--;
							Movie.P2_Start = 1;
							break;
						case 2:
							Movie.padType1=PSE_PAD_TYPE_MOUSE;
							break;
						case 1:
							Movie.padType1=PSE_PAD_TYPE_ANALOGPAD;
							break;
						case 0:
						default:
							Movie.padType1=PSE_PAD_TYPE_STANDARD;
							break;
					}
					lIndex = SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_GETCURSEL, 0, 0);
					switch (lIndex) {
						case 3:
								Movie.NumPlayers--;														
								Movie.padType2 = PSE_PAD_TYPE_NONE;
								break;
						case 2:
							Movie.padType2=PSE_PAD_TYPE_MOUSE;
							break;
						case 1:
							Movie.padType2=PSE_PAD_TYPE_ANALOGPAD;
							break;						
						case 0:
						default:
							Movie.padType2=PSE_PAD_TYPE_STANDARD;
					}					
					Movie.RecordPlayer = Movie.NumPlayers+1;
					EndDialog(hDlg, 1);
					return TRUE;
				}
				case IDCANCEL:
				{
					szChoice[0] = '\0';
					EndDialog(hDlg, 0);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

int MOV_W32_StartRecordDialog()
{
	return DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RECORDINP), gApp.hWnd, RecordDialogProc);
}

static BOOL CALLBACK CdChangeDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_INITDIALOG && Movie.mode == MOVIEMODE_PLAY) {
		char tmpText[128];
		char tmpCdromId[20];
		memcpy(tmpCdromId,Movie.CdromIds+((Movie.currentCdrom-1)*9),9);
		sprintf(tmpText, "Please insert Disc %d (%9.9s).", Movie.currentCdrom, tmpCdromId);
		SetDlgItemTextA(hDlg, IDC_FILENAME, tmpText);
	}
	if (Msg == WM_COMMAND && LOWORD(wParam) == IDOK) {
		EndDialog(hDlg, 1);
		return TRUE;
	}
	return FALSE;
}

int MOV_W32_CdChangeDialog()
{
	return DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CDCHANGE), gApp.hWnd, CdChangeDialogProc);
}

LRESULT CALLBACK PromptAuthorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char Str_Tmp[512];
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			GetWindowRect(gApp.hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowText(hDlg, "Change author");		
			strcpy(Str_Tmp,"Enter an author name for this movie.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			SendDlgItemMessage(hDlg,IDC_PROMPT_EDIT, WM_SETTEXT,0, (LPARAM) Movie.authorInfo);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,512);
					ChangeAuthor(Str_Tmp);
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case IDCANCEL:
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}

LRESULT CALLBACK PromptRerecordProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char Str_Tmp[512];
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	sprintf(Str_Tmp,"%d",Movie.rerecordCount);
	switch(uMsg)
	{
		case WM_INITDIALOG:
			GetWindowRect(gApp.hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowText(hDlg, "Change Rerecord Count");		
			strcpy(Str_Tmp,"Enter the rerecord count for this movie.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM) Str_Tmp);			
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,512);	
					ChangeRerecordCount(atoi(Str_Tmp));
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case IDCANCEL:
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}
