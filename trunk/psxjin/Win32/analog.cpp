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
#include "PSXCommon.h"

bool enable = true;
char Temp_Str[1024];

//TODO: Enable Analog conrol should be check/unchecked in INITDIALOG based on some WndMain bool

void UpdatePositionText(HWND hWnd)
{
	if (Config.WriteAnalog)		
	{
		Config.WriteAnalog = false;
		//SendDlgItemMessage(hWnd,IDC_PAD_RIGHTX,TBM_SETPOS,0,0) ??? WTF? 
	}
	sprintf(Temp_Str,"X=%03d Y=%03d",SendDlgItemMessage(hWnd,IDC_PAD_LEFTX,TBM_GETPOS,0,0),SendDlgItemMessage(hWnd,IDC_PAD_LEFTY,TBM_GETPOS,0,0));
	SetDlgItemText(hWnd, IDC_LeftBox, Temp_Str);
	sprintf(Temp_Str,"X=%03d Y=%03d",SendDlgItemMessage(hWnd,IDC_PAD_RIGHTX,TBM_GETPOS,0,0),SendDlgItemMessage(hWnd,IDC_PAD_RIGHTY,TBM_GETPOS,0,0));
	SetDlgItemText(hWnd, IDC_RightBox, Temp_Str);
}

void UpdateControls(HWND hWnd)
{
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_LEFTY) ,(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_LEFTX) ,(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_RIGHTX),(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_PAD_RIGHTY),(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_LEFTGROUP) ,(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
	EnableWindow(GetDlgItem(hWnd,IDC_RIGHTGROUP),(Config.enable_extern_analog ? MF_ENABLED:MF_GRAYED) );
}

BOOL CALLBACK AnalogControlProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
			Config.enable_extern_analog = true;
		case WM_VSCROLL:
		case WM_HSCROLL:
			Config.WriteAnalog = false; 
			Config.PadLeftX = SendDlgItemMessage(hW,IDC_PAD_LEFTX,TBM_GETPOS,0,0);
			Config.PadLeftY = SendDlgItemMessage(hW,IDC_PAD_LEFTY,TBM_GETPOS,0,0);
			Config.PadRightY = SendDlgItemMessage(hW,IDC_PAD_RIGHTY,TBM_GETPOS,0,0);
			Config.PadRightX = SendDlgItemMessage(hW,IDC_PAD_RIGHTX,TBM_GETPOS,0,0);
		case WM_PAINT: 	
			UpdatePositionText(hW);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case BN_CLICKED:
					switch(LOWORD(wParam))
					{
					case IDC_ENABLEANALOG:
						Config.enable_extern_analog = !Config.enable_extern_analog;
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
