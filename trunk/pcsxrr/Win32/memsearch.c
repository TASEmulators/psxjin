#include "resource.h"
#include "PsxCommon.h"
#include "cheat.h"

#ifdef WIN32
#include "Win32.h"
#endif

extern AppData gApp;
extern struct SCheatData Cheat;

HWND cheatSearchHWND = NULL;

typedef enum
{
    PCSX_LESS_THAN, PCSX_GREATER_THAN, PCSX_LESS_THAN_OR_EQUAL,
    PCSX_GREATER_THAN_OR_EQUAL, PCSX_EQUAL, PCSX_NOT_EQUAL
} PCSXCheatComparisonType;

typedef enum
{
    PCSX_8_BITS, PCSX_16_BITS, PCSX_24_BITS, PCSX_32_BITS
} PCSXCheatDataSize;

#define SEARCH_TITLE_CHEATERROR "PCSX Cheat Error"
#define SEARCH_TITLE_RANGEERROR "Range Error"
#define SEARCH_ERR_INVALIDNEWVALUE "You have entered an out of range or invalid value for the new value"
#define SEARCH_ERR_INVALIDCURVALUE "You have entered an out of range or invalid value for\n"\
                                   "the current value. This value is used when a cheat is unapplied.\n"\
                                   "(If left blank, no value is restored when the cheat is unapplied)"
#define SEARCH_ERR_INVALIDSEARCHVALUE "Please enter a valid value for a search!"

#define BIT_CLEAR(a,v) \
(a)[(v) >> 5] &= ~(1 << ((v) & 31))

#define BIT_SET(a,v) \
(a)[(v) >> 5] |= 1 << ((v) & 31)

#define TEST_BIT(a,v) \
((a)[(v) >> 5] & (1 << ((v) & 31)))

#define _C(c,a,b) \
((c) == PCSX_LESS_THAN ? (a) < (b) : \
 (c) == PCSX_GREATER_THAN ? (a) > (b) : \
 (c) == PCSX_LESS_THAN_OR_EQUAL ? (a) <= (b) : \
 (c) == PCSX_GREATER_THAN_OR_EQUAL ? (a) >= (b) : \
 (c) == PCSX_EQUAL ? (a) == (b) : \
 (a) != (b))

#define _D(s,m,o) \
((s) == PCSX_8_BITS ? (uint8) (*((m) + (o))) : \
 (s) == PCSX_16_BITS ? ((uint16) (*((m) + (o)) + (*((m) + (o) + 1) << 8))) : \
 (s) == PCSX_24_BITS ? ((uint32) (*((m) + (o)) + (*((m) + (o) + 1) << 8) + (*((m) + (o) + 2) << 16))) : \
((uint32)  (*((m) + (o)) + (*((m) + (o) + 1) << 8) + (*((m) + (o) + 2) << 16) + (*((m) + (o) + 3) << 24))))

#define _DS(s,m,o) \
((s) == PCSX_8_BITS ? ((int8) *((m) + (o))) : \
 (s) == PCSX_16_BITS ? ((int16) (*((m) + (o)) + (*((m) + (o) + 1) << 8))) : \
 (s) == PCSX_24_BITS ? (((int32) ((*((m) + (o)) + (*((m) + (o) + 1) << 8) + (*((m) + (o) + 2) << 16)) << 8)) >> 8): \
 ((int32) (*((m) + (o)) + (*((m) + (o) + 1) << 8) + (*((m) + (o) + 2) << 16) + (*((m) + (o) + 3) << 24))))

void PCSXStartCheatSearch()
{
	memmove (Cheat.CRAM, Cheat.RAM, 0x200000);
	memset ((char *) Cheat.ALL_BITS, 0xff, 0x200000);
}

void PCSXInitCheatData()
{
	Cheat.RAM = (u8*)psxM;
	PCSXStartCheatSearch();
}

struct ICheat
{
	uint32  address;
	uint32  new_val;
	uint32  saved_val;
	int     size;
	uint8   enabled;
	uint8   saved;
	char    name [22];
	int format;
};

BOOL TestRange(int val_type, PCSXCheatDataSize bytes,  uint32 value)
{
	if(val_type!=2)
	{
		if(bytes==PCSX_8_BITS)
		{
			if(value<256)
				return TRUE;
			else return FALSE;
		}
		if(bytes==PCSX_16_BITS)
		{
			if(value<65536)
				return TRUE;
			else return FALSE;
		}
		if(bytes==PCSX_24_BITS)
		{
			if(value<16777216)
				return TRUE;
			else return FALSE;
		}
		//if it reads in, it's a valid 32-bit unsigned!
		return TRUE;
	}
	else
	{
		if(bytes==PCSX_8_BITS)
		{
			if((int32)value<128 && (int32)value >= -128)
				return TRUE;
			else return FALSE;
		}
		if(bytes==PCSX_16_BITS)
		{
			if((int32)value<32768 && (int32)value >= -32768)
				return TRUE;
			else return FALSE;
		}
		if(bytes==PCSX_24_BITS)
		{
			if((int32)value<8388608 && (int32)value >= -8388608)
				return TRUE;
			else return FALSE;
		}
		//should be handled by sscanf
		return TRUE;
	}
}

//void PCSXSearchForChange (SCheatData *d, PCSXCheatComparisonType cmp,PCSXCheatDataSize size, bool8 is_signed, bool8 update)
void PCSXSearchForChange (PCSXCheatComparisonType cmp,PCSXCheatDataSize size, uint8 is_signed, uint8 update)
{
	int l;

	switch (size)
	{
	case PCSX_8_BITS: l = 0; break;
	case PCSX_16_BITS: l = 1; break;
	case PCSX_24_BITS: l = 2; break;
	default:
	case PCSX_32_BITS: l = 3; break;
	}

	int i;
	if (is_signed)
	{
		for (i = 0; i < 0x200000 - l; i++)
		{
			if (TEST_BIT (Cheat.ALL_BITS, i) &&
				_C(cmp, _DS(size, Cheat.RAM, i), _DS(size, Cheat.CRAM, i)))
			{
				if (update)
					Cheat.CRAM [i] = Cheat.RAM [i];
			}
			else
				BIT_CLEAR (Cheat.ALL_BITS, i);
		}
	}
	else
	{
		for (i = 0; i < 0x200000 - l; i++)
		{
			if (TEST_BIT (Cheat.ALL_BITS, i) &&
				_C(cmp, _D(size, Cheat.RAM, i), _D(size, Cheat.CRAM, i)))
			{
				if (update)
					Cheat.CRAM [i] = Cheat.RAM [i];
			}
			else
				BIT_CLEAR (Cheat.ALL_BITS, i);
		}
	}
	for (i = 0x200000 - l; i < 0x200000; i++)
		BIT_CLEAR (Cheat.ALL_BITS, i);
//	for (i = 0x10000 - l; i < 0x10000; i++)
//		BIT_CLEAR (Cheat.SRAM_BITS, i);
}

void PCSXSearchForValue (PCSXCheatComparisonType cmp,
                        PCSXCheatDataSize size, uint32 value,
                        uint8 is_signed, uint8 update)
{
	int l;

	switch (size)
	{
	case PCSX_8_BITS: l = 0; break;
	case PCSX_16_BITS: l = 1; break;
	case PCSX_24_BITS: l = 2; break;
	default:
	case PCSX_32_BITS: l = 3; break;
	}

	int i;

	if (is_signed)
	{
		for (i = 0; i < 0x200000 - l; i++)
		{
			if (TEST_BIT (Cheat.ALL_BITS, i) &&
				_C(cmp, _DS(size, Cheat.RAM, i), (int32) value))
			{
				if (update)
					Cheat.CRAM [i] = Cheat.RAM [i];
			}
			else
				BIT_CLEAR (Cheat.ALL_BITS, i);
		}
	}
	else
	{
		for (i = 0; i < 0x200000 - l; i++)
		{
			if (TEST_BIT (Cheat.ALL_BITS, i) &&
				_C(cmp, _D(size, Cheat.RAM, i), value))
			{
				if (update)
					Cheat.CRAM [i] = Cheat.RAM [i];
			}
			else
				BIT_CLEAR (Cheat.ALL_BITS, i);
		}
	}
	for (i = 0x200000 - l; i < 0x200000; i++)
		BIT_CLEAR (Cheat.ALL_BITS, i);
//	for (i = 0x10000 - l; i < 0x10000; i++)
//		BIT_CLEAR (Cheat.SRAM_BITS, i);
}

#define ITEM_QUERY(a, b, c, d, e)  ZeroMemory(&a, sizeof(LV_ITEM)); \
						a.iItem= ListView_GetSelectionMark(GetDlgItem(hDlg, b)); \
						a.iSubItem=c; \
						a.mask=LVIF_TEXT; \
						a.pszText=d; \
						a.cchTextMax=e; \
						ListView_GetItem(GetDlgItem(hDlg, b), &a);

void PCSXSearchForAddress (PCSXCheatComparisonType cmp,
                          PCSXCheatDataSize size, uint32 value, uint8 update)
{
	int l;

	switch (size)
	{
	case PCSX_8_BITS: l = 0; break;
	case PCSX_16_BITS: l = 1; break;
	case PCSX_24_BITS: l = 2; break;
	default:
	case PCSX_32_BITS: l = 3; break;
	}

	int i;

	{

		for (i = 0; i < 0x200000 - l; i++)
		{
			if (TEST_BIT (Cheat.ALL_BITS, i) &&
				_C(cmp, i, (int)value))
			{
				if (update)
					Cheat.CRAM [i] = Cheat.RAM [i];
			}
			else
				BIT_CLEAR (Cheat.ALL_BITS, i);
		}
	}
	for (i = 0x200000 - l; i < 0x200000; i++)
		BIT_CLEAR (Cheat.ALL_BITS, i);
//	for (i = 0x10000 - l; i < 0x10000; i++)
//		BIT_CLEAR (Cheat.SRAM_BITS, i);
}

static inline int CheatCount(int byteSub)
{
	int a, b=0;
	for(a=0;a<0x200000-byteSub;a++)
	{
		if(TEST_BIT(Cheat.ALL_BITS, a))
			b++;
	}
	return b;
}

INT_PTR CALLBACK DlgCheatSearchAdd(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static struct ICheat* new_cheat;
	int ret=-1;
	static HBITMAP hBmp;
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			char buf [12];
			new_cheat=(struct ICheat*)lParam;
			hBmp=(HBITMAP)LoadImage(NULL, TEXT("Gary.bmp"), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
			sprintf(buf, "%06X", new_cheat->address);
			SetDlgItemText(hDlg, IDC_NC_ADDRESS, buf);
			switch(new_cheat->format)
			{
			default:
			case 1://UNSIGNED
				memset(buf,0,12);
				sprintf(buf, "%u", new_cheat->new_val);
				SetDlgItemText(hDlg, IDC_NC_CURRVAL, buf);
				memset(buf,0,12);
				sprintf(buf, "%u", new_cheat->saved_val);
				SetDlgItemText(hDlg, IDC_NC_PREVVAL, buf);
				SetWindowLong(GetDlgItem(hDlg, IDC_NC_NEWVAL), GWL_STYLE, ES_NUMBER |GetWindowLong(GetDlgItem(hDlg, IDC_NC_NEWVAL),GWL_STYLE));
				SetWindowLong(GetDlgItem(hDlg, IDC_NC_CURRVAL), GWL_STYLE, ES_NUMBER |GetWindowLong(GetDlgItem(hDlg, IDC_NC_CURRVAL),GWL_STYLE));
				SetWindowLong(GetDlgItem(hDlg, IDC_NC_PREVVAL), GWL_STYLE, ES_NUMBER |GetWindowLong(GetDlgItem(hDlg, IDC_NC_PREVVAL),GWL_STYLE));
				if(new_cheat->size==1)
				{
					SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 3, 0);
					SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 3, 0);
					SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 3, 0);

				}
				if(new_cheat->size==2)
				{
					SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 5, 0);
					SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 5, 0);
					SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 5, 0);

				}
				if(new_cheat->size==3)
				{
					SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 8, 0);
					SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 8, 0);
					SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 8, 0);

				}
				if(new_cheat->size==4)
				{
					SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 10, 0);
					SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 10, 0);
					SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 10, 0);

				}
				break;
			case 3:
				{
					char formatstring[10];
					sprintf(formatstring, "%%%02dX",new_cheat->size*2);
					memset(buf,0,12);
					sprintf(buf, formatstring, new_cheat->new_val);
					SetDlgItemText(hDlg, IDC_NC_CURRVAL, buf);
					memset(buf,0,12);
					sprintf(buf, formatstring, new_cheat->saved_val);
					SetDlgItemText(hDlg, IDC_NC_PREVVAL, buf);
					SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, new_cheat->size*2, 0);
					SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, new_cheat->size*2, 0);
					SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, new_cheat->size*2, 0);

				}
				break; //HEX
			case 2:
			memset(buf,0,12);
			sprintf(buf, "%d", new_cheat->new_val);
			SetDlgItemText(hDlg, IDC_NC_CURRVAL, buf);
			memset(buf,0,12);
			sprintf(buf, "%d", new_cheat->saved_val);
			SetDlgItemText(hDlg, IDC_NC_PREVVAL, buf);
			if(new_cheat->size==1)
			{
				//-128
				SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 4, 0);
			}
			if(new_cheat->size==2)
			{
				//-32768
				SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 6, 0);
			}
			if(new_cheat->size==3)
			{
				//-8388608
				SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 8, 0);
				SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 8, 0);
				SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 8, 0);
			}
			if(new_cheat->size==4)
			{
				//-2147483648
				SendDlgItemMessage(hDlg, IDC_NC_CURRVAL,EM_SETLIMITTEXT, 11, 0);
				SendDlgItemMessage(hDlg, IDC_NC_PREVVAL,EM_SETLIMITTEXT, 11, 0);
				SendDlgItemMessage(hDlg, IDC_NC_NEWVAL,EM_SETLIMITTEXT, 11, 0);
			}
			break;
			}
		}
			return TRUE;
		case WM_PAINT:
		{
		PAINTSTRUCT ps;
		BeginPaint (hDlg, &ps);
		if(hBmp)
		{
			BITMAP bmp;
			ZeroMemory(&bmp, sizeof(BITMAP));
			RECT r;
			GetClientRect(hDlg, &r);
			HDC hdc=GetDC(hDlg);
			HDC hDCbmp=CreateCompatibleDC(hdc);
			GetObject(hBmp, sizeof(BITMAP), &bmp);
			HBITMAP hOldBmp=(HBITMAP)SelectObject(hDCbmp, hBmp);
			StretchBlt(hdc, 0,0,r.right,r.bottom,hDCbmp,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
			SelectObject(hDCbmp, hOldBmp);
			DeleteDC(hDCbmp);
			ReleaseDC(hDlg, hdc);
		}

		EndPaint (hDlg, &ps);
		}
		return TRUE;
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDOK:
				{
					int ret = 0;
					char buf[23];
					int temp=new_cheat->size;
					PCSXCheatDataSize tmp = PCSX_8_BITS;
					ZeroMemory(new_cheat, sizeof(struct SCheat));
					new_cheat->size=temp;
					GetDlgItemText(hDlg, IDC_NC_ADDRESS, buf, 7);
//					ScanAddress(buf, new_cheat->address);
					new_cheat->address = ScanAddress(buf);

					if(temp==1)
						tmp=PCSX_8_BITS;
					if(temp==2)
						tmp=PCSX_16_BITS;
					if(temp==3)
						tmp=PCSX_24_BITS;
					if(temp==4)
						tmp=PCSX_32_BITS;

					if(0!=GetDlgItemText(hDlg, IDC_NC_NEWVAL, buf, 12))
					{
						if(new_cheat->format==2)
							ret=sscanf(buf, "%d", &new_cheat->new_val);
						else if(new_cheat->format==1)
							ret=sscanf(buf, "%u", &new_cheat->new_val);
						else if(new_cheat->format==3)
							ret=sscanf(buf, "%x", &new_cheat->new_val);

						if(ret!=1 || !TestRange(new_cheat->format, tmp, new_cheat->new_val))
						{
							MessageBox(hDlg, SEARCH_ERR_INVALIDNEWVALUE, SEARCH_TITLE_RANGEERROR, MB_OK);
							return TRUE;
						}


						if(0==GetDlgItemText(hDlg, IDC_NC_CURRVAL, buf, 12))
							new_cheat->saved=FALSE;
						else
						{
							int i;
							if(new_cheat->format==2)
								ret=sscanf(buf, "%d", &i);
							else if(new_cheat->format==1)
								ret=sscanf(buf, "%u", &i);
							else if(new_cheat->format==3)
								ret=sscanf(buf, "%x", &i);

							if(ret!=1 || !TestRange(new_cheat->format, tmp, i))
							{
								MessageBox(hDlg, SEARCH_ERR_INVALIDCURVALUE, SEARCH_TITLE_RANGEERROR, MB_OK);
								return TRUE;
							}


							new_cheat->saved_val=i;
							new_cheat->saved=TRUE;
						}
						GetDlgItemText(hDlg, IDC_NC_DESC, new_cheat->name, 23);

						new_cheat->enabled=TRUE;
						int p;
						for(p=0; p<new_cheat->size; p++)
						{
							PCSXAddCheat(new_cheat->enabled, new_cheat->saved_val, new_cheat->address +p, (new_cheat->new_val>>(8*p)));
							strcpy(Cheat.c[Cheat.num_cheats-1].name, new_cheat->name);
						}
//						PCSXAddCheat(new_cheat->enabled,new_cheat->saved_val,new_cheat->address,new_cheat->new_val);
//						strcpy(Cheat.c[Cheat.num_cheats-1].name,new_cheat->name);
						ret=0;
					}
				}

			case IDCANCEL:
				EndDialog(hDlg, ret);
				if(hBmp)
				{
					DeleteObject(hBmp);
					hBmp=NULL;
				}
				return TRUE;
			default: break;
			}
		}
	default: return FALSE;
	}
}

INT_PTR CALLBACK DlgCheatSearch(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP hBmp;
	static PCSXCheatDataSize bytes;
	static int val_type;
	static int use_entered;
	static PCSXCheatComparisonType comp_type;
	switch(msg)
	{
		case WM_INITDIALOG:
		{
			if(val_type==0)
				val_type=1;
			hBmp=(HBITMAP)LoadImage(NULL, TEXT("Raptor.bmp"), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
			ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_ADDYS), LVS_EX_FULLROWSELECT);

			//defaults
			SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_CHECKED, 0);
			if(!use_entered)
				SendDlgItemMessage(hDlg, IDC_PREV, BM_SETCHECK, BST_CHECKED, 0);
			else if(use_entered==1)
			{
				SendDlgItemMessage(hDlg, IDC_ENTERED, BM_SETCHECK, BST_CHECKED, 0);
				EnableWindow(GetDlgItem(hDlg, IDC_VALUE_ENTER), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_ENTER_LABEL), TRUE);
			}
			else if(use_entered==2)
			{
				SendDlgItemMessage(hDlg, IDC_ENTEREDADDRESS, BM_SETCHECK, BST_CHECKED, 0);
				EnableWindow(GetDlgItem(hDlg, IDC_VALUE_ENTER), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_ENTER_LABEL), TRUE);
			}
			SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);

			if(comp_type==PCSX_GREATER_THAN)
			{
				SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_GREATER_THAN, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if(comp_type==PCSX_LESS_THAN_OR_EQUAL)
			{
				SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_LESS_THAN_EQUAL, BM_SETCHECK, BST_CHECKED, 0);

			}
			else if(comp_type==PCSX_GREATER_THAN_OR_EQUAL)
			{
				SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_GREATER_THAN_EQUAL, BM_SETCHECK, BST_CHECKED, 0);

			}
			else if(comp_type==PCSX_EQUAL)
			{
				SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_EQUAL, BM_SETCHECK, BST_CHECKED, 0);

			}
			else if(comp_type==PCSX_NOT_EQUAL)
			{
				SendDlgItemMessage(hDlg, IDC_LESS_THAN, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_NOT_EQUAL, BM_SETCHECK, BST_CHECKED, 0);

			}

			if(val_type==2)
			{
				SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);

			}
			else if(val_type==3)
			{
				SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
			}

			if(bytes==PCSX_16_BITS)
			{
				SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_2_BYTE, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if(bytes==PCSX_24_BITS)
			{
				SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_3_BYTE, BM_SETCHECK, BST_CHECKED, 0);
			}
			else if(bytes==PCSX_32_BITS)
			{
				SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_UNCHECKED, 0);
				SendDlgItemMessage(hDlg, IDC_4_BYTE, BM_SETCHECK, BST_CHECKED, 0);
			}

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

			ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDYS),   0,   &col);

			strcpy(temp,"Curr. Value");
			ZeroMemory(&col, sizeof(LVCOLUMN));
			col.mask=LVCF_FMT|LVCF_ORDER|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
			col.fmt=LVCFMT_LEFT;
			col.iOrder=1;
			col.cx=104;
			col.cchTextMax=3;
			col.pszText=temp;
			col.iSubItem=1;

			ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDYS),    1,   &col);

			strcpy(temp,"Prev. Value");
			ZeroMemory(&col, sizeof(LVCOLUMN));
			col.mask=LVCF_FMT|LVCF_ORDER|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
			col.fmt=LVCFMT_LEFT;
			col.iOrder=2;
			col.cx=104;
			col.cchTextMax=32;
			col.pszText=temp;
			col.iSubItem=2;

			ListView_InsertColumn(GetDlgItem(hDlg,IDC_ADDYS),    2,   &col);

			{
					int l = CheatCount(bytes);
					ListView_SetItemCount (GetDlgItem(hDlg, IDC_ADDYS), l);
			}

		}
		return TRUE;

		case WM_DESTROY:
		{
			cheatSearchHWND = NULL;
//			PCSXSaveCheatFile (PCSXGetFilename (".cht", CHEAT_DIR));
			break;
		}

		case WM_PAINT:
		{
		PAINTSTRUCT ps;
		BeginPaint (hDlg, &ps);
		if(hBmp)
		{
			BITMAP bmp;
			ZeroMemory(&bmp, sizeof(BITMAP));
			RECT r;
			GetClientRect(hDlg, &r);
			HDC hdc=GetDC(hDlg);
			HDC hDCbmp=CreateCompatibleDC(hdc);
			GetObject(hBmp, sizeof(BITMAP), &bmp);
			HBITMAP hOldBmp=(HBITMAP)SelectObject(hDCbmp, hBmp);
			StretchBlt(hdc, 0,0,r.right,r.bottom,hDCbmp,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
			SelectObject(hDCbmp, hOldBmp);
			DeleteDC(hDCbmp);
			ReleaseDC(hDlg, hdc);
		}

		EndPaint (hDlg, &ps);
		}
		return TRUE;

		case WM_NOTIFY:
			{
				static int selectionMarkOverride = -1;
				static int foundItemOverride = -1;
				if(wParam == IDC_ADDYS)
				{
					NMHDR * nmh=(NMHDR*)lParam;
					if(nmh->hwndFrom == GetDlgItem(hDlg, IDC_ADDYS) && nmh->code == LVN_GETDISPINFO)
					{
						static char buf[12]; // the following code assumes this variable is static
						int i, j;
						NMLVDISPINFO * nmlvdi=(NMLVDISPINFO*)lParam;
						j=nmlvdi->item.iItem;
						j++;
						for(i=0;i<(0x200000-bytes)&& j>0;i++)
						{
							if(TEST_BIT(Cheat.ALL_BITS, i))
								j--;
						}
						if (i>=0x200000 && j!=0)
						{
							return FALSE;
						}
						i--;
						if((j=nmlvdi->item.iSubItem==0))
						{
							sprintf(buf, "%06X", i);
							nmlvdi->item.pszText=buf;
							nmlvdi->item.cchTextMax=8;
						}
						if((j=nmlvdi->item.iSubItem==1))
						{
							int q=0, r=0;
							for(r=0;r<=bytes;r++)
								q+=(Cheat.RAM[i+r])<<(8*r);
							//needs to account for size
							switch(val_type)
							{
							case 1:
								sprintf(buf, "%u", q);
								break;
							case 3:
								{
									switch(bytes)
									{
										default:
										case PCSX_8_BITS:sprintf(buf, "%02X", q&0xFF);break;
										case PCSX_16_BITS: sprintf(buf, "%04X", q&0xFFFF); break;
										case PCSX_24_BITS: sprintf(buf, "%06X", q&0xFFFFFF);break;
										case PCSX_32_BITS: sprintf(buf, "%08X", q);break;
									}
								}
								break;
							case 2:
								default:
									switch(bytes)
									{
										default:
										case PCSX_8_BITS:
											if((q-128)<0)
												sprintf(buf, "%d", q&0xFF);
											else sprintf(buf, "%d", q-256);
											break;
										case PCSX_16_BITS:
											if((q-32768)<0)
												sprintf(buf, "%d", q&0xFFFF);
											else sprintf(buf, "%d", q-65536);
											break;
										case PCSX_24_BITS:
											if((q-0x800000)<0)
												sprintf(buf, "%d", q&0xFFFFFF);
											else sprintf(buf, "%d", q-0x1000000);
											break;
	
										case PCSX_32_BITS: sprintf(buf, "%d", q);break;
									}
									break;
							}
							nmlvdi->item.pszText=buf;
							nmlvdi->item.cchTextMax=4;
						}
						if((j=nmlvdi->item.iSubItem==2))
						{
							int q=0, r=0;
							for(r=0;r<=bytes;r++)
								q+=(Cheat.CRAM[i+r])<<(8*r);
							//needs to account for size
							switch(val_type)
							{
							case 1:
								sprintf(buf, "%u", q);
								break;
							case 3:
								{
									switch(bytes)
									{
										default:
										case PCSX_8_BITS:sprintf(buf, "%02X", q&0xFF);break;
										case PCSX_16_BITS: sprintf(buf, "%04X", q&0xFFFF); break;
										case PCSX_24_BITS: sprintf(buf, "%06X", q&0xFFFFFF);break;
										case PCSX_32_BITS: sprintf(buf, "%08X", q);break;
									}
									break;
								}
							case 2:
								default:
									switch(bytes)
									{
										default:
										case PCSX_8_BITS:
											if((q-128)<0)
												sprintf(buf, "%d", q&0xFF);
											else sprintf(buf, "%d", q-256);
											break;
										case PCSX_16_BITS:
											if((q-32768)<0)
												sprintf(buf, "%d", q&0xFFFF);
											else sprintf(buf, "%d", q-65536);
											break;
										case PCSX_24_BITS:
											if((q-0x800000)<0)
												sprintf(buf, "%d", q&0xFFFFFF);
											else sprintf(buf, "%d", q-0x1000000);
											break;
	
										case PCSX_32_BITS: sprintf(buf, "%d", q);break;
									}
									break;
							}
							nmlvdi->item.pszText=buf;
							nmlvdi->item.cchTextMax=4;
						}
					}
					else if(nmh->hwndFrom == GetDlgItem(hDlg, IDC_ADDYS) && (nmh->code == (UINT)LVN_ITEMACTIVATE||nmh->code == (UINT)NM_CLICK))
					{
						BOOL enable=TRUE;
						if(-1==ListView_GetSelectionMark(nmh->hwndFrom))
						{
							enable=FALSE;
						}
						EnableWindow(GetDlgItem(hDlg, IDC_C_ADD), enable);
						EnableWindow(GetDlgItem(hDlg, IDC_C_WATCH), enable);
					}
					// allow typing in an address to jump to it
					else if(nmh->hwndFrom == GetDlgItem(hDlg, IDC_ADDYS) && nmh->code == (UINT)LVN_ODFINDITEM)
					{
						LRESULT pResult;
	
						// pNMHDR has information about the item we should find
						// In pResult we should save which item that should be selected
						NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)lParam;
	
						/* pFindInfo->iStart is from which item we should search.
						We search to bottom, and then restart at top and will stop
						at pFindInfo->iStart, unless we find an item that match
						*/
	
						// Set the default return value to -1
						// That means we didn't find any match.
						pResult = -1;
	
						//Is search NOT based on string?
						if( (pFindInfo->lvfi.flags & LVFI_STRING) == 0 )
						{
							//This will probably never happend...
							return pResult;
						}
	
						//This is the string we search for
						LPCSTR searchstr = pFindInfo->lvfi.psz;
	
						int startPos = pFindInfo->iStart;
						//Is startPos outside the list (happens if last item is selected)
						if(startPos >= ListView_GetItemCount(GetDlgItem(hDlg,IDC_ADDYS)))
							startPos = 0;
	
						int currentPos, addrPos;
						for(addrPos=0,currentPos=0;addrPos<(0x200000-bytes)&&currentPos<startPos;addrPos++)
						{
							if(TEST_BIT(Cheat.ALL_BITS, addrPos))
								currentPos++;
						}
	
						pResult=currentPos;
	
						if (addrPos>=0x200000 && addrPos!=0)
							break;
	
						// ignore leading 0's
						while(searchstr[0] == '0' && searchstr[1] != '\0')
							searchstr++;
	
						int searchNum = 0;
	
	//					ScanAddress(searchstr, searchNum);
						searchNum = ScanAddress(searchstr);
	
	
	//					if (searchstr[0] != '7')
	//						break; // all searchable addresses begin with a 7
	
						BOOL looped = FALSE;
	
						// perform search
						do
						{
	
							if(addrPos == searchNum)
							{
								// select this item and stop search
								pResult = currentPos;
								break;
							}
							else if(addrPos > searchNum)
							{
								if(looped)
								{
									pResult = currentPos;
									break;
								}
	
								// optimization: the items are ordered alphabetically, so go back to the top since we know it can't be anything further down
								currentPos = 0;
								addrPos = 0;
								while(!TEST_BIT(Cheat.ALL_BITS, addrPos))
									addrPos++;
								looped = TRUE;
								continue;
							}
	
							//Go to next item
							addrPos++;
							while(!TEST_BIT(Cheat.ALL_BITS, addrPos))
								addrPos++;
							currentPos++;
	
							//Need to restart at top?
							if(currentPos >= ListView_GetItemCount(GetDlgItem(hDlg,IDC_ADDYS)))
							{
								currentPos = 0;
								addrPos = 0;
								while(!TEST_BIT(Cheat.ALL_BITS, addrPos))
									addrPos++;
							}
	
						//Stop if back to start
						}while(currentPos != startPos);
	
						foundItemOverride = pResult;
	
						// in case previously-selected item is 0
						ListView_SetItemState (GetDlgItem(hDlg,IDC_ADDYS), 1, LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
	
						return pResult; // HACK: for some reason this selects the first item instead of what it's returning... current workaround is to manually re-select this return value upon the next changed event
					}
					else if(nmh->hwndFrom == GetDlgItem(hDlg, IDC_ADDYS) && nmh->code == LVN_ITEMCHANGED)
					{
						// hack - see note directly above
						LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;
						if(lpnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED))
						{
							if(foundItemOverride != -1 && lpnmlv->iItem == 0)
							{
								ListView_SetItemState (GetDlgItem(hDlg,IDC_ADDYS), foundItemOverride, LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
								ListView_EnsureVisible (GetDlgItem(hDlg,IDC_ADDYS), foundItemOverride, FALSE);
								selectionMarkOverride = foundItemOverride;
								foundItemOverride = -1;
							}
							else
							{
								selectionMarkOverride = lpnmlv->iItem;
							}
						}
					}
				}
			}
			break;

		case WM_ACTIVATE:
			ListView_RedrawItems(GetDlgItem(hDlg, IDC_ADDYS),0, 0x200000);
			break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_LESS_THAN:
			case IDC_GREATER_THAN:
			case IDC_LESS_THAN_EQUAL:
			case IDC_GREATER_THAN_EQUAL:
			case IDC_EQUAL:
			case IDC_NOT_EQUAL:
				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_LESS_THAN))
					comp_type=PCSX_LESS_THAN;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_GREATER_THAN))
					comp_type=PCSX_GREATER_THAN;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_LESS_THAN_EQUAL))
					comp_type=PCSX_LESS_THAN_OR_EQUAL;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_GREATER_THAN_EQUAL))
					comp_type=PCSX_GREATER_THAN_OR_EQUAL;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_EQUAL))
					comp_type=PCSX_EQUAL;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_NOT_EQUAL))
					comp_type=PCSX_NOT_EQUAL;
				break;

			case IDC_1_BYTE:
			case IDC_2_BYTE:
			case IDC_3_BYTE:
			case IDC_4_BYTE:
				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_1_BYTE))
					bytes=PCSX_8_BITS;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_2_BYTE))
					bytes=PCSX_16_BITS;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_3_BYTE))
					bytes=PCSX_24_BITS;
				else bytes=PCSX_32_BITS;
				{
					int l = CheatCount(bytes);
					ListView_SetItemCount (GetDlgItem(hDlg, IDC_ADDYS), l);
				}
				break;

			case IDC_SIGNED:
			case IDC_UNSIGNED:
			case IDC_HEX:
				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_UNSIGNED))
					val_type=1;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_SIGNED))
					val_type=2;
				else val_type=3;
				ListView_RedrawItems(GetDlgItem(hDlg, IDC_ADDYS),0, 0x200000);
				break;


			case IDC_C_ADD:
				{
					// account for size
					struct ICheat cht;
//					int idx=-1;
					LVITEM lvi;
					static char buf[12]; // the following code assumes this variable is static, I think
					ZeroMemory(&cht, sizeof(struct SCheat));

					//retrieve and convert to SCheat
					if(bytes==PCSX_8_BITS)
						cht.size=1;
					else if (bytes==PCSX_16_BITS)
						cht.size=2;
					else if (bytes==PCSX_24_BITS)
						cht.size=3;
					else if (bytes==PCSX_32_BITS)
						cht.size=4;

					ITEM_QUERY(lvi, IDC_ADDYS, 0, buf, 7);

//					ScanAddress(buf, cht.address);
					cht.address = ScanAddress(buf);

					memset(buf, 0, 7);
					if(val_type==1)
					{
						ITEM_QUERY(lvi, IDC_ADDYS, 1, buf, 12);
						sscanf(buf, "%u", &cht.new_val);
						memset(buf, 0, 7);
						ITEM_QUERY(lvi, IDC_ADDYS, 2, buf, 12);
						sscanf(buf, "%u", &cht.saved_val);
					}
					else if(val_type==3)
					{
						ITEM_QUERY(lvi, IDC_ADDYS, 1, buf, 12);
						sscanf(buf, "%x", &cht.new_val);
						memset(buf, 0, 7);
						ITEM_QUERY(lvi, IDC_ADDYS, 2, buf, 12);
						sscanf(buf, "%x", &cht.saved_val);
					}
					else
					{
						ITEM_QUERY(lvi, IDC_ADDYS, 1, buf, 12);
						sscanf(buf, "%d", &cht.new_val);
						memset(buf, 0, 7);
						ITEM_QUERY(lvi, IDC_ADDYS, 2, buf, 12);
						sscanf(buf, "%d", &cht.saved_val);
					}
					cht.format=val_type;

					//invoke dialog
					if(!DialogBoxParam(gApp.hInstance, MAKEINTRESOURCE(IDD_CHEAT_FROM_SEARCH), hDlg, DlgCheatSearchAdd, (LPARAM)&cht))
					{
						int p;
						for(p=0; p<cht.size; p++)
						{
							PCSXAddCheat(TRUE, cht.saved, cht.address +p, ((cht.new_val>>(8*p))&0xFF));
							strcpy(Cheat.c[Cheat.num_cheats-1].name, cht.name);
						}
					}
				}
				break;

			case IDC_C_RESET:
				PCSXStartCheatSearch();
				{
					int l = CheatCount(bytes);
					ListView_SetItemCount (GetDlgItem(hDlg, IDC_ADDYS), l);
				}
				ListView_RedrawItems(GetDlgItem(hDlg, IDC_ADDYS),0, 0x200000);
				return TRUE;

			case IDC_C_WATCH:
				{
					uint32 address = (uint32)-1;
					char buf [12];
					LVITEM lvi;
					ITEM_QUERY(lvi, IDC_ADDYS, 0, buf, 7);
					if(lvi.iItem != -1)
					{
						address = ScanAddress(buf);
						memset(buf, 0, 7);
						sprintf(buf, "%X", address);
						AddMemWatch(buf);
					}
				}
				break;

			case IDC_REFRESHLIST:
				ListView_RedrawItems(GetDlgItem(hDlg, IDC_ADDYS),0, 0x32000);
				break;

			case IDC_ENTERED:
			case IDC_ENTEREDADDRESS:
			case IDC_PREV:
				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ENTERED))
				{
					use_entered=1;
					EnableWindow(GetDlgItem(hDlg, IDC_VALUE_ENTER), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_ENTER_LABEL), TRUE);
				}
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ENTEREDADDRESS))
				{
					use_entered=2;
					EnableWindow(GetDlgItem(hDlg, IDC_VALUE_ENTER), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_ENTER_LABEL), TRUE);
				}
				else
				{
					use_entered=0;
					EnableWindow(GetDlgItem(hDlg, IDC_VALUE_ENTER), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_ENTER_LABEL), FALSE);
				}
				return TRUE;
				break;


			case IDC_C_SEARCH:
				{
				val_type=0;

				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_LESS_THAN))
					comp_type=PCSX_LESS_THAN;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_GREATER_THAN))
					comp_type=PCSX_GREATER_THAN;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_LESS_THAN_EQUAL))
					comp_type=PCSX_LESS_THAN_OR_EQUAL;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_GREATER_THAN_EQUAL))
					comp_type=PCSX_GREATER_THAN_OR_EQUAL;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_EQUAL))
					comp_type=PCSX_EQUAL;
				else comp_type=PCSX_NOT_EQUAL;

				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_UNSIGNED))
					val_type=1;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_SIGNED))
					val_type=2;
				else val_type=3;

				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_1_BYTE))
					bytes=PCSX_8_BITS;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_2_BYTE))
					bytes=PCSX_16_BITS;
				else if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_3_BYTE))
					bytes=PCSX_24_BITS;
				else bytes=PCSX_32_BITS;

				if(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ENTERED) ||
				   BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ENTEREDADDRESS))
				{
					char buf[20];
					GetDlgItemText(hDlg, IDC_VALUE_ENTER, buf, 20);
					uint32 value;
					int ret;
					if(use_entered==2)
					{
//						ret = ScanAddress(buf, value);
//						value -= 0x7E0000;
						value = ScanAddress(buf);
						PCSXSearchForAddress (comp_type, bytes, value, FALSE);
					}
					else
					{
						if(val_type==1)
							ret=sscanf(buf, "%ul", &value);
						else if (val_type==2)
							ret=sscanf(buf, "%d", &value);
						else ret=sscanf(buf, "%x", &value);

						if(ret!=1||!TestRange(val_type, bytes, value))
						{
							MessageBox(hDlg, TEXT(SEARCH_ERR_INVALIDSEARCHVALUE), TEXT(SEARCH_TITLE_CHEATERROR), MB_OK);
							return TRUE;
						}

						PCSXSearchForValue (comp_type,bytes, value,(val_type==2), FALSE);
					}
				}
				else
				{
					PCSXSearchForChange (comp_type,bytes, (val_type==2), FALSE);
				}
				int l = CheatCount(bytes);
				ListView_SetItemCount (GetDlgItem(hDlg, IDC_ADDYS), l);
				}

				// if non-modal, update "Prev. Value" column after Search
				if(cheatSearchHWND)
				{
					CopyMemory(Cheat.CRAM, Cheat.RAM, 0x200000);
				}


				ListView_RedrawItems(GetDlgItem(hDlg, IDC_ADDYS),0, 0x200000);
				return TRUE;
				break;

			case IDOK:
				CopyMemory(Cheat.CRAM, Cheat.RAM, 0x200000);
				/* fall through */
			case IDCANCEL:
				if(cheatSearchHWND)
					DestroyWindow(hDlg);
				else
					EndDialog(hDlg, 0);
				if(hBmp)
				{
					DeleteObject(hBmp);
					hBmp=NULL;
				};
				return TRUE;
			default: break;
			}
		}
		default: return FALSE;
	}
	return FALSE;
}

void CreateMemSearch()
{
	if(!cheatSearchHWND) // create and show non-modal cheat search window
	{
		cheatSearchHWND = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_CHEAT_SEARCH), gApp.hWnd, DlgCheatSearch);
		ShowWindow(cheatSearchHWND, SW_SHOW);
	}
	else // already open so just reactivate the window
	{
		SetActiveWindow(cheatSearchHWND);
	}
//	DialogBox(gApp.hInstance,MAKEINTRESOURCE(IDD_CHEAT_SEARCH),gApp.hWnd,DlgCheatSearch);
}

void UpdateMemSearch()
{
	if(cheatSearchHWND)
		SendMessage(cheatSearchHWND, WM_COMMAND, (WPARAM)(IDC_REFRESHLIST),(LPARAM)(NULL));
}
