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

HWND AnalogControlHWnd = NULL;
char Temp_Str[1024];

//TODO: Enable Analog conrol should be check/unchecked in INITDIALOG based on some WndMain bool

void UpdatePositionText(HWND hWnd)
{
	if (Config.WriteAnalog)		
	{
		Config.WriteAnalog = false;
		SendDlgItemMessage(hWnd,IDC_PAD_LEFTX,TBM_SETPOS,true,Config.PadLeftX);
		SendDlgItemMessage(hWnd,IDC_PAD_LEFTY,TBM_SETPOS,true,Config.PadLeftY);
		SendDlgItemMessage(hWnd,IDC_PAD_RIGHTX,TBM_SETPOS,true,Config.PadRightX);
		SendDlgItemMessage(hWnd,IDC_PAD_RIGHTY,TBM_SETPOS,true,Config.PadRightY);
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

void CloseAnalogCongrol(HWND hWnd)
{
	Config.enable_extern_analog = false;
	DestroyWindow(hWnd);
	AnalogControlHWnd = NULL;
}

BOOL CALLBACK AnalogControlProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	switch(uMsg)
	{
		case WM_DESTROY:
		case WM_CLOSE:
		case WM_QUIT:
			CloseAnalogCongrol(hW);
			break;
		case WM_INITDIALOG:				
			{
			GetWindowRect(gApp.hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hW, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			// push it away from the main window if we can
			const int width = (r.right-r.left); 
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}
			}
			SendDlgItemMessage(hW,IDC_PAD_LEFTX,TBM_SETRANGE,true,MAKELONG(0, 255));		
			SendDlgItemMessage(hW,IDC_PAD_LEFTY,TBM_SETRANGE,true,MAKELONG(0, 255));		
			SendDlgItemMessage(hW,IDC_PAD_RIGHTX,TBM_SETRANGE,true,MAKELONG(0, 255));		
			SendDlgItemMessage(hW,IDC_PAD_RIGHTY,TBM_SETRANGE,true,MAKELONG(0, 255));		
			SendDlgItemMessage(hW,IDC_PAD_LEFTX,TBM_SETPOS,true,128);
			SendDlgItemMessage(hW,IDC_PAD_LEFTY,TBM_SETPOS,true,128);
			SendDlgItemMessage(hW,IDC_PAD_RIGHTX,TBM_SETPOS,true,128);
			SendDlgItemMessage(hW,IDC_PAD_RIGHTY,TBM_SETPOS,true,128);
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
					CloseAnalogCongrol(hW);
					break;
			}		
	}
	return false;
}

void OpenAnalogControl()
{
	if (!AnalogControlHWnd)
		AnalogControlHWnd = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_ANALOG_CONTROL), NULL, (DLGPROC)AnalogControlProc);
	else
		SetForegroundWindow(AnalogControlHWnd);
}