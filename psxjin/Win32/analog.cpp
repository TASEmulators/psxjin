/*  PSXjin - Pc Psx Emulator
 *  Copyright (C) 1999-2003  PSXjin Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <string>
#include "resource.h"

bool enable = true;

//TODO: Enable Analog conrol should be check/unchecked in INITDIALOG based on some WndMain bool

void UpdateControls(HWND hWnd)
{
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_LEFTY) ,(enable ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_LEFTX) ,(enable ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_RIGHTX),(enable ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_RIGHTY),(enable ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_LEFTGROUP) ,(enable ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_RIGHTGROUP),(enable ? MF_ENABLED:MF_GRAYED) );
}

BOOL CALLBACK AnalogControlProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char guifuckingsucks[1024];
	switch(uMsg)
	{
		case WM_DESTROY:
		case WM_CLOSE:
		case WM_QUIT:
			DestroyWindow(hW);
			break;
		case WM_INITDIALOG:
			ScrollBar_SetRange(GetDlgItem(hW,IDC_PAD_LEFTX), 0, 255, true);
			ScrollBar_SetRange(GetDlgItem(hW,IDC_PAD_LEFTY), 0, 255, true);
			ScrollBar_SetRange(GetDlgItem(hW,IDC_PAD_RIGHTX), 0, 255, true);
			ScrollBar_SetRange(GetDlgItem(hW,IDC_PAD_RIGHTY), 0, 255, true);		
			ScrollBar_SetPos(GetDlgItem(hW,IDC_PAD_LEFTX),128,true);
			ScrollBar_SetPos(GetDlgItem(hW,IDC_PAD_LEFTY),128,true);
			ScrollBar_SetPos(GetDlgItem(hW,IDC_PAD_RIGHTX),128,true);
			ScrollBar_SetPos(GetDlgItem(hW,IDC_PAD_RIGHTY),128,true);
			
		case WM_PAINT: 
		{
			sprintf(guifuckingsucks,"X=%03d Y=%03d",ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_LEFTX)),ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_LEFTY)));
			SetDlgItemText(hW, IDC_LeftBox, guifuckingsucks);
			sprintf(guifuckingsucks,"X=%03d Y=%03d",ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_RIGHTX)),ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_RIGHTY)));
			SetDlgItemText(hW, IDC_RightBox, guifuckingsucks);
		}
		break;
		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case BN_CLICKED:
					switch(LOWORD(wParam))
					{
					case IDC_ENABLEANALOG:
						enable = !enable;
						UpdateControls(hW);
						break;
					}
				break;
			}
			switch(LOWORD(wParam))
			{
				case IDOK:
				DestroyWindow(hW);
				break;
			}		
	}
	return false;
}
