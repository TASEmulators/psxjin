/*  PSXjin - Pc Psx Emulator
 *  Copyright (C) 1999-2011  PSXjin Team
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
#include <math.h>
#include <algorithm>

//TODO: Enable Analog control should be check/unchecked in INITDIALOG based on some WndMain bool

//HOWTO: make sure AllowUserInput is set when the user is allowed to edit the analog panels
//the analog panels will always redraw themselves from Config.PadLeftX etc. just call UpdateAll() to make them get repainted
//this, of course, applies even while a movie is playing
//
//you should probably change the sliders to work the same as in mupen tasinput plugin. i sort of like it better. but i didnt do that now

HWND AnalogControlHWnd = NULL;
static bool AllowUserInput = true;

void UpdatePositionText(HWND hWnd)
{
	{
		SendDlgItemMessage(hWnd,IDC_PAD_LEFTX,TBM_SETPOS,true,Config.PadLeftX);
		SendDlgItemMessage(hWnd,IDC_PAD_LEFTY,TBM_SETPOS,true,Config.PadLeftY);
		SendDlgItemMessage(hWnd,IDC_PAD_RIGHTX,TBM_SETPOS,true,Config.PadRightX);
		SendDlgItemMessage(hWnd,IDC_PAD_RIGHTY,TBM_SETPOS,true,Config.PadRightY);
	}
	char tmp[1024];
	sprintf(tmp,"X=%03d Y=%03d",Config.PadLeftX,Config.PadLeftY);
	SetDlgItemText(hWnd, IDC_LeftBox, tmp);
	sprintf(tmp,"X=%03d Y=%03d",Config.PadRightX,Config.PadRightY);
	SetDlgItemText(hWnd, IDC_RightBox, tmp);
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

void CloseAnalogControl(HWND hWnd)
{
	Config.enable_extern_analog = false;
	DestroyWindow(hWnd);
	AnalogControlHWnd = NULL;
}

void UpdateAll();
static WNDPROC wpOrigEditProc;

class AnalogPanel
{
public:
	AnalogPanel(int _which) : which(_which) {}
	int which;
	HWND hwnd;

	inline float range() { return 255.9999f; }

	void update()
	{
		InvalidateRect(hwnd,NULL,true);
		UpdateWindow(hwnd);
	}

	POINT getCurPoint()
	{
		int x,y;
		if(which==0)
		{
			x = Config.PadLeftX;
			y = Config.PadLeftY;
		}
		else
		{
			x = Config.PadRightX;
			y = Config.PadRightY;
		}

		//convert to [0,1]
		float fx = x/range();
		float fy = y/range();

		//convert to [0,clientSize]
		RECT rect;
		GetClientRect(hwnd,&rect);
		POINT pt;
		pt.x = (int)(fx*(rect.right-rect.left));
		pt.y = (int)(fy*(rect.right-rect.left));

		return pt;
	}

	//receives pixel coordinates, converts to analog coordinates, and sets emulator state
	void userSet(int x, int y, bool shift)
	{
		RECT rect;
		GetClientRect(hwnd,&rect);

		//clamp while converting to [0,1]
		float fx = (float)x/(rect.right-rect.left);
		float fy = (float)y/(rect.right-rect.left);
		fx = std::min(std::max(fx,0.0f),1.0f);
		fy = std::min(std::max(fy,0.0f),1.0f);

		//convert to [-1,1]
		fx = fx*2-1;
		fy = fy*2-1;

		//convert to polar coordinates
		float r = sqrt(fx*fx+fy*fy);
		float theta = atan2(fy,fx);
		
		//optionally the radius can't be more than 1
		if(shift)
			r = std::min(r,1.0f);

		//convert back to cartesian [-1,1]
		fx = r*cos(theta);
		fy = r*sin(theta);

		//printf("%f, %f\n",fx,fy);

		//yes, some of these conversions are redundant. but they make things clear

		//convert to cartesian integer left[0,255]right and up[0,255]down
		x = (int)(((fx+1)/2)*range());
		y = (int)(((fy+1)/2)*range());

		//printf("[%d, %d]\n",x,y);
		
		//shove back into emulator state
		if(which==0)
		{
			Config.PadLeftX = x;
			Config.PadLeftY = y;
		}
		else
		{
			Config.PadRightX = x;
			Config.PadRightY = y;
		}

		UpdateAll();
	}
};

static AnalogPanel analogPanels[2] = { AnalogPanel(0), AnalogPanel(1) };

static void UpdateAll()
{
	for(int i=0;i<2;i++)
		analogPanels[i].update();
	UpdatePositionText(AnalogControlHWnd);
}


//wndproc for the AnalogPanel
LRESULT APIENTRY AnalogPanelProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AnalogPanel* panel = (AnalogPanel*)GetWindowLong(hW,GWL_USERDATA);

	switch(uMsg)
	{
		case WM_LBUTTONDOWN:
			if(AllowUserInput)
			{
				SetCapture(hW);
				panel->userSet(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),wParam&MK_SHIFT);
			}
			break;

		case WM_MOUSEMOVE:
			if(GetCapture()==hW)
				panel->userSet(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),wParam&MK_SHIFT);
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hW, &ps);
			RECT rect;
			GetClientRect(hW,&rect);

			//only paint this control
			HRGN rectRgn = CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
			SelectClipRgn(hdc,rectRgn);

			//fill the panel
			DeleteObject(SelectObject(hdc,GetStockObject(LTGRAY_BRUSH)));
			Rectangle(hdc,rect.left,rect.top,rect.right,rect.bottom);

			//draw the baseline ellipse
			DeleteObject(SelectObject(hdc,GetStockObject(WHITE_BRUSH)));
			Ellipse(hdc,rect.left,rect.top,rect.right,rect.bottom);

			HPEN hpenOld, hpenBlue, hpenRed;
			hpenBlue = CreatePen(PS_SOLID, 3, RGB(0, 0, 255)); // these need to be re-created every time...
			hpenRed = CreatePen(PS_SOLID, 7, RGB(255, 0, 0));
			hpenOld = (HPEN)SelectObject(hdc, hpenBlue);

			//draw the blue line
			MoveToEx(hdc, (rect.left+rect.right)/2, (rect.top+rect.bottom)/2, NULL);
			POINT pt = panel->getCurPoint();
			LineTo(hdc, pt.x,pt.y);
			SelectObject(hdc, hpenOld);
			DeleteObject(hpenBlue);

			//draw the X/Y axes
			MoveToEx(hdc, rect.left, (rect.top+rect.bottom)>>1, NULL);
			LineTo(hdc, rect.right, (rect.top+rect.bottom)>>1);
			MoveToEx(hdc, (rect.left+rect.right)>>1, rect.top, NULL);
			LineTo(hdc, (rect.left+rect.right)>>1, rect.bottom);

			//draw the little red dot
			hpenOld = (HPEN)SelectObject(hdc, hpenRed);
			MoveToEx(hdc, pt.x,pt.y, NULL);
			LineTo(hdc, pt.x,pt.y);
			SelectObject(hdc, hpenOld);
			DeleteObject(hpenRed);

			DeleteObject(rectRgn);
			EndPaint(hW,&ps);
		}
	}

	return CallWindowProc(wpOrigEditProc, hW, uMsg, wParam, lParam); 
}

BOOL CALLBACK AnalogControlProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	switch(uMsg)
	{
		case WM_DESTROY:
			break;

		case WM_CLOSE:
			CloseAnalogControl(hW);
			break;

		case WM_INITDIALOG:				
			{
			analogPanels[0].hwnd = GetDlgItem(hW,IDC_ANALOGLEFT);
			analogPanels[1].hwnd = GetDlgItem(hW,IDC_ANALOGRIGHT);
			SetWindowLong(analogPanels[0].hwnd,GWL_USERDATA,(LONG)&analogPanels[0]);
			SetWindowLong(analogPanels[1].hwnd,GWL_USERDATA,(LONG)&analogPanels[1]);
			wpOrigEditProc = (WNDPROC)SetWindowLong(analogPanels[0].hwnd,GWL_WNDPROC,(LONG)AnalogPanelProc);
			wpOrigEditProc = (WNDPROC)SetWindowLong(analogPanels[1].hwnd,GWL_WNDPROC,(LONG)AnalogPanelProc);


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
			break;

		case WM_VSCROLL:
		case WM_HSCROLL:
			Config.WriteAnalog = false; 
			Config.PadLeftX = SendDlgItemMessage(hW,IDC_PAD_LEFTX,TBM_GETPOS,0,0);
			Config.PadLeftY = SendDlgItemMessage(hW,IDC_PAD_LEFTY,TBM_GETPOS,0,0);
			Config.PadRightY = SendDlgItemMessage(hW,IDC_PAD_RIGHTY,TBM_GETPOS,0,0);
			Config.PadRightX = SendDlgItemMessage(hW,IDC_PAD_RIGHTX,TBM_GETPOS,0,0);
			UpdateAll();
			break;

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
					CloseAnalogControl(hW);
					break;
			}
			break;
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