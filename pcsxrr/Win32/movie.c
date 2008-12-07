#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include "PsxCommon.h"
#include "resource.h"
#include "Win32.h"
#include "../movie.h"

//------------------------------------------------------

char szFilter[1024];
char szChoice[MAX_PATH];
OPENFILENAME ofn;
extern AppData gApp;

static void MakeOfn(char* pszFilter)
{
	sprintf(pszFilter, "%s Input Recording Files", "PCSX-RR");
	memcpy(pszFilter + strlen(pszFilter), " (*.pxm)\0*.pxm\0\0", 14 * sizeof(char));

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = gApp.hWnd;
	ofn.lpstrFilter = pszFilter;
	ofn.lpstrFile = szChoice;
	ofn.nMaxFile = sizeof(szChoice) / sizeof(char);
	ofn.lpstrInitialDir = ".\\movies";
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "pxm";

	return;
}

static void GetMovieFilenameMini(char* filenameMini)
{
	char fszDrive[MAX_PATH];
	char fszDirectory[MAX_PATH];
	char fszFilename[MAX_PATH];
	char fszExt[MAX_PATH];
	fszDrive[0] = '\0';
	fszDirectory[0] = '\0';
	fszFilename[0] = '\0';
	fszExt[0] = '\0';
	_splitpath(filenameMini, fszDrive, fszDirectory, fszFilename, fszExt);

	strcpy(currentMovie.movieFilenameMini, fszFilename);
}

static char* GetRecordingPath(char* szPath)
{
	char szDrive[MAX_PATH];
	char szDirectory[MAX_PATH];
	char szFilename[MAX_PATH];
	char szExt[MAX_PATH];
	szDrive[0] = '\0';
	szDirectory[0] = '\0';
	szFilename[0] = '\0';
	szExt[0] = '\0';
	_splitpath(szPath, szDrive, szDirectory, szFilename, szExt);
	if (szDrive[0] == '\0' && szDirectory[0] == '\0') {
		char szTmpPath[MAX_PATH];
		sprintf(szTmpPath, "%smovies\\", szCurrentPath);
		strncpy(szTmpPath + strlen(szTmpPath), szPath, MAX_PATH - strlen(szTmpPath));
		szTmpPath[MAX_PATH-1] = '\0';
		strcpy(szPath, szTmpPath);
	}

	return szPath;
}

static void DisplayReplayProperties(HWND hDlg, int bClear)
{
	struct Movie_Type dataMovie;

	// set default values
	SetDlgItemTextA(hDlg, IDC_LENGTH, "");
	SetDlgItemTextA(hDlg, IDC_FRAMES, "");
	SetDlgItemTextA(hDlg, IDC_UNDO, "");
	SetDlgItemTextA(hDlg, IDC_METADATA, "");
	SetDlgItemTextA(hDlg, IDC_REPLAYRESET, "");
	SetDlgItemTextA(hDlg, IDC_PADTYPE1, "");
	SetDlgItemTextA(hDlg, IDC_PADTYPE2, "");
	EnableWindow(GetDlgItem(hDlg, IDC_READONLY), FALSE);
	SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_UNCHECKED, 0);
	EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
	if(bClear)
		return;

	long lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
	long lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
	if (lIndex == CB_ERR)
		return;

	if (lIndex == lCount - 1) {                   // Last item is "Browse..."
		EnableWindow(GetDlgItem(hDlg, IDOK), TRUE); // Browse is selectable
		return;
	}

	long lStringLength = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETLBTEXTLEN, (WPARAM)lIndex, 0);
	if(lStringLength + 1 > MAX_PATH)
		return;

	// save movie filename in szChoice
	SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)szChoice);

	// ensure a relative path has the "movies\" path in prepended to it
	GetRecordingPath(szChoice);

	if (! loadMovieFile(szChoice,&dataMovie) )
		return;

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
	int nFPS;
	if (dataMovie.palTiming)
		nFPS = 50;
	else
		nFPS = 60;
	int nSeconds = dataMovie.totalFrames / nFPS;
	int nMinutes = nSeconds / 60;
	int nHours = nSeconds / 3600;

	//format strings
	char szFramesString[32];
	char szLengthString[32];
	char szUndoCountString[32];
	sprintf(szFramesString, "%lu", dataMovie.totalFrames);
	sprintf(szLengthString, "%02d:%02d:%02d", nHours, nMinutes % 60, nSeconds % 60);
	sprintf(szUndoCountString, "%lu", dataMovie.rerecordCount);

	// write strings to dialog
	SetDlgItemTextA(hDlg, IDC_LENGTH, szLengthString);
	SetDlgItemTextA(hDlg, IDC_FRAMES, szFramesString);
	SetDlgItemTextA(hDlg, IDC_UNDO, szUndoCountString);
	SetDlgItemTextA(hDlg, IDC_METADATA, dataMovie.authorInfo);

	if (!dataMovie.saveStateIncluded)
		SetDlgItemTextA(hDlg, IDC_REPLAYRESET, "Power-On");
	else
		SetDlgItemTextA(hDlg, IDC_REPLAYRESET, "Savestate");

	switch (dataMovie.padType1) {
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
	if (Msg == WM_INITDIALOG) {
		char szFindPath[32];
		WIN32_FIND_DATA wfd;
		HANDLE hFind;
		int i = 0;

		SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, BST_UNCHECKED, 0);

		memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
//		if (bDrvOkay) {
//			_stprintf(szFindPath, _T("movies\\%.8s*.pxm"), BurnDrvGetText(DRV_NAME));
//		}
		sprintf(szFindPath, "%smovies\\*.pxm", szCurrentPath);

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

		if (i>1) {
			DisplayReplayProperties(hDlg, 0);
		}

		SetFocus(GetDlgItem(hDlg, IDC_CHOOSE_LIST));
		return FALSE;
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			LONG lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR) {
				DisplayReplayProperties(hDlg, (lIndex == lCount - 1));		// Selecting "Browse..." will clear the replay properties display
			}
		} else if (HIWORD(wParam) == CBN_CLOSEUP) {
			LONG lCount = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCOUNT, 0, 0);
			LONG lIndex = SendDlgItemMessage(hDlg, IDC_CHOOSE_LIST, CB_GETCURSEL, 0, 0);
			if (lIndex != CB_ERR) {
				if (lIndex == lCount - 1) {
					// send an OK notification to open the file browser
					SendMessage(hDlg, WM_COMMAND, (WPARAM)IDOK, 0);
				}
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

								int nRet = GetOpenFileName(&ofn);
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
								loadMovieFile(szChoice,&currentMovie);
								// get readonly status
								currentMovie.readOnly = 0;
								if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_READONLY, BM_GETCHECK, 0, 0))
									currentMovie.readOnly = 1;
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

int PCSX_MOV_StartReplayDialog()
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
	char szFilename[MAX_PATH];
	GetDlgItemText(hDlg, IDC_FILENAME, szFilename, MAX_PATH);

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

static BOOL CALLBACK RecordDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	char* defaultFilename = "movie";
	if (Msg == WM_INITDIALOG) {
		// come up with a unique name
		char szPath[MAX_PATH];
		char szFilename[MAX_PATH];
		int i = 0;
//		sprintf(szFilename, "%.8s.pxm", BurnDrvGetText(DRV_NAME));
		sprintf(szFilename, "%.8s.pxm", defaultFilename);
		strcpy(szPath, szFilename);
		while(VerifyRecordingAccessMode(szPath, 0) == 1) {
//			sprintf(szFilename, _T("%.8s-%d.pxm"), BurnDrvGetText(DRV_NAME), ++i);
			sprintf(szFilename, "%.8s-%d.pxm", defaultFilename, ++i);
			strcpy(szPath, szFilename);
		}

		SetDlgItemText(hDlg, IDC_FILENAME, szFilename);
		SetDlgItemText(hDlg, IDC_METADATA, "");
		CheckDlgButton(hDlg, IDC_REPLAYRESET, BST_CHECKED);

		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Standard");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Dual Analog");
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_INSERTSTRING, -1, (LPARAM)"Mouse");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Standard");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Dual Analog");
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_INSERTSTRING, -1, (LPARAM)"Mouse");
		
		SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_SETCURSEL, 0, 0);

		VerifyRecordingFilename(hDlg);

		SetFocus(GetDlgItem(hDlg, IDC_METADATA));
		return FALSE;
	}

	if (Msg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE) {
			VerifyRecordingFilename(hDlg);
		} else {
			int wID = LOWORD(wParam);
			switch (wID) {
				case IDC_BROWSE:
					{
						sprintf(szChoice, "%.8s", defaultFilename);
						MakeOfn(szFilter);
						ofn.lpstrTitle = "Record Input to File";
						ofn.Flags |= OFN_OVERWRITEPROMPT;
						int nRet = GetSaveFileName(&ofn);
						if (nRet != 0) {
							// this should trigger an EN_CHANGE message
							SetDlgItemText(hDlg, IDC_FILENAME, szChoice);
						}
					}
					return TRUE;
				case IDOK:
					GetDlgItemText(hDlg, IDC_FILENAME, szChoice, MAX_PATH);
					GetDlgItemText(hDlg, IDC_METADATA, currentMovie.authorInfo, MOVIE_MAX_METADATA);
					currentMovie.saveStateIncluded = 1;
					if (BST_CHECKED == SendDlgItemMessage(hDlg, IDC_REPLAYRESET, BM_GETCHECK, 0, 0)) {
						currentMovie.saveStateIncluded = 0;
					}
				currentMovie.authorInfo[MOVIE_MAX_METADATA-1] = '\0';
					// ensure a relative path has the "movies\" path in prepended to it
					currentMovie.movieFilename = GetRecordingPath(szChoice);
					GetMovieFilenameMini(currentMovie.movieFilename);
					LONG lIndex = SendDlgItemMessage(hDlg, IDC_PADTYPE1, CB_GETCURSEL, 0, 0);
					currentMovie.padType1 = (unsigned char)lIndex;
					switch (currentMovie.padType1) {
						case 2:
							currentMovie.padType1=PSE_PAD_TYPE_MOUSE;
							break;
						case 1:
							currentMovie.padType1=PSE_PAD_TYPE_ANALOGPAD;
							break;
						case 0:
						default:
							currentMovie.padType1=PSE_PAD_TYPE_STANDARD;
					}
					lIndex = SendDlgItemMessage(hDlg, IDC_PADTYPE2, CB_GETCURSEL, 0, 0);
					currentMovie.padType2 = (unsigned char)lIndex;
					switch (currentMovie.padType2) {
						case 2:
							currentMovie.padType2=PSE_PAD_TYPE_MOUSE;
							break;
						case 1:
							currentMovie.padType2=PSE_PAD_TYPE_ANALOGPAD;
							break;
						case 0:
						default:
							currentMovie.padType2=PSE_PAD_TYPE_STANDARD;
					}
					EndDialog(hDlg, 1);
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

int PCSX_MOV_StartMovieDialog()
{
	return DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_RECORDINP), gApp.hWnd, RecordDialogProc);
}
