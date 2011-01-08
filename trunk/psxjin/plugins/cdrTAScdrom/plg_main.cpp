// Plugin Defs
#include "Locals.h"
#include <conio.h>

//#define	USE_FPSE_INTERFACE

HINSTANCE	hInstance = (HINSTANCE)-1;	// Handle to EXE hInstance
HANDLE	hProcessHeap = (HANDLE)-1;	// Handle to Process Heap

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// FPSE SDK
#include "type.h"
#include "sdk.h"
#include "Win32Def.h"
// PSEMU SDK
#include "PSEmu Plugin Defs.h"
typedef struct {
    DWORD	Type;			// cdrom type: 0x00 = unknown, 0x01 = data, 0x02 = audio, 0xff = no cdrom
    DWORD	Status;			// drive status: 0x00 = unknown, 0x01 = error, 0x04 = seek error, 0x10 = shell open, 0x20 = reading, 0x40 = seeking, 0x80 = playing
    MSF		Time;			// current playing time
} CdrStat;
extern long CALLBACK	CDRgetStatus(CdrStat *stat);

static void dump(UINT8 *data, int len)
{
	int   i,j;
	UINT8 c;

	for(i=0;i<len;i++)
	{
		if ((i&15)==0) printf("%04x:",i & ~15);
		printf("%02x ",data[i]);
		if ((i&15)==15) {
			for (j=0; j<16; j++)
			{
				c = data[(i & ~15)+j];
				if (c > 31 && c < 128) printf("%c",c);
								  else printf(".");
			}
			printf("\n");
		}
	}

	printf("\n");
}

//static const char *INI_Name = "./fpse.ini";
static int dummy_ReadCfg(char *Section, char *Entry, char *Value)
{
	*Value = '\0';
	//GetPrivateProfileString(Section, Entry, "", Value, 1024, INI_Name);
	return FPSE_OK;
}
static int dummy_WriteCfg(char *Section, char *Entry, char *Value)
{
	//WritePrivateProfileString(Section, Entry, Value, INI_Name);
	return FPSE_OK;
}

int main(int argc,char **argv)
{
	UINT8	param[16];
	int		i, j;
	int		res;
	char	*buf;
#ifdef	USE_FPSE_INTERFACE
	FPSEWin32	par;

	memset(&par, 0, sizeof(par));
	par.ReadCfg = dummy_ReadCfg;
	par.WriteCfg = dummy_WriteCfg;
	printf("CDRSAPU TEST (FPSE MODE)\n\n");
#else //USE_FPSE_INTERFACE
	extern int CALLBACK		CDRinit(void);
	extern int CALLBACK		CDRshutdown(void);
	extern int CALLBACK		CDRopen(void);
	extern int CALLBACK		CDRclose(void);
	extern long CALLBACK	CDRconfigure(void);
	extern long CALLBACK	CDRabout(void);
	extern int CALLBACK		CDRtest(void);
	extern long CALLBACK	CDRgetTN(unsigned char *result);
	extern long CALLBACK	CDRgetTD(unsigned char track, unsigned char *result);
	extern long CALLBACK	CDRreadTrack(unsigned char *param);
	extern unsigned char * CALLBACK	CDRgetBuffer(void);
	extern char CALLBACK	CDRgetDriveLetter(void);
	extern long CALLBACK	CDRplay(unsigned char *param);
	extern long CALLBACK	CDRstop(void);
	extern long CALLBACK	CDRgetStatus(CdrStat *stat);
	extern unsigned char * CALLBACK CDRgetBufferSub(void);

	printf("CDRSAPU TEST (PSEMU MODE)\n\n");
#endif//USE_FPSE_INTERFACE

	hInstance = GetModuleHandle(NULL);
	hProcessHeap = GetProcessHeap();

#if 1
#ifdef	USE_FPSE_INTERFACE
	CD_Configure((UINT32 *)&par);
#else //USE_FPSE_INTERFACE
	CDRconfigure();
	CDRabout();
#endif//USE_FPSE_INTERFACE
#endif

	printf("Inquiry drives..\n");
//	scan_AvailDrives();

	printf("Init...\n");
#if 1
#ifdef	USE_FPSE_INTERFACE
	if ((res = CD_Open((UINT32 *)&par)) != FPSE_OK)
		return 1;
#else //USE_FPSE_INTERFACE
	if ((res = CDRinit()) != PSE_CDR_ERR_SUCCESS ||
		(res = CDRopen()) != PSE_CDR_ERR_SUCCESS)
		return 1;
#endif//USE_FPSE_INTERFACE
#else
	cfg_InitDefaultValue(&cfg_data);
	cfg_data.interface_type = InterfaceType_Mscdex;
	cfg_data.drive_id.haid = 0xFF;
	cfg_data.drive_id.target = 0xFF;
	cfg_data.drive_id.lun = 0xFF;
	cfg_data.drive_id.drive_letter = 0;
	cfg_data.read_mode = ReadMode_Auto;
	if (!open_Driver())
		return 1;
	plg_is_open = TRUE;
#endif

	printf("drive_id = [%d:%d:%d] %c\n", (int)cfg_data.drive_id.haid, (int)cfg_data.drive_id.target, (int)cfg_data.drive_id.lun, (char)cfg_data.drive_id.drive_letter);
	printf("if_type  = %d\n", (int)cfg_data.interface_type);
	printf("rd_mode  = %x\n", (int)cfg_data.read_mode);

/*
	char	sense_buf[1024];
	printf("MODE_SENSE 6...\n");
	memset(sense_buf, 0, sizeof(sense_buf));
	if (fn_ModeSense(MODE_SENSE_RETURN_ALL, sense_buf, 255, &cfg_data.drive_id))
	{
		MODE_PARAMETER_HEADER	*hdr6 = (MODE_PARAMETER_HEADER *)sense_buf;
		dump(sense_buf, hdr6->ModeDataLength);
	}
	printf("MODE_SENSE 10...\n");
	memset(sense_buf, 0, sizeof(sense_buf));
	if (fn_ModeSense(MODE_SENSE_RETURN_ALL, sense_buf, sizeof(sense_buf), &cfg_data.drive_id))
	{
		MODE_PARAMETER_HEADER10	*hdr10 = (MODE_PARAMETER_HEADER10 *)sense_buf;
		dump(sense_buf, (hdr10->ModeDataLength[0] << 8) | hdr10->ModeDataLength[1]);
	}
*/

#if 1
	printf("Reading TOC...\n");
#ifdef	USE_FPSE_INTERFACE
	if ((res = CD_GetTN(param)) == FPSE_OK)
	{
		j = param[2];	// last track nr.
#else //USE_FPSE_INTERFACE
	if ((res = CDRgetTN(param)) == PSE_CDR_ERR_SUCCESS)
	{
		//j = BCD2INT(param[1]);	// last track nr.
		j = param[1];	// last track nr.
#endif//USE_FPSE_INTERFACE
		for (i = 0; i <= j; i++)
		{
#ifdef	USE_FPSE_INTERFACE
			if ((res = CD_GetTD(param, i)) != FPSE_OK) {
				printf("CD_GetTD(%d) error\n", i);
				break;
			}
			printf("#%2d = %02d:%02d:%02d\n", i, param[1], param[2], param[3]);
#else //USE_FPSE_INTERFACE
			if ((res = CDRgetTD((unsigned char)i, param)) != PSE_CDR_ERR_SUCCESS) {
				printf("CDRgetTD(%d) error\n", i);
				break;
			}
			//printf("#%2d = %02d:%02d:%02d\n", i, BCD2INT(param[0]), BCD2INT(param[1]), BCD2INT(param[2]));
			printf("#%2d = %02d:%02d:%02d\n", i, param[2], param[1], param[0]);
#endif//USE_FPSE_INTERFACE
		}
#if 1
		if (j > 1)
		{
#ifndef	USE_FPSE_INTERFACE
			UINT8	play_loc[3];
			CDRgetTD(2, param);
			//play_loc[0] = BCD2INT(param[0]);
			//play_loc[1] = BCD2INT(param[1]);
			//play_loc[2] = BCD2INT(param[2]);
			play_loc[0] = param[2];
			play_loc[1] = param[1];
			play_loc[2] = param[0];
			CDRplay(play_loc);
			for (i = 0; i < 1000; i++)
			{
				CdrStat	stat;
				CDRgetStatus(&stat);
				printf("CDRgetStatus: Type = %02x, Status = %02x, Time = %02d:%02d:%02d\n", stat.Type, stat.Status, stat.Time.minute, stat.Time.second, stat.Time.frame);
				Sleep(500);
				if (kbhit() && getch() == 0x1b)
					break;
			}
			CDRstop();
#endif//USE_FPSE_INTERFACE
		}
#endif
	} else {
#ifdef	USE_FPSE_INTERFACE
		printf("CD_GetTN() error\n");
#else //USE_FPSE_INTERFACE
		printf("CDRgetTN() error\n");
#endif//USE_FPSE_INTERFACE
	}
#endif

#if 0
	for (i=0/*16*/; i<=360000; i++)
	{
		Q_SUBCHANNEL_DATA	subQbuf;
		memset(&subQbuf, 0, sizeof(subQbuf));
		res = cache_ReadSubQ(&subQbuf, i);
		if (res) {
			printf("subQ #%d:", i);
			for (j = 0; j < 16; j++)
				printf("%c%02x", ((j>=4&&j<=5)||(j>=8&&j<=9)) ? ':' : ' ', *((BYTE *)&subQbuf + j));
			printf("\n");
		} else {
			printf("subQ %d - read error\n", i);
		}
		if (kbhit() && getch() == 0x1b)
			break;
	}
#endif

#if 1
	printf("Start reading...\n\n");
	for (i=0; i<=360000; i++)
	{	// 144956 -> start read errors
		static UINT8	sync_field_pattern[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

#ifdef	USE_FPSE_INTERFACE
		INT2MSF((MSF *)param, i);
		buf = CD_Read(param);
		if ((res = CD_Wait()) == FPSE_OK)
#else //USE_FPSE_INTERFACE
		INT2MSFBCD((MSF *)param, i);
		res = CDRreadTrack(param);
		buf = (char *)CDRgetBuffer();
		if (res == PSE_CDR_ERR_SUCCESS)
#endif//USE_FPSE_INTERFACE
		{
			if (memcmp(buf-12, sync_field_pattern, 12))
				__asm int 3
#ifdef	USE_FPSE_INTERFACE
			if (buf[0] != INT2BCD(param[0]) || buf[1] != INT2BCD(param[1]) || buf[2] != INT2BCD(param[2]))
#else //USE_FPSE_INTERFACE
			if (buf[0] != param[0] || buf[1] != param[1] || buf[2] != param[2])
#endif//USE_FPSE_INTERFACE
				__asm int 3
			//printf("frame %d:\n",i);
			//dump(buf,RAW_SECTOR_SIZE);
			if ((i & 15) == 0) printf(".");
		} else {
			printf("frame %d - read error\n", i);
		}
		if (kbhit() && getch() == 0x1b)
			break;
	}
#endif

#ifdef	USE_FPSE_INTERFACE
	CD_Close();
#else //USE_FPSE_INTERFACE
	CDRclose();
	CDRshutdown();
#endif//USE_FPSE_INTERFACE

	return 0;
}

