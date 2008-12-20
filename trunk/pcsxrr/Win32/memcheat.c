#include "PsxCommon.h"
#include "resource.h"
#include "../cheat.h"
#include "../movie.h"

#ifdef WIN32
#include "Win32.h"
#endif

extern AppData gApp;

static void LoadCheatFile(char nameo[2048])
{
	const char filter[]="PCSX cheat list(*.cht)\0*.cht\0";
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=gApp.hInstance;
	ofn.lpstrTitle="Load Cheat List...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=".\\";
	
	if(!GetOpenFileName(&ofn))
		nameo[0]=0;
}

static void SaveCheatFile(char nameo[2048])
{
	const char filter[]="PCSX cheat list(*.cht)\0*.cht\0";
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=gApp.hInstance;
	ofn.lpstrTitle="Save Cheat List...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=".\\";
	if(GetSaveFileName(&ofn))
	{
		int i;

		//quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		//add .cht if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 'c';
			nameo[i+2] = 'h';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}
	}
	else
		nameo[0]=0;
}

static BOOL CALLBACK ChtEdtrCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP hBmp;
	static int internal_change;
	static int has_sel;
	static int  sel_idx;
	static uint8 new_sel;
	static CheatTracker ct;
	switch(uMsg)
	{

	case WM_INITDIALOG:
	{
		hBmp=(HBITMAP)LoadImage(NULL, TEXT("funkyass.bmp"), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
		ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

//		SendDlgItemMessage(hwndDlg, IDC_CHEAT_CODE, EM_LIMITTEXT, 14, 0);
		SendDlgItemMessage(hwndDlg, IDC_CHEAT_DESCRIPTION, EM_LIMITTEXT, 22, 0);
		SendDlgItemMessage(hwndDlg, IDC_CHEAT_ADDRESS, EM_LIMITTEXT, 8, 0);
		SendDlgItemMessage(hwndDlg, IDC_CHEAT_BYTE, EM_LIMITTEXT, 3, 0);

		LVCOLUMN col;
		char temp[32];
		strcpy(temp,"Address");
		ZeroMemory(&col, sizeof(LVCOLUMN));
		col.mask=LVCF_FMT|LVCF_ORDER|LVCF_TEXT|LVCF_WIDTH;
		col.fmt=LVCFMT_LEFT;
		col.iOrder=0;
		col.cx=70;
		col.cchTextMax=7;
		col.pszText=temp;

		ListView_InsertColumn(GetDlgItem(hwndDlg,IDC_CHEAT_LIST),    0,   &col);

		strcpy(temp,"Value");
		ZeroMemory(&col, sizeof(LVCOLUMN));
		col.mask=LVCF_FMT|LVCF_ORDER|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
		col.fmt=LVCFMT_LEFT;
		col.iOrder=1;
		col.cx=43;
		col.cchTextMax=3;
		col.pszText=temp;
		col.iSubItem=1;

		ListView_InsertColumn(GetDlgItem(hwndDlg,IDC_CHEAT_LIST),    1,   &col);

		strcpy(temp,"Description");
		ZeroMemory(&col, sizeof(LVCOLUMN));
		col.mask=LVCF_FMT|LVCF_ORDER|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
		col.fmt=LVCFMT_LEFT;
		col.iOrder=2;
		col.cx=165;
		col.cchTextMax=32;
		col.pszText=temp;
		col.iSubItem=2;

		ListView_InsertColumn(GetDlgItem(hwndDlg,IDC_CHEAT_LIST),    2,   &col);

		ct.index=malloc(sizeof(int)*Cheat.num_cheats);
		ct.state=malloc(sizeof(DWORD)*Cheat.num_cheats);

		uint32 counter;
		for(counter=0; counter<Cheat.num_cheats; counter++)
		{
			char buffer[7];
			int curr_idx=-1;
			sprintf(buffer, "%06X", Cheat.c[counter].address);
			LVITEM lvi;
			ZeroMemory(&lvi, sizeof(LVITEM));
			lvi.mask=LVIF_TEXT;
			lvi.pszText=buffer;
			lvi.cchTextMax=7;
			lvi.iItem=counter;
			curr_idx=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), &lvi);

			unsigned int k;
			for(k=0;k<counter;k++)
			{
				if(ct.index[k]>=curr_idx)
					ct.index[k]++;
			}
			ct.index[counter]=curr_idx;
			ct.state[counter]=Untouched;

			sprintf(buffer, "%02X", Cheat.c[counter].byte);
			ZeroMemory(&lvi, sizeof(LVITEM));
			lvi.iItem=curr_idx;
			lvi.iSubItem=1;
			lvi.mask=LVIF_TEXT;
			lvi.pszText=buffer;
			lvi.cchTextMax=3;
			SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);

			ZeroMemory(&lvi, sizeof(LVITEM));
			lvi.iItem=curr_idx;
			lvi.iSubItem=2;
			lvi.mask=LVIF_TEXT;
			lvi.pszText=Cheat.c[counter].name;
			lvi.cchTextMax=23;
			SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);

			ListView_SetCheckState(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), curr_idx, Cheat.c[counter].enabled);
		}
	return 1;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint (hwndDlg, &ps);
		if(hBmp)
		{
			BITMAP bmp;
			ZeroMemory(&bmp, sizeof(BITMAP));
			RECT r;
			GetClientRect(hwndDlg, &r);
			HDC hdc=GetDC(hwndDlg);
			HDC hDCbmp=CreateCompatibleDC(hdc);
			GetObject(hBmp, sizeof(BITMAP), &bmp);
			HBITMAP hOldBmp=(HBITMAP)SelectObject(hDCbmp, hBmp);
			StretchBlt(hdc, 0,0,r.right,r.bottom,hDCbmp,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
			SelectObject(hDCbmp, hOldBmp);
			DeleteDC(hDCbmp);
			ReleaseDC(hwndDlg, hdc);
		}
	
		EndPaint (hwndDlg, &ps);
		return 1;
	}

	case WM_NOTIFY:
	{
		switch(LOWORD(wParam))
		{
		case IDC_CHEAT_LIST:
			if(0==ListView_GetSelectedCount(GetDlgItem(hwndDlg, IDC_CHEAT_LIST)))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_CHEAT), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 0);
				has_sel=0;
				sel_idx=-1;
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_CHEAT), 1);
				if(!has_sel||sel_idx!=ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_CHEAT_LIST)))
				{
					new_sel=3;
					//change
					char buf[25];
					LV_ITEM lvi;

					ITEM_QUERY (lvi, IDC_CHEAT_LIST, 0, buf, 7);

					SetDlgItemText(hwndDlg, IDC_CHEAT_ADDRESS, lvi.pszText);

					ITEM_QUERY (lvi, IDC_CHEAT_LIST, 1, &buf[strlen(buf)], 3);

//					SetDlgItemText(hwndDlg, IDC_CHEAT_CODE, buf);
					char temp[4];
					int q;
					sscanf(lvi.pszText, "%02X", &q);
					sprintf(temp, "%d", q);
					SetDlgItemText(hwndDlg, IDC_CHEAT_BYTE, temp);

					ITEM_QUERY (lvi, IDC_CHEAT_LIST, 2, buf, 24);

					internal_change=1;
					SetDlgItemText(hwndDlg, IDC_CHEAT_DESCRIPTION, lvi.pszText);
				}
				sel_idx=ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_CHEAT_LIST));
				has_sel=1;
			}
			return 1;
		default:
			return 0;
		}
	}

	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
			case IDC_CHEAT_DESCRIPTION:
			{
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					if(internal_change)
					{
						internal_change=!internal_change;
						return 0;
					}
					if(!has_sel)
						return 1;
					EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 1);
					return 1;
				}
				break;
			}

			case IDC_ADD_CHEAT:
			{
				char temp[24];
				uint8 byte;
				char buffer[7];
				char buffer2[7];
				
				GetDlgItemText(hwndDlg, IDC_CHEAT_ADDRESS, buffer, 7);
				GetDlgItemText(hwndDlg, IDC_CHEAT_BYTE, buffer2, 7);
				
				int curr_idx=-1;
				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.mask=LVIF_TEXT;
				lvi.pszText=buffer;
				lvi.cchTextMax=6;
				lvi.iItem=0;
				curr_idx=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), &lvi);
				
				int scanres;
				if(buffer2[0]=='$')
					sscanf(buffer2,"$%2X", (unsigned int*)&scanres);
				else sscanf(buffer2,"%d", &scanres);
				byte = (uint8)(scanres & 0xff);
				
				sprintf(buffer2, "%02X", byte);
				
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.iItem=curr_idx;
				lvi.iSubItem=1;
				lvi.mask=LVIF_TEXT;
				lvi.pszText=buffer2;
				lvi.cchTextMax=2;
				SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);
				
				GetDlgItemText(hwndDlg, IDC_CHEAT_DESCRIPTION, temp, 23);
				
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.iItem=curr_idx;
				lvi.iSubItem=2;
				lvi.mask=LVIF_TEXT;
				lvi.pszText=temp;
				lvi.cchTextMax=23;
				SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);
				int j;
				for(j=0;j<(int)Cheat.num_cheats;j++)
				{
					ct.index[j]++;
				}
				break;
			}

			case IDC_UPDATE_CHEAT:
			{
				char temp[24];
				uint8 byte;
				char buffer[7];
				
				GetDlgItemText(hwndDlg, IDC_CHEAT_ADDRESS, buffer, 7);
				int j;
				for(j=0;j<(int)Cheat.num_cheats;j++)
				{
					if(ct.index[j]==sel_idx)
						ct.state[j]=Modified;
				}

				LVITEM lvi;
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.mask=LVIF_TEXT;
				lvi.pszText=buffer;
				lvi.cchTextMax=6;
				lvi.iItem=sel_idx;
				ListView_SetItem(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), &lvi);
				
				GetDlgItemText(hwndDlg, IDC_CHEAT_BYTE, buffer, 7);
				
				int scanres;
				if(buffer[0]=='$')
					sscanf(buffer,"$%2X", (unsigned int*)&scanres);
				else sscanf(buffer,"%d", &scanres);
				byte = (uint8)(scanres & 0xff);
				
				sprintf(buffer, "%02X", byte);
				
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.iItem=sel_idx;
				lvi.iSubItem=1;
				lvi.mask=LVIF_TEXT;
				lvi.pszText=buffer;
				lvi.cchTextMax=2;
				SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);
				
				GetDlgItemText(hwndDlg, IDC_CHEAT_DESCRIPTION, temp, 23);
				
				ZeroMemory(&lvi, sizeof(LVITEM));
				lvi.iItem=sel_idx;
				lvi.iSubItem=2;
				lvi.mask=LVIF_TEXT;
				lvi.pszText=temp;
				lvi.cchTextMax=23;
				SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);
				break;
			}

			case IDC_DELETE_CHEAT:
			{
				{
					unsigned int j;
					for(j=0;j<Cheat.num_cheats;j++)
					{
						if(ct.index[j]==sel_idx)
							ct.state[j]=Deleted;
					}
					for(j=0;j<Cheat.num_cheats;j++)
					{
						if(ct.index[j]>sel_idx)
							ct.index[j]--;
					}
				}
				ListView_DeleteItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), sel_idx);
				
				break;
			}

			case IDC_CLEAR_CHEATS:
			{
				internal_change = TRUE;
				SetDlgItemText(hwndDlg,IDC_CHEAT_CODE,"");
				SetDlgItemText(hwndDlg,IDC_CHEAT_ADDRESS,"");
				SetDlgItemText(hwndDlg,IDC_CHEAT_BYTE,"");
				SetDlgItemText(hwndDlg,IDC_CHEAT_DESCRIPTION,"");
				ListView_SetItemState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),sel_idx, 0, LVIS_SELECTED|LVIS_FOCUSED);
				ListView_SetSelectionMark(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), -1);
				sel_idx=-1;
				has_sel=FALSE;
				break;
			}

			case IDC_CHEAT_ADDRESS:
			{
				uint32 j, k;
				long index;
				char buffer[9];
				char buffer2[9];
				POINT point;
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					if(internal_change)
					{
						internal_change=0;
						return 1;
					}
					SendMessage((HWND)lParam, WM_GETTEXT, 9,(LPARAM)buffer);
					GetCaretPos(&point);

					index = SendMessage((HWND)lParam,(UINT) EM_CHARFROMPOS, 0, (LPARAM) ((point.x&0x0000FFFF) | (((point.y&0x0000FFFF))<<16)));  

					k=0;
					for(j=0; j<strlen(buffer);j++)
					{
						if( (buffer[j]>='0' && buffer[j]<='9') || (buffer[j]>='A' && buffer[j]<='F'))
						{
							buffer2[k]=buffer[j];
							k++;
						}
						else index --;
					}
					buffer2[k]='\0';

					// hack to prevent people from using 80xxxxxx addresses
					if (strlen(buffer2)>7) {
						for(j=0; j<strlen(buffer2)-2;j++)
						{
							buffer2[j]=buffer2[j+2];
						}
						buffer2[j]='\0';
					}

					internal_change=1;
					SendMessage((HWND)lParam, WM_SETTEXT, 0,(LPARAM)buffer2);
					SendMessage((HWND)lParam,  (UINT) EM_SETSEL, (WPARAM) (index), index);

					SendMessage(GetDlgItem(hwndDlg, IDC_CHEAT_BYTE), WM_GETTEXT, 4,(LPARAM)buffer);
					
//					if(has_sel&&!new_sel&&0!=strlen(buffer2))
//						SetDlgItemText(hwndDlg, IDC_CHEAT_CODE, "");

					if(new_sel!=0)
						new_sel--;

					if(strlen(buffer2)!=0 && strlen(buffer) !=0)
					{
						if(has_sel)
							EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 1);
						else EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_CHEAT), 1);
					}
					else
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_CHEAT), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 0);
					}
					break;
				}
				break;
			}

			case IDC_CHEAT_BYTE:
			{
				uint32 j, k;
				long index;
				char buffer[4];
				char buffer2[4];
				POINT point;
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
					if(internal_change)
					{
						internal_change=0;
						return 1;
					}
					SendMessage((HWND)lParam, WM_GETTEXT, 4,(LPARAM)buffer);
					GetCaretPos(&point);

					index = SendMessage((HWND)lParam,(UINT) EM_CHARFROMPOS, 0, (LPARAM) ((point.x&0x0000FFFF) | (((point.y&0x0000FFFF))<<16)));  

					k=0;
					for(j=0; j<strlen(buffer);j++)
					{
						if( (buffer[j]>='0' && buffer[j]<='9') || (buffer[j]>='A' && buffer[j]<='F') || buffer[j]=='$')
						{
							buffer2[k]=buffer[j];
							k++;
						}
						else index --;
					}
					buffer2[k]='\0';
					
					if(new_sel!=0)
						new_sel--;

					internal_change=1;
					SendMessage((HWND)lParam, WM_SETTEXT, 0,(LPARAM)buffer2);
					SendMessage((HWND)lParam,  (UINT) EM_SETSEL, (WPARAM) (index), index);
					
					SendMessage(GetDlgItem(hwndDlg, IDC_CHEAT_ADDRESS), WM_GETTEXT, 7,(LPARAM)buffer);
					if(strlen(buffer2)!=0 && strlen(buffer) !=0)
					{
						if(has_sel)
							EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 1);
						else EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_CHEAT), 1);
					}
					else
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_CHEAT), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE_CHEAT), 0);
					}
					break;
				}
				break;
			}

			case IDC_LOAD_CHEATS:
			{
				char nameo[2048];
				LoadCheatFile(nameo);
				if (!nameo[0]) return 1;

				free(ct.index);
				free(ct.state);
				int k=0;
				int totalk = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_CHEAT_LIST));
				for(k=0;k<totalk; k++) {
					ListView_DeleteItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), 0);
				}

				PCSXLoadCheatFile(nameo);
				ct.index=malloc(sizeof(int)*Cheat.num_cheats);
				ct.state=malloc(sizeof(DWORD)*Cheat.num_cheats);

				uint32 counter;
				for(counter=0; counter<Cheat.num_cheats; counter++)
				{
					char buffer[7];
					int curr_idx=-1;
					sprintf(buffer, "%06X", Cheat.c[counter].address);
					LVITEM lvi;
					ZeroMemory(&lvi, sizeof(LVITEM));
					lvi.mask=LVIF_TEXT;
					lvi.pszText=buffer;
					lvi.cchTextMax=7;
					lvi.iItem=counter;
					curr_idx=ListView_InsertItem(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), &lvi);

					unsigned int k;
					for(k=0;k<counter;k++)
					{
						if(ct.index[k]>=curr_idx)
							ct.index[k]++;
					}
					ct.index[counter]=curr_idx;
					ct.state[counter]=Untouched;

					sprintf(buffer, "%02X", Cheat.c[counter].byte);
					ZeroMemory(&lvi, sizeof(LVITEM));
					lvi.iItem=curr_idx;
					lvi.iSubItem=1;
					lvi.mask=LVIF_TEXT;
					lvi.pszText=buffer;
					lvi.cchTextMax=3;
					SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);

					ZeroMemory(&lvi, sizeof(LVITEM));
					lvi.iItem=curr_idx;
					lvi.iSubItem=2;
					lvi.mask=LVIF_TEXT;
					lvi.pszText=Cheat.c[counter].name;
					lvi.cchTextMax=23;
					SendDlgItemMessage(hwndDlg,IDC_CHEAT_LIST, LVM_SETITEM, 0, (LPARAM)&lvi);

					ListView_SetCheckState(GetDlgItem(hwndDlg,IDC_CHEAT_LIST), curr_idx, Cheat.c[counter].enabled);
				}
				UpdateMemWatch();
				break;
			}

			case IDC_SAVE_CHEATS:
			{
				char nameo[2048];
				SaveCheatFile(nameo);
				if (!nameo[0]) return 1;

				//fake IDOK
				{
					int k,l,fakeNumCheats;
					BOOL hit;
					unsigned int scanned;
					fakeNumCheats = (int)Cheat.num_cheats;
					int totalk = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_CHEAT_LIST))-1;
					for(k=totalk;k>=0; k--)
					{
						hit=FALSE;
						if (!(k==0 && fakeNumCheats==0)) {
							for(l=0;l<fakeNumCheats;l++)
							{
								if(ct.index[l]==k)
								{
									hit=TRUE;
									Cheat.c[l].enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),ct.index[l]);
									if(ct.state[l]==Untouched)
										l=Cheat.num_cheats;
									else if(ct.state[l]==(unsigned long)Modified)
									{
										if(Cheat.c[l].enabled)
											PCSXDisableCheat(l);
										
										char buf[25];
										LV_ITEM lvi;
										ZeroMemory(&lvi, sizeof(LV_ITEM));
										lvi.iItem= k;
										lvi.mask=LVIF_TEXT;
										lvi.pszText=buf;
										lvi.cchTextMax=7;
										
										ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
										
										ScanAddress(lvi.pszText,&Cheat.c[l].address);
										
										ZeroMemory(&lvi, sizeof(LV_ITEM));
										lvi.iItem= k;
										lvi.iSubItem=1;
										lvi.mask=LVIF_TEXT;
										lvi.pszText=buf;
										lvi.cchTextMax=3;
										
										ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
										
										sscanf(lvi.pszText, "%02X", &scanned);
										Cheat.c[l].byte = (uint8)(scanned & 0xff);
										
										ZeroMemory(&lvi, sizeof(LV_ITEM));
										lvi.iItem= k;
										lvi.iSubItem=2;
										lvi.mask=LVIF_TEXT;
										lvi.pszText=buf;
										lvi.cchTextMax=24;
										
										ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
										
										strcpy(Cheat.c[l].name,lvi.pszText);
										
										Cheat.c[l].enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),ct.index[l]);
										
										if(Cheat.c[l].enabled)
											PCSXEnableCheat(l);
									}
								}
							}
						}
						if(!hit)
						{
							uint32 address;
							uint8 byte;
							uint8 enabled;
							char buf[25];
							
							LV_ITEM lvi;
							ZeroMemory(&lvi, sizeof(LV_ITEM));
							lvi.iItem= k;
							lvi.mask=LVIF_TEXT;
							lvi.pszText=buf;
							lvi.cchTextMax=7;
							
							ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
							
							ScanAddress(lvi.pszText,&address);
							
							ZeroMemory(&lvi, sizeof(LV_ITEM));
							lvi.iItem= k;
							lvi.iSubItem=1;
							lvi.mask=LVIF_TEXT;
							lvi.pszText=buf;
							lvi.cchTextMax=3;
							
							ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
							
							sscanf(lvi.pszText, "%02X", &scanned);
							byte = (uint8)(scanned & 0xff);
							
							enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),k);
							
							PCSXAddCheat(enabled,1,address,byte);
							
							ZeroMemory(&lvi, sizeof(LV_ITEM));
							lvi.iItem= k;
							lvi.iSubItem=2;
							lvi.mask=LVIF_TEXT;
							lvi.pszText=buf;
							lvi.cchTextMax=24;
							
							ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
							
							strcpy(Cheat.c[Cheat.num_cheats-1].name, lvi.pszText);
						}
					}
	
					for(l=(int)Cheat.num_cheats;l>=0;l--)
					{
						if(ct.state[l]==Deleted)
						{
							PCSXDeleteCheat(l);
						}
					}
				}
				PCSXSaveCheatFile(nameo);
				break;
			}

			case IDOK:
			{
				int k,l,fakeNumCheats;
				BOOL hit;
				unsigned int scanned;
				fakeNumCheats = (int)Cheat.num_cheats;
				int totalk = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_CHEAT_LIST))-1;
				for(k=totalk;k>=0; k--)
				{
					hit=FALSE;
					if (!(k==0 && fakeNumCheats==0)) {
						for(l=0;l<fakeNumCheats;l++)
						{
							if(ct.index[l]==k)
							{
								hit=TRUE;
								Cheat.c[l].enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),ct.index[l]);
								if(ct.state[l]==Untouched)
									l=Cheat.num_cheats;
								else if(ct.state[l]==(unsigned long)Modified)
								{
									if(Cheat.c[l].enabled)
										PCSXDisableCheat(l);
									
									char buf[25];
									LV_ITEM lvi;
									ZeroMemory(&lvi, sizeof(LV_ITEM));
									lvi.iItem= k;
									lvi.mask=LVIF_TEXT;
									lvi.pszText=buf;
									lvi.cchTextMax=7;
									
									ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
									
									ScanAddress(lvi.pszText,&Cheat.c[l].address);
									
									ZeroMemory(&lvi, sizeof(LV_ITEM));
									lvi.iItem= k;
									lvi.iSubItem=1;
									lvi.mask=LVIF_TEXT;
									lvi.pszText=buf;
									lvi.cchTextMax=3;
									
									ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
									
									sscanf(lvi.pszText, "%02X", &scanned);
									Cheat.c[l].byte = (uint8)(scanned & 0xff);
									
									ZeroMemory(&lvi, sizeof(LV_ITEM));
									lvi.iItem= k;
									lvi.iSubItem=2;
									lvi.mask=LVIF_TEXT;
									lvi.pszText=buf;
									lvi.cchTextMax=24;
									
									ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
									
									strcpy(Cheat.c[l].name,lvi.pszText);
									
									Cheat.c[l].enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),ct.index[l]);
									
									if(Cheat.c[l].enabled)
										PCSXEnableCheat(l);
								}
							}
						}
					}
					if(!hit)
					{
						uint32 address;
						uint8 byte;
						uint8 enabled;
						char buf[25];
						
						LV_ITEM lvi;
						ZeroMemory(&lvi, sizeof(LV_ITEM));
						lvi.iItem= k;
						lvi.mask=LVIF_TEXT;
						lvi.pszText=buf;
						lvi.cchTextMax=7;
						
						ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
						
						ScanAddress(lvi.pszText,&address);
						
						ZeroMemory(&lvi, sizeof(LV_ITEM));
						lvi.iItem= k;
						lvi.iSubItem=1;
						lvi.mask=LVIF_TEXT;
						lvi.pszText=buf;
						lvi.cchTextMax=3;
						
						ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
						
						sscanf(lvi.pszText, "%02X", &scanned);
						byte = (uint8)(scanned & 0xff);
						
						enabled=ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_CHEAT_LIST),k);
						
						PCSXAddCheat(enabled,1,address,byte);
						
						ZeroMemory(&lvi, sizeof(LV_ITEM));
						lvi.iItem= k;
						lvi.iSubItem=2;
						lvi.mask=LVIF_TEXT;
						lvi.pszText=buf;
						lvi.cchTextMax=24;
						
						ListView_GetItem(GetDlgItem(hwndDlg, IDC_CHEAT_LIST), &lvi);
						
						strcpy(Cheat.c[Cheat.num_cheats-1].name, lvi.pszText);
					}
				}

				for(l=(int)Cheat.num_cheats;l>=0;l--)
				{
					if(ct.state[l]==Deleted)
					{
						PCSXDeleteCheat(l);
					}
				}

				UpdateMemWatch();
				if (!cheatsEnabled) {
					cheatsEnabled = 1;
					GPU_displayText(_("*PCSX*: Cheats Enabled"));
				}
			}

			case IDCANCEL:
			{
				free(ct.index);
				free(ct.state);
				EndDialog(hwndDlg, 0);
				if(hBmp)
				{
					DeleteObject(hBmp);
					hBmp=NULL;
				}
				return 1;
			}

			default:
				return 0;
		}
	}

	default:
		return 0;
	}
	return 0;
}

void CreateCheatEditor()
{
	DialogBox(gApp.hInstance,MAKEINTRESOURCE(IDD_CHEATER),gApp.hWnd,(DLGPROC)ChtEdtrCallB);
}
