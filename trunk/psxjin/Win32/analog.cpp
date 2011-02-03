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
			sprintf(guifuckingsucks,"Left: X=%03d Y=%03d",ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_LEFTX)),ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_LEFTY)));
			Static_SetText(GetDlgItem(hW,IDC_PAD_LEFT_TEXT2),guifuckingsucks);
			sprintf(guifuckingsucks,"Right: X=%03d Y=%03d",ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_RIGHTX)),ScrollBar_GetPos(GetDlgItem(hW,IDC_PAD_RIGHTY)));
			Static_SetText(GetDlgItem(hW,IDC_PAD_RIGHT_TEXT),guifuckingsucks);
		}
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				DestroyWindow(hW);
				break;
			}		
	}
	return false;
}
