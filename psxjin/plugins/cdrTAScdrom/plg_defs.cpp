// Plugin Defs
#include "Locals.h"
// Dialog includes:
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"

typedef struct {
	DWORD	cbSize;
	DWORD	dwMajorVersion;				// Major version
	DWORD	dwMinorVersion;				// Minor version
	DWORD	dwBuildNumber;				// Build number
	DWORD	dwPlatformID;				// DLLVER_PLATFORM_*
} DLLVERSIONINFO_COMCTL32;

#define	MAX_TOOLTIP_TEXT_LEN	1024	// max tooltip multi-line text len

static void	dummy_ReadCfg(CFG_DATA *cfg) { cfg_InitDefaultValue(cfg); }
static void	dummy_WriteCfg(const CFG_DATA *cfg) { }

// Plugin specific functions:
void	(*fn_ReadCfg)(CFG_DATA *cfg) = dummy_ReadCfg;
void	(*fn_WriteCfg)(const CFG_DATA *cfg) = dummy_WriteCfg;

// Misc data:
static HWND	hwndTT = NULL;				// tooltip control window
static HWND	hwndSliderTT = NULL;		// tooltip control for slider
static char	*tt_text_long = NULL;		// tooltip buffer for multi-line text
static int	comctl32_version = 400;		// comctl32.dll version (see comctl32_GetVersion())

// Plugin Config / About dialog callbacks:
BOOL CALLBACK	ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wPar, LPARAM lPar);
BOOL CALLBACK	AboutDialogProc(HWND hwnd, UINT msg, WPARAM wPar, LPARAM lPar);

// Default configuration data:
static const CFG_DATA default_cfg_data = {
	{ 0xFF, 0xFF, 0xFF, '\0', },		// drive_id.haid/target/lun/drive_letter = Auto
	InterfaceType_Auto,					// interface_type = Auto
	ReadMode_Auto,						// read_mode = Auto
	0,									// cdrom_speed =  Default
	0,									// cdrom_spindown =  Default
	CacheLevel_ReadAsync,				// cache_level = Enabled w/ multi-sector read & prefetch
	75,									// cache_asynctrig = Async read triggered at 75% of cache buffer
	TRUE,								// track_fsys = Enable ISO9660 tracking
	SubQMode_Auto,						// subq_mode = Auto
	FALSE,								// use_subfile = Use Subchannel file if present
};

// Tooltips for 'Config' dialog:
static struct {
	int		ctrl_id;
	char	*text_short;	// single-line text (for comctl32 versions < 4.70, w/o multi-line support)
	char	*text_long;		// multi-line text (for comctl32 versions >= 4.70): if NULL, use single-line test
} configdialog_tooltips[] = {
	{ IDC_COMBO_DRIVE, 	"Select the CD-ROM drive you want to use",
		"   Autodetect = Choose the first CD-ROM with a PSX disk inside (recommended)\n" },
	{ IDC_COMBO_INTERFACE,	"Select the method to be used for access the CD-ROM drive",
		"Supported modes for Win95/98/Me:\n"
		"   Autodetect = Choose the best mode automatically (recommended)\n"
		"   ASPI = Uses the SCSI ASPI driver, if it's installed\n"
		"   MSCDEX = Uses the native MSCDEX driver (not recommended)\n"
		"Supported modes for WinNT/2K/XP:\n"
		"   Autodetect = Choose the best mode automatically (recommended)\n"
		"   IOCTL/SCSI = Uses the native SCSI IOCTL interface\n"
		"   ASPI = Uses the SCSI ASPI driver, if it's installed\n"
		"   IOCTL/RAW = Uses the native RAW IOCTL interface (not recommended)" },
	{ IDC_COMBO_READMODE,	"Select the command to be used for RAW sectors reading from the CD-ROM drive",
		"Supported modes for ASPI and IOCTL/SCSI:\n"
		"   Autodetect = Choose the best mode automatically (recommended)\n"
		"   SCSI_BE = Supported by the most common and newer CD-ROM/DVD drives (best mode)\n"
		"   SCSI_28 = Supported by some older CD-ROM drives or with SCSI interface\n"
		"   SCSI/D8 = Non-standard mode, only for some PLEXTOR/TEAC models (not recommended)\n"
		"   SCSI/D4 = Non-standard mode, only for some NEC/SONY models (not recommended)\n"
		"For MSCDEX and IOCTL/RAW, the only supported mode is RAW" },
	{ IDC_COMBO_CDSPEED,	"Limit the CD-ROM drive speed (if supported from drive)",
		"Available options:\n"
		"   Default = don't change drive speed (recommended)\n"
		"   2X...16X = set 2X...16X speed\n"
		"   MAX = use MAX drive speed", },
	{ IDC_COMBO_CDSPINDOWN,	"Change the CD-ROM spindown time (if supported from drive)",
		"Available options:\n"
		"   Default = don't change spindown time (recommended)\n"
		"   30sec...16min = Stop CD after 30sec...16min of inactivity", },
	{ IDC_COMBO_CACHELEVEL,	"Select if the readed data can be cached / prefetched",
		"Available options:\n"
		"   0 - Disabled (no caching) = Always access CD-ROM drive for reading (slowest)\n"
		"   1 - Enabled, read one sector at time = Read/cache one sector at time (slow)\n"
		"   2 - Enabled, prefetch multiple sectors = Read/cache multiple sectors a time (fast)\n"
		"   3 - Enabled, prefetch with async reads = Add 'intelligent' asynchronous reads (fastest, recommended)", },
	{ IDC_SLIDER_ASYNCTRIG,	"Select the delay before starting async reads (% of last cache block readed)",
		"It's recommended to leave the default value (75%), or at least to keep the slider inside the 'green' zone\n"
		"Note: This option needs to be changed ONLY if there are noticeable delays during Movies && Sounds playback", },
	{ IDC_CHECK_TRACKISOFS,	"Track the CD-ROM files location/size (better read prediction, recommended)",
		"When enabled, remember the exact files properties, improving read performance", },
	{ IDC_COMBO_SUBREADMODE,	"Select the command to be used for Subchannel reading from the CD-ROM drive",
		"Supported modes for ASPI and IOCTL/SCSI:\n"
		"   Autodetect = Choose the best mode automatically (recommended)\n"
		"   SCSI_nn+16 = Same as 'Sector Read' modes, reads 16 bytes for Q Subchannel data\n"
		"   SCSI_nn+96 = Same as 'Sector Read' modes, reads 96 bytes for P-W Subchannel data\n"
		"   SCSI_42/RAW = Non-standard mode, supported but completely unreliable (not recommended)\n"
		"   Disabled = Don't read any subchannel data from the CD-ROM drive\n"
		"Note: Subchannel reading is NOT supported from some CD-ROM drives !!!", },
	{ IDC_CHECK_USESUBFILE, "Use Subchannel SBI / M3S file if present",
		"When enabled, read subchannel data from SBI / M3S file instead from CD-ROM drive\n"
		"The SBI / M3S files require a specific name convention described as follow:\n"
		"   NNNN_NNN.NN.SBI / NNNN_NNN.NN.M3S (as example: SLES_123.45.SBI)\n"
		"where NNNN_NN.NN is the exact name of the file SLES*, SCES* or SLUS* on the CD\n"
		"Note: This option is needed ONLY if CD-ROM does not support Subchannel reading", },
	{ IDOK,					"Save the current configuration",
		NULL, },
	{ IDCANCEL,				"Exit without saving the configuration",
		NULL, },
};


//------------------------------------------------------------------------------
// Support functions:
//------------------------------------------------------------------------------

const char *if_type_name(eInterfaceType if_type)
{
	switch (if_type) {
	case InterfaceType_Auto:	return "Autodetect...";
	case InterfaceType_Aspi:	return "ASPI";
	case InterfaceType_NtSpti:	return "IOCTL/SCSI";
	case InterfaceType_NtRaw:	return "IOCTL/RAW";
	case InterfaceType_Mscdex:	return "MSCDEX";
	default:					return "???";
	}
}

const char *rd_mode_name(eReadMode rmode)
{
	switch (rmode) {
	case ReadMode_Auto:			return "Autodetect...";
	case ReadMode_Raw:			return "RAW";
	case ReadMode_Scsi_28:		return "SCSI_28 (READ_10)";
	case ReadMode_Scsi_BE:		return "SCSI_BE (READ_CD)";
	case ReadMode_Scsi_D4:		return "SCSI_D4 (CUSTOM)";
	case ReadMode_Scsi_D8:		return "SCSI_D8 (CUSTOM)";
	default:					return "???";
	}
}

const char *cd_speed_name(int speed)
{
	switch (speed) {
	case 0:						return "Default";
	case 2:						return "2 X";
	case 4:						return "4 X";
	case 6:						return "6 X";
	case 8:						return "8 X";
	case 12:					return "12 X";
	case 16:					return "16 X";
	case 0xFFFF:				return "MAX";
	default:					return "???";
	}
}

const char *cd_spindown_name(int timer)
{
	switch (timer) {
	case INACTIVITY_TIMER_VENDOR_SPECIFIC:	return "Default";	// 0 = Default
	case INACTIVITY_TIMER_125MS:return "125 msec";
	case INACTIVITY_TIMER_250MS:return "250 msec";
	case INACTIVITY_TIMER_500MS:return "500 msec";
	case INACTIVITY_TIMER_1S:	return "1 sec";
	case INACTIVITY_TIMER_2S:	return "2 sec";
	case INACTIVITY_TIMER_4S:	return "4 sec";
	case INACTIVITY_TIMER_8S:	return "8 sec";
	case INACTIVITY_TIMER_16S:	return "16 sec";
	case INACTIVITY_TIMER_32S:	return "32 sec";
	case INACTIVITY_TIMER_1MIN:	return "1 min";
	case INACTIVITY_TIMER_2MIN:	return "2 min";
	case INACTIVITY_TIMER_4MIN:	return "4 min";
	case INACTIVITY_TIMER_8MIN:	return "8 min";
	case INACTIVITY_TIMER_16MIN:return "16 min";
	case INACTIVITY_TIMER_32MIN:return "32 min";
	default:					return "???";
	}
}

const char *subq_mode_name(eSubQMode subQmode)
{
	switch (subQmode) {
	case SubQMode_Auto:			return "Autodetect...";
	case MAKE_SUBQMODE(ReadMode_Scsi_28, SubQMode_Read_Q):	return "SCSI_28+16";
	case MAKE_SUBQMODE(ReadMode_Scsi_28, SubQMode_Read_PW):	return "SCSI_28+96";
	case MAKE_SUBQMODE(ReadMode_Scsi_BE, SubQMode_Read_Q):	return "SCSI_BE+16";
	case MAKE_SUBQMODE(ReadMode_Scsi_BE, SubQMode_Read_PW):	return "SCSI_BE+96";
	case MAKE_SUBQMODE(ReadMode_Scsi_D8, SubQMode_Read_Q):	return "SCSI_D8+16";
	case MAKE_SUBQMODE(ReadMode_Scsi_D8, SubQMode_Read_PW):	return "SCSI_D8+96";
	case MAKE_SUBQMODE(ReadMode_Raw,     SubQMode_SubQ):	return "SCSI_42/RAW";
	case SubQMode_Disabled:		return "Disabled";
	default:					return "???";
	}
}

const char *cache_level_name(eCacheLevel cache_level)
{
	switch (cache_level) {
	case CacheLevel_Disabled:	return "0 - Disabled (no caching)";
	case CacheLevel_ReadOne:	return "1 - Enabled, read one sector at time";
	case CacheLevel_ReadMulti:	return "2 - Enabled, prefetch multiple sectors";
	case CacheLevel_ReadAsync:	return "3 - Enabled, prefetch with async reads (recommended)";
	default:					return "???";
	}
}

const char *about_line_text(int nline)
{
	switch (nline) {
	case 0:						return "A 'smart' CD-ROM plugin for use with any";
	case 1:						return "FPSE or PSEmu compatible PSX emulator";
	case 2:						return "FREE for any non-commercial use";
	case 3:						return "(C) 2002-2009 by SaPu";
	default:					return NULL;	// end of 'About' text !!!
	}
}

//------------------------------------------------------------------------------
// Generic plugin functions:
//------------------------------------------------------------------------------

void cfg_InitDefaultValue(CFG_DATA *cfg)
{
	memcpy(cfg, &default_cfg_data, sizeof(*cfg));
}

void cfg_SetReadWriteCfgFn(void (*readCfgFn)(CFG_DATA *), void (*writeCfgFn)(const CFG_DATA *))
{
	fn_ReadCfg = readCfgFn;
	fn_WriteCfg = writeCfgFn;
}

static void comctl32_GetVersion(void)
{
	HINSTANCE   hDll;
	HRESULT (CALLBACK* pDllGetVersion)(DLLVERSIONINFO_COMCTL32 *);
	DLLVERSIONINFO_COMCTL32 dvi;            

	if ((hDll = LoadLibrary("comctl32.dll")) != NULL)
	{
		memset(&dvi, 0, sizeof(dvi));
		dvi.cbSize = sizeof(dvi);
		if ((pDllGetVersion = (HRESULT (CALLBACK *)(DLLVERSIONINFO_COMCTL32 *))GetProcAddress(hDll, "DllGetVersion")) != NULL &&
			(*pDllGetVersion)(&dvi) >= 0)
			comctl32_version = dvi.dwMajorVersion * 100 + dvi.dwMinorVersion;
		FreeLibrary(hDll);
	}
}
//------------------------------------------------------------------------------
// Plugin configuration dialog:
//------------------------------------------------------------------------------

void config_Dialog(HWND hwnd)
{
	InitCommonControls();	// ensure that the common control DLL is loaded
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_CONFIG),
		hwnd, (DLGPROC)ConfigDialogProc);
}

static void ConfigDialog_SetDriveId(HWND hwnd, const SCSIADDR *cur_drive_id)
{
	HWND	hdlg;
    int		idx, cur;
    char	buf[256];

	hdlg = GetDlgItem(hwnd, IDC_COMBO_DRIVE);
	ComboBox_ResetContent(hdlg);
	ComboBox_AddString(hdlg, "Autodetect...");
	for (idx = 0, cur = 0; idx < num_found_drives; idx++) {
		SCSIADDR	addr = found_drives[idx];
		char		*p = buf;

		if (addr.drive_letter) 
			p += sprintf(p, "%c:", addr.drive_letter);
		if (addr.haid != 0xFF && addr.target != 0xFF && addr.lun != 0xFF)
			p += sprintf(p, " [%d:%d:%d]", addr.haid, addr.target, addr.lun);
		if (*drive_info[idx].VendorId) {
			p += sprintf(p, " %.8s", drive_info[idx].VendorId);
			while (*(p - 1) == ' ') p--;
		}
		if (*drive_info[idx].ProductId) {
			p += sprintf(p, " %.16s", drive_info[idx].ProductId);
			while (*(p - 1) == ' ') p--;
		}
		/*if (*drive_info[idx].VendorSpecific) {
			p += sprintf(p, " V%.20s", drive_info[idx].VendorSpecific);
			while (*(p - 1) == ' ') p--;
		}*/
		*p = '\0';
		ComboBox_AddString(hdlg, buf);
		if (SCSIADDR_EQ(*cur_drive_id, found_drives[idx]))
			cur = idx + 1;				// idx(0) == Autodetect...
	}
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetDriveId(HWND hwnd, SCSIADDR *addr)
{
    int		idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_DRIVE));
	*addr = (idx > 0 && idx <= num_found_drives) ?	// idx(0) == Autodetect...
		found_drives[idx - 1] : default_cfg_data.drive_id;
}

static void ConfigDialog_SetIfType(HWND hwnd, eInterfaceType cur_if_type)
{
	HWND	hdlg;
    int		idx, cur, num_if_types;
	const eInterfaceType	*if_types;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_INTERFACE);
	ComboBox_ResetContent(hdlg);
	ComboBox_AddString(hdlg, "Autodetect...");
	num_if_types = get_if_types_auto(&if_types, FALSE);
	for (idx = 0, cur = 0; idx < num_if_types; idx++) {
		ComboBox_AddString(hdlg, if_type_name(if_types[idx]));
		if (cur_if_type == if_types[idx])
			cur = idx + 1;				// idx(0) == Autodetect...
	}
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetIfType(HWND hwnd, eInterfaceType *if_type)
{
	const eInterfaceType	*if_types;
	int		num_if_types = get_if_types_auto(&if_types, FALSE);
	int		idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_INTERFACE));
	*if_type = (idx > 0 && idx <= num_if_types) ?	// idx(0) == Autodetect...
		if_types[idx - 1] : default_cfg_data.interface_type;
}

static eInterfaceType	last_if_type_for_RdMode_combo = InterfaceType_Auto;

static void ConfigDialog_SetRdMode(HWND hwnd, eReadMode cur_rd_mode)
{
	HWND	hdlg;
    int		idx, cur, num_rd_modes;
	const eReadMode	*rd_modes;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_READMODE);
	ComboBox_ResetContent(hdlg);
	// read 'if_type' & save the value for the *_GetReadMode() call
	ConfigDialog_GetIfType(hwnd, &last_if_type_for_RdMode_combo);
	num_rd_modes = get_rd_modes_auto(&rd_modes, last_if_type_for_RdMode_combo);
	for (idx = 0, cur = 0; idx < num_rd_modes; idx++) {
		ComboBox_AddString(hdlg, rd_mode_name(rd_modes[idx]));
		if (cur_rd_mode == rd_modes[idx])
			cur = idx;
	}
	ComboBox_Enable(hdlg, (num_rd_modes <= 1) ? FALSE : TRUE);
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetRdMode(HWND hwnd, eReadMode *rd_mode)
{
	int		idx, num_rd_modes;
	const eReadMode	*rd_modes;
	// Note: The 'rd_mode' value is 'if_type' dependent, so we can need ...
	// ... to use the value saved from the previous *_SetRdMode() call !!!
	num_rd_modes = get_rd_modes_auto(&rd_modes, last_if_type_for_RdMode_combo);
	// Note: if no choices availables (RAW only), don't need to write the ...
	// ... rd_mode value (don't override the last working SCSI read mode).
	if (num_rd_modes <= 1) return;
	idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_READMODE));
	*rd_mode = (idx >= 0 && idx < num_rd_modes) ?
		rd_modes[idx] : default_cfg_data.read_mode;
}

static void ConfigDialog_UpdateRdMode(HWND hwnd, eReadMode *rd_mode)
{
	ConfigDialog_GetRdMode(hwnd, rd_mode);
	ConfigDialog_SetRdMode(hwnd, *rd_mode);
}

static eInterfaceType	last_if_type_for_CdSpeed_combo = InterfaceType_Auto;

static void ConfigDialog_SetCdSpeed(HWND hwnd, int cur_cd_speed)
{
	HWND	hdlg;
    int		idx, cur, num_cd_speeds;
	const int	*cd_speeds;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_CDSPEED);
	ComboBox_ResetContent(hdlg);
	// read 'if_type' & save the value for the *_GetCdSpeed() call
	ConfigDialog_GetIfType(hwnd, &last_if_type_for_CdSpeed_combo);
	num_cd_speeds = get_cd_speeds_auto(&cd_speeds, last_if_type_for_CdSpeed_combo);
	for (idx = 0, cur = 0; idx < num_cd_speeds; idx++) {
		ComboBox_AddString(hdlg, cd_speed_name(cd_speeds[idx]));
		if (cur_cd_speed == cd_speeds[idx])
			cur = idx;
	}
	ComboBox_Enable(hdlg, (num_cd_speeds <= 1) ? FALSE : TRUE);
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetCdSpeed(HWND hwnd, int *cd_speed)
{
	int		idx, num_cd_speeds;
	const int	*cd_speeds;
	// Note: The 'cd_speed' value is 'if_type' dependent, so we can need ...
	// ... to use the value saved from the previous *_SetCdSpeed() call !!!
	num_cd_speeds = get_cd_speeds_auto(&cd_speeds, last_if_type_for_CdSpeed_combo);
	// Note: if no choices availables (RAW only), don't need to write the ...
	// ... cd_speed value (don't override the last working SCSI cdrom speed).
	if (num_cd_speeds <= 1) return;
	idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_CDSPEED));
	*cd_speed = (idx >= 0 && idx < num_cd_speeds) ?
		cd_speeds[idx] : default_cfg_data.cdrom_speed;
}

static void ConfigDialog_UpdateCdSpeed(HWND hwnd, int *cd_speed)
{
	ConfigDialog_GetCdSpeed(hwnd, cd_speed);
	ConfigDialog_SetCdSpeed(hwnd, *cd_speed);
}

static eInterfaceType	last_if_type_for_CdSpindown_combo = InterfaceType_Auto;

static void ConfigDialog_SetCdSpindown(HWND hwnd, int cur_cd_spindown)
{
	HWND	hdlg;
    int		idx, cur, num_cd_spindowns;
	const int	*cd_spindowns;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_CDSPINDOWN);
	ComboBox_ResetContent(hdlg);
	// read 'if_type' & save the value for the *_GetCdSpindown() call
	ConfigDialog_GetIfType(hwnd, &last_if_type_for_CdSpindown_combo);
	num_cd_spindowns = get_cd_spindowns_auto(&cd_spindowns, last_if_type_for_CdSpindown_combo);
	for (idx = 0, cur = 0; idx < num_cd_spindowns; idx++) {
		ComboBox_AddString(hdlg, cd_spindown_name(cd_spindowns[idx]));
		if (cur_cd_spindown == cd_spindowns[idx])
			cur = idx;
	}
	ComboBox_Enable(hdlg, (num_cd_spindowns <= 1) ? FALSE : TRUE);
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetCdSpindown(HWND hwnd, int *cd_spindown)
{
	int		idx, num_cd_spindowns;
	const int	*cd_spindowns;
	// Note: The 'cd_spindown' value is 'if_type' dependent, so we can need ...
	// ... to use the value saved from the previous *_SetCdSpindown() call !!!
	num_cd_spindowns = get_cd_spindowns_auto(&cd_spindowns, last_if_type_for_CdSpindown_combo);
	// Note: if no choices availables (RAW only), don't need to write the ...
	// ... cd_spindown value (don't override the last working SCSI cdrom spindown).
	if (num_cd_spindowns <= 1) return;
	idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_CDSPINDOWN));
	*cd_spindown = (idx >= 0 && idx < num_cd_spindowns) ?
		cd_spindowns[idx] : default_cfg_data.cdrom_spindown;
}

static void ConfigDialog_UpdateCdSpindown(HWND hwnd, int *cd_spindown)
{
	ConfigDialog_GetCdSpindown(hwnd, cd_spindown);
	ConfigDialog_SetCdSpindown(hwnd, *cd_spindown);
}

static void ConfigDialog_SetCacheLevel(HWND hwnd, eCacheLevel cur_cache_level)
{
	HWND	hdlg;
    int		idx, cur;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_CACHELEVEL);
	ComboBox_ResetContent(hdlg);
	for (idx = (int)CacheLevel_Disabled, cur = (int)default_cfg_data.cache_level; idx <= (int)CacheLevel_ReadAsync; idx++) {
		ComboBox_AddString(hdlg, cache_level_name((eCacheLevel)idx));
		if ((int)cur_cache_level == idx)
			cur = idx;
	}
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetCacheLevel(HWND hwnd, eCacheLevel *cache_level)
{
	int		idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_CACHELEVEL));
	*cache_level = (idx >= (int)CacheLevel_Disabled && idx <= (int)CacheLevel_ReadAsync) ?
		(eCacheLevel)idx : default_cfg_data.cache_level;
}

static void ConfigDialog_SetAsyncTrig(HWND hwnd, int cur_cache_asynctrig)
{
	HWND	hdlg;
	eCacheLevel	tmp_cache_level;

	hdlg = GetDlgItem(hwnd, IDC_SLIDER_ASYNCTRIG);
	SendMessage(hdlg, TBM_SETRANGE, 0, MAKELONG(25, 75));
	SendMessage(hdlg, TBM_SETTICFREQ, (75-25)/3, 0);
	SendMessage(hdlg, TBM_SETPOS, TRUE, (LPARAM)cur_cache_asynctrig);
	if (comctl32_version >= 470) {
		SendMessage(hdlg, TBM_SETTIPSIDE, TBTS_BOTTOM, 0);
		hwndSliderTT = (HWND)SendMessage(hdlg, TBM_GETTOOLTIPS, 0, 0);
	}
	ConfigDialog_GetCacheLevel(hwnd, &tmp_cache_level);
	EnableWindow(hdlg, (tmp_cache_level < CacheLevel_ReadAsync) ? FALSE : TRUE);
}

static void ConfigDialog_GetAsyncTrig(HWND hwnd, int *cache_asynctrig)
{
	*cache_asynctrig = SendMessage(GetDlgItem(hwnd, IDC_SLIDER_ASYNCTRIG), TBM_GETPOS, 0, 0);
}

static void ConfigDialog_UpdateAsyncTrig(HWND hwnd, int *cache_asynctrig)
{
	ConfigDialog_GetAsyncTrig(hwnd, cache_asynctrig);
	ConfigDialog_SetAsyncTrig(hwnd, *cache_asynctrig);
}

static void ConfigDialog_SetTrackIsoFs(HWND hwnd, BOOL cur_track_fsys)
{
	HWND	hdlg;
	eCacheLevel	tmp_cache_level;

	hdlg = GetDlgItem(hwnd, IDC_CHECK_TRACKISOFS);
	Button_SetCheck(hdlg, (cur_track_fsys) ? BST_CHECKED : BST_UNCHECKED);
	ConfigDialog_GetCacheLevel(hwnd, &tmp_cache_level);
	Button_Enable(hdlg, (tmp_cache_level < CacheLevel_ReadMulti) ? FALSE : TRUE);
}

static void ConfigDialog_GetTrackIsoFs(HWND hwnd, BOOL *track_fsys)
{
	*track_fsys = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_TRACKISOFS)) == BST_CHECKED) ? TRUE : FALSE;
}

static void ConfigDialog_UpdateTrackIsoFs(HWND hwnd, BOOL *track_fsys)
{
	ConfigDialog_GetTrackIsoFs(hwnd, track_fsys);
	ConfigDialog_SetTrackIsoFs(hwnd, *track_fsys);
}

static eInterfaceType	last_if_type_for_SubQMode_combo = InterfaceType_Auto;

static void ConfigDialog_SetSubQMode(HWND hwnd, eSubQMode cur_subq_mode)
{
	HWND	hdlg;
    int		idx, cur, num_subq_modes;
	const eSubQMode	*subq_modes;

	hdlg = GetDlgItem(hwnd, IDC_COMBO_SUBREADMODE);
	ComboBox_ResetContent(hdlg);
	// read 'if_type' & save the value for the *_GetSubQMode() call
	ConfigDialog_GetIfType(hwnd, &last_if_type_for_SubQMode_combo);
	num_subq_modes = get_subq_modes_auto(&subq_modes, last_if_type_for_SubQMode_combo);
	for (idx = 0, cur = 0; idx < num_subq_modes; idx++) {
		ComboBox_AddString(hdlg, subq_mode_name(subq_modes[idx]));
		if (cur_subq_mode == subq_modes[idx])
			cur = idx;
	}
	ComboBox_Enable(hdlg, (num_subq_modes <= 1) ? FALSE : TRUE);
	ComboBox_SetCurSel(hdlg, cur);
}

static void ConfigDialog_GetSubQMode(HWND hwnd, eSubQMode *subq_mode)
{
	int		idx, num_subq_modes;
	const eSubQMode	*subq_modes;
	// Note: The 'subq_mode' value is 'if_type' dependent, so we can need ...
	// ... to use the value saved from the previous *_SetSubQMode() call !!!
	num_subq_modes = get_subq_modes_auto(&subq_modes, last_if_type_for_SubQMode_combo);
	// Note: if no choices availables (RAW only), don't need to write the ...
	// ... subq_mode value (don't override the last working SCSI subchannel mode).
	if (num_subq_modes <= 1) return;
	idx = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_SUBREADMODE));
	*subq_mode = (idx >= 0 && idx < num_subq_modes) ?
		subq_modes[idx] : default_cfg_data.subq_mode;
}

static void ConfigDialog_UpdateSubQMode(HWND hwnd, eSubQMode *subq_mode)
{
	ConfigDialog_GetSubQMode(hwnd, subq_mode);
	ConfigDialog_SetSubQMode(hwnd, *subq_mode);
}

static void ConfigDialog_SetUseSubFile(HWND hwnd, BOOL cur_use_subfile)
{
	Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_USESUBFILE), (cur_use_subfile) ? BST_CHECKED : BST_UNCHECKED);
}

static void ConfigDialog_GetUseSubFile(HWND hwnd, BOOL *use_subfile)
{
	*use_subfile = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_USESUBFILE)) == BST_CHECKED) ? TRUE : FALSE;
}

static void ConfigDialog_InstallToolTips(HWND hwnd)
{
	TOOLINFO	ti;
	int		idx;

	if ((hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPSTR) NULL,
		TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, hwnd, (HMENU) NULL, hInstance, NULL)) == NULL)
		return;
	tt_text_long = (char *)HeapAlloc(hProcessHeap, 0, MAX_TOOLTIP_TEXT_LEN + 1);
	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
	for (idx = 0; idx < (sizeof(configdialog_tooltips) / sizeof(configdialog_tooltips[0])); idx++)
	{
		ZeroMemory(&ti, sizeof(TOOLINFO));
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
		ti.hwnd = hwnd;
		ti.uId = (UINT)GetDlgItem(hwnd, configdialog_tooltips[idx].ctrl_id);
		ti.lpszText = LPSTR_TEXTCALLBACK;
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	}
	SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(30*1000,0));
	SendMessage(hwndTT, TTM_ACTIVATE, TRUE, 0);
}

static void ConfigDialog_ToolTipsSetTextCallback(LPNMHDR lPar)
{
	LPTOOLTIPTEXT	lpttt;
	int		idCtrl, idx;

	lpttt = (LPTOOLTIPTEXT)lPar;
	idCtrl = GetDlgCtrlID((HWND)lPar->idFrom);
	if (lPar->hwndFrom == hwndTT) {
		for (idx = 0; idx < (sizeof(configdialog_tooltips) / sizeof(configdialog_tooltips[0])); idx++)
		{
			if (idCtrl != configdialog_tooltips[idx].ctrl_id)
				continue;
			if (comctl32_version >= 470 && configdialog_tooltips[idx].text_long != NULL)
			{
				int len = sprintf(tt_text_long, "%s\n%s", configdialog_tooltips[idx].text_short, configdialog_tooltips[idx].text_long);
#ifdef	DEBUG
				if (len >= MAX_TOOLTIP_TEXT_LEN)
					__asm int 3
#endif//DEBUG
				SendMessage(lPar->hwndFrom, TTM_SETMAXTIPWIDTH, 0, MAX_TOOLTIP_TEXT_LEN);
				lpttt->lpszText = tt_text_long;
			}
			else
				lpttt->lpszText = configdialog_tooltips[idx].text_short;
			break;
		}
	} else if (lPar->hwndFrom == hwndSliderTT) {
		if (idCtrl == IDC_SLIDER_ASYNCTRIG)
			sprintf(lpttt->szText , "%d%%", (int)SendMessage((HWND)lPar->idFrom, TBM_GETPOS, 0, 0));
	}
}

static void ConfigDialog_RemoveToolTips(void)
{
	if (hwndTT != NULL) {
		SendMessage(hwndTT, TTM_ACTIVATE, FALSE, 0);
		DestroyWindow(hwndTT);
		HeapFree(hProcessHeap, 0, tt_text_long);
		tt_text_long = NULL;
	}
	hwndTT = NULL;
}

BOOL CALLBACK ConfigDialogProc(HWND hwnd, UINT msg, WPARAM wPar, LPARAM lPar)
{
	static CFG_DATA config_data;
	static HWND	hwndTT = NULL;

	switch (msg) {
	case WM_INITDIALOG:
		comctl32_GetVersion();	// read the common control DLL version
		fn_ReadCfg(&config_data);
		if (!num_found_drives && scan_AvailDrives())
			setup_IfType(InterfaceType_Auto);	// always reset I/F type to Auto !!!
		ConfigDialog_SetDriveId(hwnd, &config_data.drive_id);
		ConfigDialog_SetIfType(hwnd, config_data.interface_type);
		ConfigDialog_SetRdMode(hwnd, config_data.read_mode);
		ConfigDialog_SetCdSpeed(hwnd, config_data.cdrom_speed);
		ConfigDialog_SetCdSpindown(hwnd, config_data.cdrom_spindown);
		ConfigDialog_SetCacheLevel(hwnd, config_data.cache_level);
		ConfigDialog_SetAsyncTrig(hwnd, config_data.cache_asynctrig);
		ConfigDialog_SetTrackIsoFs(hwnd, config_data.track_fsys);
		ConfigDialog_SetSubQMode(hwnd, config_data.subq_mode);
		ConfigDialog_SetUseSubFile(hwnd, config_data.use_subfile);
		ConfigDialog_InstallToolTips(hwnd);
		return TRUE;

	case WM_DESTROY:
		EndDialog(hwnd,0);
		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR)lPar)->code == TTN_NEEDTEXT)
			ConfigDialog_ToolTipsSetTextCallback((LPNMHDR)lPar);
		break;	//return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wPar)) {
        case IDC_COMBO_INTERFACE:
			// track the 'if_type' combo changes to replace the previous ...
			// ... avail read modes with the only 'if_type allowed' ones
			if (HIWORD(wPar) == CBN_SELCHANGE) {
				ConfigDialog_UpdateRdMode(hwnd, &config_data.read_mode);
				ConfigDialog_UpdateCdSpeed(hwnd, &config_data.cdrom_speed);
				ConfigDialog_UpdateCdSpindown(hwnd, &config_data.cdrom_spindown);
				ConfigDialog_UpdateSubQMode(hwnd, &config_data.subq_mode);
			} break;
		case IDC_COMBO_CACHELEVEL:
			if (HIWORD(wPar) == CBN_SELCHANGE) {
				ConfigDialog_UpdateTrackIsoFs(hwnd, &config_data.track_fsys);
				ConfigDialog_UpdateAsyncTrig(hwnd, &config_data.cache_asynctrig);
			} break;
//		case IDC_BUTTON_TESTMODE:
//			{ int __todo__testmode_button_; }
//			return TRUE;
		case IDOK:
			ConfigDialog_GetDriveId(hwnd, &config_data.drive_id);
			ConfigDialog_GetIfType(hwnd, &config_data.interface_type);
			ConfigDialog_GetRdMode(hwnd, &config_data.read_mode);
			ConfigDialog_GetCdSpeed(hwnd, &config_data.cdrom_speed);
			ConfigDialog_GetCdSpindown(hwnd, &config_data.cdrom_spindown);
			ConfigDialog_GetCacheLevel(hwnd, &config_data.cache_level);
			ConfigDialog_GetAsyncTrig(hwnd, &config_data.cache_asynctrig);
			ConfigDialog_GetTrackIsoFs(hwnd, &config_data.track_fsys);
			ConfigDialog_GetSubQMode(hwnd, &config_data.subq_mode);
			ConfigDialog_GetTrackIsoFs(hwnd, &config_data.track_fsys);
			ConfigDialog_GetUseSubFile(hwnd, &config_data.use_subfile);
			fn_WriteCfg(&config_data);
		case IDCANCEL:
			ConfigDialog_RemoveToolTips();
			EndDialog(hwnd,0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

//------------------------------------------------------------------------------
// Plugin about dialog:
//------------------------------------------------------------------------------

void about_Dialog(HWND hwnd)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
		hwnd, (DLGPROC)AboutDialogProc);
}

static void AboutDialog_SetStaticText(HWND hwnd)
{
	static int	about_static_ctrl_id[] = {
		IDC_STATIC_ABOUT_0, IDC_STATIC_ABOUT_1, IDC_STATIC_ABOUT_2, IDC_STATIC_ABOUT_3,
	};
	int		idx;

	Static_SetText(GetDlgItem(hwnd, IDC_STATIC_PLGNAME), PLUGIN_NAME);
	for (idx = 0; idx < (sizeof(about_static_ctrl_id) / sizeof(about_static_ctrl_id[0])); idx++)
		Static_SetText(GetDlgItem(hwnd, about_static_ctrl_id[idx]), about_line_text(idx));
}

BOOL CALLBACK AboutDialogProc(HWND hwnd, UINT msg, WPARAM wPar, LPARAM lPar)
{
	switch (msg) {
	case WM_INITDIALOG:
		AboutDialog_SetStaticText(hwnd);
		return TRUE;

	case WM_DESTROY:
		EndDialog(hwnd,0);
		return TRUE;

	case WM_COMMAND:
		switch (wPar) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd,0);
			return TRUE;
		}
	}
	return FALSE;
}

