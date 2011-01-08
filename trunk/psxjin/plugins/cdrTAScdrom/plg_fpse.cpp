// Plugin Defs
#include "Locals.h"
// FPSE SDK
#include "type.h"
#include "sdk.h"
#include "Win32Def.h"
// Dialog includes:
#include "resource.h"

// FPSE plugin interface structs (not included in 'sdk.h'):
typedef struct {
    DWORD	Type;			// cdrom type
// Type:	0x00 = unknown, 0x01 = data, 0x02 = audio, 0xff = no cdrom
    DWORD	Status;			// drive status
// Status:	0x00 = unknown, 0x01 = error, 0x04 = seek error,
//			0x10 = shell open, 0x20 = reading, 0x40 = seeking, 0x80 = playing
    MSF		Time;			// current playing time (M,S,F)
} CdrStat;

// Plugin name:
static const char fpse_ini_keyname[] = FPSE_PLUGIN_CFGNAME;

// FPSE plugin interface functions:
int		CD_Open(UINT32 *par);
void	CD_Close(void);
int		CD_GetTN(UINT8 *result);
int		CD_GetTD(UINT8 *result, int track);
UINT8	*CD_Read(UINT8 *param);
int		CD_Wait(void);
int		CD_Play(UINT8 *param);
int		CD_Stop(void);
int		CD_Configure(UINT32 *par);
void	CD_About(UINT32 *par);

// FPSE configuration support functions:
static void	fpse_ReadCfg(CFG_DATA *cfg);
static void	fpse_WriteCfg(const CFG_DATA *cfg);

// Local variables:
static FPSEWin32	*infocfg = NULL;
static int	last_read_sector_loc = 0;
static int	last_wait_sector_loc = 0;



//------------------------------------------------------------------------------
// FPSE plugin interface functions:
//------------------------------------------------------------------------------

int CD_Open(UINT32 *par)
{
	infocfg = (FPSEWin32 *)par;

	if (plg_is_open)
		return FPSE_OK;
	fpse_ReadCfg(&cfg_data);
	if (!num_found_drives && !scan_AvailDrives())
		return FPSE_ERR;
	if (!open_Driver())
		return FPSE_ERR;
	plg_is_open = TRUE;
	return FPSE_OK;
}

void CD_Close(void)
{
	if (plg_is_open && play_in_progress && drive_IsReady())
		if_drive.StopAudioTrack();
	play_in_progress = FALSE;
	close_Driver();
	plg_is_open = FALSE;
}

int CD_GetTN(UINT8 *result)
{
	if (!plg_is_open || !drive_IsReady() || !cache_ReadToc(FALSE))
	{
		result[1] = result[2] = 1;
		return FPSE_ERR;
	}
	result[1] = toc.FirstTrack;
	result[2] = toc.LastTrack;
	return FPSE_OK;
}

int CD_GetTD(UINT8 *result, int track)
{
	if (!plg_is_open || !drive_IsReady() || !cache_ReadToc(TRUE))
	{
		result[1] = 0; result[2] = 2; result[3] = 0;
		return FPSE_ERR;
	}
	// track==1 -> toc.first, track==n -> toc.last, track==0 -> leadout
	// toc.track[0, ..., n-1, n] = first, ..., last, leadout
	if (track >= toc.FirstTrack && track <= toc.LastTrack)
		track -= toc.FirstTrack;
	else
		track = toc.LastTrack;
	result[1] = toc.TrackData[track].Address[1]; // minute
	result[2] = toc.TrackData[track].Address[2]; // second
	result[3] = toc.TrackData[track].Address[3]; // frame
	return FPSE_OK;
}

static BOOL tmp_res;
static BOOL tmp_wait = FALSE;

UINT8 *CD_Read(UINT8 *param)
{
	int		loc = MSF2INT((MSF *)param);
	BYTE	*readbuf;

	if (!plg_is_open || !drive_IsReady())
		return NULL;
	last_read_sector_loc = loc;			// save request sector number (for subtrack reading)
	tmp_res = if_drive.StartReadSector(loc, &readbuf);
	tmp_wait = TRUE;
	return readbuf + 12;
}

int CD_Wait(void)
{
	if (!plg_is_open || !tmp_wait)
		return FPSE_OK;
	last_wait_sector_loc = last_read_sector_loc;	// save readed sector number (for subtrack reading)
	tmp_wait = FALSE;
	if (!tmp_res || !if_drive.WaitEndReadSector())
		return FPSE_ERR;
	return FPSE_OK;
}

UINT8 *CD_GetSeek(void)
{
	static Q_SUBCHANNEL_DATA	subchannel;

	if (!plg_is_open || !drive_IsReady())
		return NULL;
	memset(&subchannel, 0, sizeof(subchannel));
	if (!cache_ReadSubQ(&subchannel, (play_in_progress) ? -1 : last_wait_sector_loc))
		return NULL;
	return (UINT8 *)&subchannel;
}

void CD_Eject(void)
{
	if (!plg_is_open || !drive_IsReady())
		return;
	if (if_drive.LoadEjectUnit(FALSE))
		cache_FlushBuffers();			// media removed: empty cache buffers !!!
}

int CD_Play(UINT8 *param)
{
	if (!plg_is_open || !drive_IsReady() || !cache_ReadToc(TRUE))
		return FPSE_ERR;
	if_drive.FinishAsyncReadsPending();	// stops any async reads pending
	PRINTF(("START PLAY %d:%d:%d\n", (int)param[0], (int)param[1], (int)param[2]));
	if (!if_drive.PlayAudioTrack((MSF *)param, (MSF *)&toc.TrackData[toc.LastTrack].Address[1]))
		return FPSE_ERR;
	play_in_progress = TRUE;
	return FPSE_OK;
}

int CD_Stop(void)
{
	PRINTF(("STOP PLAY\n"));
	play_in_progress = FALSE;
	if (!plg_is_open || !drive_IsReady() || !if_drive.StopAudioTrack())
		return FPSE_ERR;
	return FPSE_OK;
}

UINT8 *CD_GetStatus(void)
{
	static CdrStat	stat;
	BYTE	audio_status;

//	PRINTF(("CDRgetStatus\n"));
	memset(&stat, 0, sizeof(stat));
	if (!plg_is_open || !drive_IsReady() ||
		!if_drive.TestUnitReady(FALSE) || !cache_ReadToc(TRUE))
	{
		stat.Type = 0xff;				// no cdrom
		stat.Status = 0x10;				// shell open
		return (UINT8 *)&stat;
	}
	stat.Type = (toc.TrackData[0].Control & AUDIO_DATA_TRACK) ?
		0x01 : 0x02;					// AUDIO_DATA_TRACK: 0 = AUDIO, 1 = DATA
	if (if_drive.GetPlayLocation(&stat.Time, &audio_status) &&
		audio_status == AUDIO_STATUS_IN_PROGRESS)
		stat.Status = 0x80;				// playing
	// Note: The 'CdrStat.Time' format is NOT BCD encoded !!!
	return (UINT8 *)&stat;
}


int CD_Configure(UINT32 *par)
{
	infocfg = (FPSEWin32 *)par;
	cfg_SetReadWriteCfgFn(fpse_ReadCfg, fpse_WriteCfg);
	config_Dialog(infocfg->HWnd);
	return FPSE_OK;
}

void CD_About(UINT32 *par)
{
	FPSEWin32About	*about_info = (FPSEWin32About *)par;
	int		idx;
	char	*p;
	const char	*q;

	memset(about_info, 0, sizeof(*about_info));
	about_info->PlType = FPSE_CDROM;	// Plugin type: CDROM
	about_info->VerLo = PLUGIN_BUILD;	// Version High
	about_info->VerHi = PLUGIN_REVISION;// Version Low
	about_info->TestResult = FPSE_OK;	// Returns if it'll work or not
	strcpy(about_info->Author, FPSE_PLUGIN_AUTHOR);	// Name of the author
	strcpy(about_info->Name, FPSE_PLUGIN_NAME);	// Name of plugin
	for (idx = 0, p = about_info->Description; (q = about_line_text(idx)) != NULL; idx++)
		p += sprintf(p, "%s\r\n", q);	// Description to put in the edit box
}

//------------------------------------------------------------------------------
// FPSE configuration support functions:
//------------------------------------------------------------------------------

void fpse_ReadCfg(CFG_DATA *cfg)
{
	char	buf[1024];

	cfg_InitDefaultValue(cfg);	// init default cfg values
#define	GET_DWORD_VALUE(name, value, type) \
	if (((int (*)(const char *, const char *, char *))(infocfg->ReadCfg))(fpse_ini_keyname, name, buf) == FPSE_OK && *buf) \
		value = (type)atoi(buf);
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
}

void fpse_WriteCfg(const CFG_DATA *cfg)
{
	char	buf[1024];
#define	SET_DWORD_VALUE(name, value, type) \
	((int (*)(const char *, const char *, char *))(infocfg->WriteCfg))(fpse_ini_keyname, name, itoa((int)value, buf, 10));
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
}

