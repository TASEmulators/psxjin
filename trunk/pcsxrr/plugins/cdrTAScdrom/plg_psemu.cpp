// Plugin Defs
#include "Locals.h"
#include "resource.h"
// PSEMU SDK
#include "PSEmu Plugin Defs.h"

// PSEMU plugin interface structs (not included in 'PSEmu Plugin Defs.h'):
typedef struct {
    DWORD	Type;			// cdrom type
// Type:	0x00 = unknown, 0x01 = data, 0x02 = audio, 0xff = no cdrom
    DWORD	Status;			// drive status
// Status:	0x00 = unknown, 0x01 = error, 0x04 = seek error,
//			0x10 = shell open, 0x20 = reading, 0x40 = seeking, 0x80 = playing
    MSF		Time;			// current playing time (M,S,F)
} CdrStat;

// PSEMU plugin interface functions:
unsigned long CALLBACK	PSEgetLibType(void);
char * CALLBACK	PSEgetLibName(void);
unsigned long CALLBACK	PSEgetLibVersion(void);
int CALLBACK	CDRinit(void);
int CALLBACK	CDRshutdown(void);
int CALLBACK	CDRopen(void);
int CALLBACK	CDRclose(void);
long CALLBACK	CDRconfigure(void);
long CALLBACK	CDRabout(void);
int CALLBACK	CDRtest(void);
long CALLBACK	CDRgetTN(unsigned char *result);
long CALLBACK	CDRgetTD(unsigned char track, unsigned char *result);
long CALLBACK	CDRreadTrack(unsigned char *param);
unsigned char * CALLBACK	CDRgetBuffer(void);
char CALLBACK	CDRgetDriveLetter(void);
long CALLBACK	CDRplay(unsigned char *param);
long CALLBACK	CDRstop(void);
long CALLBACK	CDRgetStatus(CdrStat *stat);
unsigned char * CALLBACK CDRgetBufferSub(void);
long CALLBACK	CDRreadCDDA(unsigned char m, unsigned char s, unsigned char f, unsigned char *buffer);
long CALLBACK	CDRgetTE(unsigned char track, unsigned char *m, unsigned char *s, unsigned char *f);

// PSEMU configuration support functions:
static void	psemu_ReadCfg(CFG_DATA *cfg);
static void	psemu_WriteCfg(const CFG_DATA *cfg);
static void psemu_CheckForSpecialAppFixes(void);

// Plugin name:
static const char psemu_plugin_name[] = PLUGIN_NAME;
static const char psemu_reg_keyname[] = "Software\\PCSX-RR\\CDR";

// Local variables:
static BOOL use_psemu_tdtn_format = FALSE;
static BYTE	*readbuf = NULL;
static int	last_read_sector_loc = 0;
static BYTE fake_readbuf[RAW_SECTOR_SIZE];	// fake read buffer, used on 'drive not ready' error

//------------------------------------------------------------------------------
// PSEMU plugin interface functions:
//------------------------------------------------------------------------------

unsigned long CALLBACK PSEgetLibType(void)
{
	return PSE_LT_CDR;
}

char * CALLBACK PSEgetLibName(void)
{
	return (char *)psemu_plugin_name;
}

unsigned long CALLBACK	PSEgetLibVersion(void)
{
	return PLUGIN_VERSION<<16 | PLUGIN_REVISION<<8 | PLUGIN_BUILD;
}

int CALLBACK CDRinit(void)
{
	PRINTF(("CDRinit\n"));
	return PSE_CDR_ERR_SUCCESS;
}

int CALLBACK CDRshutdown(void)
{
	PRINTF(("CDRshutdown\n"));
	return PSE_CDR_ERR_SUCCESS;
}

int CALLBACK CDRopen(void)
{
	PRINTF(("CDRopen\n"));
	if (plg_is_open)
		return PSE_CDR_ERR_SUCCESS;
	psemu_ReadCfg(&cfg_data);
	if (!num_found_drives && !scan_AvailDrives())
		return PSE_CDR_ERR_NOREAD;
	if (!open_Driver())
		return PSE_CDR_ERR_NOREAD;
	plg_is_open = TRUE;
	return PSE_CDR_ERR_SUCCESS;
}

int CALLBACK CDRclose(void)
{
	PRINTF(("CDRclose\n"));
	if (plg_is_open && play_in_progress && drive_IsReady())
		if_drive.StopAudioTrack();
	play_in_progress = FALSE;
	close_Driver();
	plg_is_open = FALSE;
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRconfigure(void)
{
	PRINTF(("CDRconfigure\n"));
	cfg_SetReadWriteCfgFn(psemu_ReadCfg, psemu_WriteCfg);
	config_Dialog(GetActiveWindow());
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRabout(void)
{
	PRINTF(("CDRabout\n"));
	about_Dialog(GetActiveWindow());
	return PSE_CDR_ERR_SUCCESS;
}

int CALLBACK CDRtest(void)
{
	PRINTF(("CDRtest\n"));
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRgetTN(unsigned char *result)
{
//	PRINTF(("CDRgetTN\n"));
	// Note: Some buggy emu (like ePSXe) can't call CDRopen() before trying ...
	// ... to access to the cdrom drive, so i'll need to do it by myself.
	if (!plg_is_open) CDRopen();

	if (!plg_is_open || !drive_IsReady() || !if_drive.ReadToc(FALSE))
	{
		result[0] = result[1] = 1;
		return PSE_CDR_ERR_FAILURE;
	}
	if (use_psemu_tdtn_format) {		// 'PSEMU standard' version:
		result[0] = INT2BCD(toc.FirstTrack);
		result[1] = INT2BCD(toc.LastTrack);
	} else {							// NON 'PSEMU standard' version:
		result[0] = toc.FirstTrack;
		result[1] = toc.LastTrack;
	}
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRgetTD(unsigned char track, unsigned char *result)
{
//	PRINTF(("CDRgetTD %d\n", track));
	// Note: Some buggy emu (like ePSXe) can't call CDRopen() before trying ...
	// ... to access to the cdrom drive, so i'll need to do it by myself.
	if (!plg_is_open) CDRopen();

	if (!plg_is_open || !drive_IsReady() || !if_drive.ReadToc(TRUE))
	{
		result[0] = 0; result[1] = 2; result[2] = 0;
		return PSE_CDR_ERR_FAILURE;
	}
	// track==1 -> toc.first, track==n -> toc.last, track==0 -> leadout
	// toc.track[0, ..., n-1, n] = first, ..., last, leadout
	if (track >= toc.FirstTrack && track <= toc.LastTrack)
		track -= toc.FirstTrack;
	else
		track = toc.LastTrack;
	if (use_psemu_tdtn_format) {		// 'PSEMU standard' version:
		result[0] = INT2BCD(toc.TrackData[track].Address[1]); // minute
		result[1] = INT2BCD(toc.TrackData[track].Address[2]); // second
		result[2] = INT2BCD(toc.TrackData[track].Address[3]); // frame
	} else {							// NON 'PSEMU standard' version:
		result[0] = toc.TrackData[track].Address[3]; // frame
		result[1] = toc.TrackData[track].Address[2]; // second
		result[2] = toc.TrackData[track].Address[1]; // minute
	}
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRreadTrack(unsigned char *param)
{
	int		loc = MSFBCD2INT((MSF *)param);

//	PRINTF(("CDRreadTrack %d\n", loc));
	// Note: Some buggy emu (like ePSXe) can't call CDRopen() before trying ...
	// ... to access to the cdrom drive, so i'll need to do it by myself.
	if (!plg_is_open) CDRopen();

	if (!plg_is_open || !drive_IsReady()) {
		readbuf = fake_readbuf;			// ensure CDRgetBuffer() never returns NULL, or ePSXe crashes
		return PSE_CDR_ERR_FAILURE;
	}
	last_read_sector_loc = loc;			// save sector number (for subtrack reading)
	if (!if_drive.StartReadSector(loc, &readbuf) ||
		!if_drive.WaitEndReadSector())
		return PSE_CDR_ERR_NOREAD;
	return PSE_CDR_ERR_SUCCESS;
}

unsigned char * CALLBACK CDRgetBuffer(void)
{
//	PRINTF(("CDRgetBuffer\n"));
	if (!plg_is_open)
		return NULL;
	return readbuf + 12;
}

unsigned char * CALLBACK CDRgetBufferSub(void)
{
	static BYTE	subchannel[96];

//	PRINTF(("CDRgetBufferSub\n"));
	if (!plg_is_open || !drive_IsReady())
		return NULL;
	memset(subchannel, 0, sizeof(subchannel));
	if (!if_drive.ReadSubQ((Q_SUBCHANNEL_DATA *)&subchannel[12], (play_in_progress) ? -1 : last_read_sector_loc))
		return NULL;
	return subchannel;
}

char CALLBACK CDRgetDriveLetter(void)
{
	PRINTF(("CDRgetDriveLetter\n"));
//	if (!drive_IsReady())
//		return 0;
	return cfg_data.drive_id.drive_letter;
}

long CALLBACK CDRplay(unsigned char *param)
{
//	PRINTF(("CDRplay %d:%d:%d\n", (int)param[0], (int)param[1], (int)param[2]));
	// Note: Some buggy emu (like ePSXe) can't call CDRopen() before trying ...
	// ... to access to the cdrom drive, so i'll need to do it by myself.
	if (!plg_is_open) CDRopen();

	if (!plg_is_open || !drive_IsReady() || !if_drive.ReadToc(TRUE))
		return PSE_CDR_ERR_FAILURE;
	if_drive.FinishAsyncReadsPending();	// stops any async reads pending
	if (!if_drive.PlayAudioTrack((MSF *)param, (MSF *)&toc.TrackData[toc.LastTrack].Address[1]))
		return PSE_CDR_ERR_NOREAD;
	play_in_progress = TRUE;
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRstop(void)
{
//	PRINTF(("CDRstop\n"));
	if (!play_in_progress)
		return PSE_CDR_ERR_FAILURE;
	play_in_progress = FALSE;
	if (!plg_is_open || !drive_IsReady() || !if_drive.StopAudioTrack())
		return PSE_CDR_ERR_NOREAD;
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRgetStatus(CdrStat *stat)
{
	BYTE	audio_status;

//	PRINTF(("CDRgetStatus\n"));
	memset(stat, 0, sizeof(*stat));
	if (!plg_is_open || !drive_IsReady() ||
		!if_drive.TestUnitReady(FALSE) || !if_drive.ReadToc(TRUE))
	{
		stat->Type = 0xff;				// no cdrom
		stat->Status = 0x10;			// shell open
		return PSE_CDR_ERR_SUCCESS;
	}
	stat->Type = (toc.TrackData[0].Control & AUDIO_DATA_TRACK) ?
		0x01 : 0x02;					// AUDIO_DATA_TRACK: 0 = AUDIO, 1 = DATA
	if (if_drive.GetPlayLocation(&stat->Time, &audio_status) &&
		audio_status == AUDIO_STATUS_IN_PROGRESS)
		stat->Status = 0x80;			// playing
	// Note: The 'CdrStat.Time' format is NOT BCD encoded !!!
	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRreadCDDA(unsigned char m, unsigned char s, unsigned char f, unsigned char *buffer)
{
	MSF		msf = { m, s, f };
	int		loc = MSF2INT(&msf);
	BYTE	*cdda_readbuf = NULL;

//	PRINTF(("CDRreadCDDA %d:%d:%d\n", m, s, f));
	if (!plg_is_open || !drive_IsReady())
		return PSE_CDR_ERR_FAILURE;
	if (!if_drive.StartReadSector(loc, &cdda_readbuf) ||
		!if_drive.WaitEndReadSector())
		return PSE_CDR_ERR_NOREAD;
	memcpy(buffer, cdda_readbuf, RAW_SECTOR_SIZE);

	return PSE_CDR_ERR_SUCCESS;
}

long CALLBACK CDRgetTE(unsigned char track, unsigned char *m, unsigned char *s, unsigned char *f)
{
	MSF		msf;

//	PRINTF(("CDRgetTE %d\n", track));
	if (!plg_is_open || !drive_IsReady() || !if_drive.ReadToc(TRUE))
	{
		*m = 0; *s = 0; *f = 0;
		return PSE_CDR_ERR_FAILURE;
	}
	// Note: the returned MSF value must be subtracted with MSF_OFFSET !!!
	msf.minute = toc.TrackData[track].Address[1];	// minute
	msf.second = toc.TrackData[track].Address[2];	// second
	msf.frame = toc.TrackData[track].Address[3];	// frame
	INT2MSF(&msf, MSF2INT(&msf) - MSF_OFFSET);
	*m = msf.minute;
	*s = msf.second;
	*f = msf.frame;
	return PSE_CDR_ERR_SUCCESS;
}

//------------------------------------------------------------------------------
// PSEMU configuration support functions:
//------------------------------------------------------------------------------

void psemu_CheckForSpecialAppFixes(void)
{
	char	buff[1024], *appname;
	int		i;
	// Emulators using a 'PSEMU standard' CDRgetTD/CDRgetTN format:
	// - psemu, fpse, adripsx
	// Emulators using a 'NON PSEMU standard' CDRgetTD/CDRgetTN format:
	// - epsxe, pcsx
	static const char	*psemu_tdtn_format_apps[] = {
		"psemu",	// PsEmu (of course) uses the old 'PSEMU standard' format ...
		"fpse",		// ... for CDRgetTD/TN (BCD values and MSF order) !!!
		"adripsx",	// FPSE and ADRIPSX seems also to follow the original format.
	};
//	static const char	*non_psemu_tdtn_format_apps[] = {
//		"epsxe",	// ePSXe and PCSX are both using the other 'NON PSEMU standard' format.
//		"pcsx",		// (and they have changed a working standard instead of fixing their emu: too bad !!!)
//	};

	// CDRgetTD/TN default format: it's a nonsense, but it seems that the standard ...
	// ... for the new 'psemu plugins' seems now to be 'NON PSEMU standard' !!!
	use_psemu_tdtn_format = FALSE;		// default = 'NON PSEMU standard' format

	if (GetModuleFileName(NULL, buff, sizeof(buff)) != 0 &&
		(appname = strrchr(buff, '\\')) != NULL) {
		for (i = 0, appname++; i < (sizeof(psemu_tdtn_format_apps) / sizeof(psemu_tdtn_format_apps[0])); i++) {
			if (!strnicmp(appname, psemu_tdtn_format_apps[i], strlen(psemu_tdtn_format_apps[i]))) {
				use_psemu_tdtn_format = TRUE;
				break;
			}
		}
	}
	PRINTF(("CDRgetTD/TN format = %sPSEMU\n", (use_psemu_tdtn_format) ? "" : "NON-"));
}

void psemu_ReadCfg(CFG_DATA *cfg)
{
	HKEY	hKey;
	DWORD	dwType, dwValue, dwSize;

	cfg_InitDefaultValue(cfg);	// init default cfg values

	// check if the caller application need any special fix (see CDRgetTN/TD)
	psemu_CheckForSpecialAppFixes();

	if (RegOpenKeyEx(HKEY_CURRENT_USER, psemu_reg_keyname, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
		return;
#define	GET_DWORD_VALUE(name, value, type) \
	dwSize = sizeof(dwValue); \
	if (RegQueryValueEx(hKey, name, 0, &dwType, (unsigned char *)&dwValue, &dwSize) == ERROR_SUCCESS) \
		value = (type)dwValue;
	GET_DWORD_VALUE("AdapterID", cfg->drive_id.haid, BYTE)
	GET_DWORD_VALUE("TargetID", cfg->drive_id.target, BYTE)
	GET_DWORD_VALUE("LunID", cfg->drive_id.lun, BYTE)
	GET_DWORD_VALUE("DriveLetter", cfg->drive_id.drive_letter, BYTE)
	GET_DWORD_VALUE("IfType", cfg->interface_type, eInterfaceType)
	GET_DWORD_VALUE("ReadMode", cfg->read_mode, eReadMode)
	GET_DWORD_VALUE("CdSpeed", cfg->cdrom_speed, int)
	GET_DWORD_VALUE("CdSpindown", cfg->cdrom_spindown, int)
	GET_DWORD_VALUE("CacheLevel", cfg->cache_level, eCacheLevel)
//	GET_DWORD_VALUE("CacheAsyncTrig", cfg->cache_asynctrig, int)
	GET_DWORD_VALUE("TrackFsys", cfg->track_fsys, BOOL)
	GET_DWORD_VALUE("SubQMode", cfg->subq_mode, eSubQMode)
//	GET_DWORD_VALUE("UseSubFile", cfg->use_subfile, BOOL)
#undef	GET_DWORD_VALUE
	RegCloseKey(hKey);
}

void psemu_WriteCfg(const CFG_DATA *cfg)
{
	HKEY	hKey;
	DWORD	dwValue;
	DWORD	dwDisposition = 0;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, psemu_reg_keyname, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
		return;
#define	SET_DWORD_VALUE(name, value, type) \
	dwValue = (DWORD)value; \
	RegSetValueEx(hKey, name, 0, REG_DWORD, (unsigned char *)&dwValue, sizeof(dwValue));
	SET_DWORD_VALUE("AdapterID", cfg->drive_id.haid, BYTE)
	SET_DWORD_VALUE("TargetID", cfg->drive_id.target, BYTE)
	SET_DWORD_VALUE("LunID", cfg->drive_id.lun, BYTE)
	SET_DWORD_VALUE("DriveLetter", cfg->drive_id.drive_letter, BYTE)
	SET_DWORD_VALUE("IfType", cfg->interface_type, eInterfaceType)
	SET_DWORD_VALUE("ReadMode", cfg->read_mode, eReadMode)
	SET_DWORD_VALUE("CdSpeed", cfg->cdrom_speed, int)
	SET_DWORD_VALUE("CdSpindown", cfg->cdrom_spindown, int)
	SET_DWORD_VALUE("CacheLevel", cfg->cache_level, eCacheLevel)
//	SET_DWORD_VALUE("CacheAsyncTrig", cfg->cache_asynctrig, int)
	SET_DWORD_VALUE("TrackFsys", cfg->track_fsys, BOOL)
	SET_DWORD_VALUE("SubQMode", cfg->subq_mode, eSubQMode)
//	SET_DWORD_VALUE("UseSubFile", cfg->use_subfile, BOOL)
#undef	SET_DWORD_VALUE
	RegCloseKey(hKey);
}

