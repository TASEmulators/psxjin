#include "resource.h"
#include "PsxCommon.h"

#include <windows.h>
#include <wingdi.h>

extern AppData gApp;

static HDC hdc;
static HWND hwndMemWatch=0;
static char addresses[24][16];
static char labels[24][24];
static int NeedsInit = 1;
char *MemWatchDir = 0;

HFONT hFixedFont=NULL;

static char *U8ToStr(uint8 a)
{
 static char TempArray[8];
 TempArray[0] = '0' + a/100;
 TempArray[1] = '0' + (a%100)/10;
 TempArray[2] = '0' + (a%10);
 TempArray[3] = 0;
 return TempArray;
}

static uint32 FastStrToU32(char* s)
{
	uint32 v=0;

	sscanf(s,"%x",&v);

	return v;
}

static char *U16ToDecStr(uint16 a)
{
 static char TempArray[8];
 TempArray[0] = '0' + a/10000;
 TempArray[1] = '0' + (a%10000)/1000;
 TempArray[2] = '0' + (a%1000)/100;
 TempArray[3] = '0' + (a%100)/10;
 TempArray[4] = '0' + (a%10);
 TempArray[5] = 0;
 return TempArray;
}


static char *U16ToHexStr(uint16 a)
{
 static char TempArray[8];
 TempArray[0] = a/4096 > 9?'A'+a/4096-10:'0' + a/4096;
 TempArray[1] = (a%4096)/256 > 9?'A'+(a%4096)/256 - 10:'0' + (a%4096)/256;
 TempArray[2] = (a%256)/16 > 9?'A'+(a%256)/16 - 10:'0' + (a%256)/16;
 TempArray[3] = a%16 > 9?'A'+(a%16) - 10:'0' + (a%16);
 TempArray[4] = 0;
 return TempArray;
}

static char *U8ToHexStr(uint8 a)
{
 static char TempArray[8];
 TempArray[0] = a/16 > 9?'A'+a/16 - 10:'0' + a/16;
 TempArray[1] = a%16 > 9?'A'+(a%16) - 10:'0' + (a%16);
 TempArray[2] = 0;
 return TempArray;
}

static const int MW_ADDR_Lookup[] = {
	MW_ADDR00,MW_ADDR01,MW_ADDR02,MW_ADDR03,
	MW_ADDR04,MW_ADDR05,MW_ADDR06,MW_ADDR07,
	MW_ADDR08,MW_ADDR09,MW_ADDR10,MW_ADDR11,
	MW_ADDR12,MW_ADDR13,MW_ADDR14,MW_ADDR15,
	MW_ADDR16,MW_ADDR17,MW_ADDR18,MW_ADDR19,
	MW_ADDR20,MW_ADDR21,MW_ADDR22,MW_ADDR23
};
#define MWNUM sizeof(MW_ADDR_Lookup)/sizeof(MW_ADDR_Lookup[0])

static int yPositions[MWNUM];
static int xPositions[MWNUM];

struct MWRec
{
	int valid, twobytes, hex;
	uint32 addr;
};

static struct MWRec mwrecs[MWNUM];

static int MWRec_findIndex(WORD ctl)
{
	int i;
	for(i=0;i<MWNUM;i++)
		if(MW_ADDR_Lookup[i] == ctl)
			return i;
	return -1;
}

void MWRec_parse(WORD ctl,int changed)
{
	char TempArray[16];
	GetDlgItemText(hwndMemWatch,ctl,TempArray,16);
	TempArray[15]=0;

	mwrecs[changed].valid = mwrecs[changed].hex = mwrecs[changed].twobytes = 0;
	switch(TempArray[0])
	{
		case 0:
			break;
		case '!':
			mwrecs[changed].twobytes=1;
			mwrecs[changed].valid = 1;
			mwrecs[changed].addr=FastStrToU32(TempArray+1);
			break;
		case 'x':
			mwrecs[changed].hex = 1;
			mwrecs[changed].valid = 1;
			mwrecs[changed].addr=FastStrToU32(TempArray+1);
			break;
		case 'X':
			mwrecs[changed].hex = mwrecs[changed].twobytes = 1;
			mwrecs[changed].valid = 1;
			mwrecs[changed].addr=FastStrToU32(TempArray+1);
			break;
		default:
			mwrecs[changed].valid = 1;
			mwrecs[changed].addr=FastStrToU32(TempArray);
			break;
		}
}

void UpdateMemWatch()
{
	int i;
	if(hwndMemWatch)
	{
		SetTextColor(hdc,RGB(0,0,0));
		SetBkColor(hdc,GetSysColor(COLOR_3DFACE));

		for(i = 0; i < MWNUM; i++)
		{
			struct MWRec mwrec;
			memcpy(&mwrec,&mwrecs[i],sizeof(mwrecs[i]));

			char* text;
			if(mwrec.valid)
			{
				if(mwrec.hex)
				{
					if(mwrec.twobytes)
					{
						text = U16ToHexStr(psxMs16(mwrec.addr));
					}
					else
					{
						text = U8ToHexStr(psxMs8(mwrec.addr));
					}
				}
				else
				{
					if(mwrec.twobytes)
					{
						text = U16ToDecStr(psxMs16(mwrec.addr));
					}
					else
					{
						text = U8ToStr(psxMs8(mwrec.addr));
					}
				}
				int len = strlen(text);
				int j;
				for(j=len;j<5;j++)
					text[j] = ' ';
				text[5] = 0;
			}
			else
			{
				text = "-    ";
			}

			MoveToEx(hdc,xPositions[i],yPositions[i],NULL);
			TextOut(hdc,0,0,text,strlen(text));
		}
	}
}

//Save labels/addresses so next time dialog is opened,
//you don't lose what you've entered.
static void SaveStrings()
{
	int i;
	for(i=0;i<24;i++)
	{
		GetDlgItemText(hwndMemWatch,1001+i*3,addresses[i],16);
		GetDlgItemText(hwndMemWatch,1000+i*3,labels[i],24);
	}
}

//replaces spaces with a dummy character
static void TakeOutSpaces(int i)
{
	int j;
	for(j=0;j<16;j++)
	{
		if(addresses[i][j] == ' ') addresses[i][j] = '|';
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
	for(;j<24;j++)
	{
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
}

//replaces dummy characters with spaces
static void PutInSpaces(int i)
{
	int j;
	for(j=0;j<16;j++)
	{
		if(addresses[i][j] == '|') addresses[i][j] = ' ';
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
	for(;j<24;j++)
	{
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
}

//Saves all the addresses and labels to disk
static void SaveMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=gApp.hInstance;
	ofn.lpstrTitle="Save Memory Watch As...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=".\\";
	if(GetSaveFileName(&ofn))
	{
		int i;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}

		//quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		//add .txt if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 't';
			nameo[i+2] = 'x';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}

		SaveStrings();
		FILE *fp=fopen(nameo,"w");
		for(i=0;i<24;i++)
		{
			//Use dummy strings to fill empty slots
			if(labels[i][0] == 0)
			{
				labels[i][0] = '|';
				labels[i][1] = 0;
			}
			if(addresses[i][0] == 0)
			{
				addresses[i][0] = '|';
				addresses[i][1] = 0;
			}
			//spaces can be a problem for scanf so get rid of them
			TakeOutSpaces(i);
			fprintf(fp, "%s %s\n", addresses[i], labels[i]);
			PutInSpaces(i);
		}
		fclose(fp);
	}
}

//Loads a previously saved file
static void LoadMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=gApp.hInstance;
	ofn.lpstrTitle="Load Memory Watch...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=".\\";
	
	if(GetOpenFileName(&ofn))
	{
		int i,j;
		
		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}
		
		FILE *fp=fopen(nameo,"r");
		for(i=0;i<24;i++)
		{
			fscanf(fp, "%s ", nameo); //re-using nameo--bady style :P
			for(j = 0; j < 16; j++)
				addresses[i][j] = nameo[j];
			fscanf(fp, "%s\n", nameo);
			for(j = 0; j < 24; j++)
				labels[i][j] = nameo[j];
			
			//Replace dummy strings with empty strings
			if(addresses[i][0] == '|')
			{
				addresses[i][0] = 0;
			}
			if(labels[i][0] == '|')
			{
				labels[i][0] = 0;
			}
			PutInSpaces(i);
			
			addresses[i][15] = 0;
			labels[i][23] = 0; //just in case

			SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
		}
		fclose(fp);
	}
	UpdateMemWatch();
}

static BOOL CALLBACK MemWatchCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int kLabelControls[] = {MW_ValueLabel1,MW_ValueLabel2};

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hdc = GetDC(hwndDlg);
		hFixedFont = CreateFont(13,8,0,0,
		             400,FALSE,FALSE,FALSE,
		             ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK,
		             DEFAULT_QUALITY,DEFAULT_PITCH,"Courier");
		SelectObject (hdc, hFixedFont);
		SetTextAlign(hdc,TA_UPDATECP | TA_TOP | TA_LEFT);
		//find the positions where we should draw string values
		int i;
		for(i=0;i<MWNUM;i++) {
			int col=0;
			if(i>=MWNUM/2)
				col=1;
			RECT r;
			uint8* rPtr = (uint8*)&r;
			GetWindowRect(GetDlgItem(hwndDlg,MW_ADDR_Lookup[i]),&r);
			ScreenToClient(hwndDlg,(LPPOINT)rPtr);
			ScreenToClient(hwndDlg,(LPPOINT)&r.right);
			yPositions[i] = r.top;
			yPositions[i] += ((r.bottom-r.top)-13)/2; //vertically center
			GetWindowRect(GetDlgItem(hwndDlg,kLabelControls[col]),&r);
			ScreenToClient(hwndDlg,(LPPOINT)rPtr);
			xPositions[i] = r.left;
		}
		break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hwndDlg, &ps);
		EndPaint(hwndDlg, &ps);
		UpdateMemWatch();
		break;
	}
	case WM_CLOSE:
	case WM_QUIT:
		SaveStrings();
		DeleteObject(hdc);
		DestroyWindow(hwndMemWatch);
		hwndMemWatch=0;
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		
		case EN_CHANGE:
			{
				//the contents of an address box changed. re-parse it.
				//first, find which address changed
				int changed = MWRec_findIndex(LOWORD(wParam));
				if(changed==-1) break;
				MWRec_parse(LOWORD(wParam),changed);
				UpdateMemWatch();
				break;
			}
			
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case 101: //Save button clicked
//				StopSound();
				SaveMemWatch();
				break;			
			case 102: //Load button clicked
//				StopSound();
				LoadMemWatch();
				break;
			case 103: //Clear button clicked
//				StopSound();
				if(MessageBox(hwndMemWatch, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					int i;
					for(i=0;i<24;i++)
					{
						addresses[i][0] = 0;
						labels[i][0] = 0;
						SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
						SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
					}
					UpdateMemWatch();
				}
				break;
			default:
				break;
			}
		}

		if(!(wParam>>16)) //Close button clicked
		{
			switch(wParam&0xFFFF)
			{
			case 1:
				SaveStrings();
				DestroyWindow(hwndMemWatch);
				hwndMemWatch=0;
				break;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

//Open the Memory Watch dialog
void CreateMemWatch()
{
	if(NeedsInit) //Clear the strings
	{
		NeedsInit = 0;
		int i,j;
		for(i=0;i<24;i++)
		{
			for(j=0;j<24;j++)
			{
				addresses[i][j] = 0;
				labels[i][j] = 0;
			}
		}
	}

	if(hwndMemWatch) //If already open, give focus
	{
		SetFocus(hwndMemWatch);
		return;
	}
	
	//Create
	//hwndMemWatch=CreateDialog(gApp.hInstance,"MEMWATCH",parent,MemWatchCallB);
	hwndMemWatch=CreateDialog(gApp.hInstance,MAKEINTRESOURCE(MEMWATCH),NULL,MemWatchCallB);
	UpdateMemWatch();

	//Initialize values to previous entered addresses/labels
	{
		int i;
		for(i = 0; i < 24; i++)
		{
			SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
		}
	}
}

void AddMemWatch(char memaddress[32])
{
	char TempArray[32];
	int i;
	CreateMemWatch();
	for(i = 0; i < MWNUM; i++)
	{
		GetDlgItemText(hwndMemWatch,MW_ADDR_Lookup[i],TempArray,32);
		if (TempArray[0] == 0)
		{
			SetDlgItemText(hwndMemWatch,MW_ADDR_Lookup[i],memaddress);
			break;
		}
	}
}
