// Plugin Defs
#include "Locals.h"

// Configuration data:
CFG_DATA	cfg_data;

// Misc support data:
OSVERSIONINFO	os_version;
BOOL	bHaveOsVersionFlg = FALSE;

// Drive data & flags:
CDROM_TOC	toc;
SCSIADDR	found_drives[MAX_SCSI_UNITS];
INQUIRYDATA	drive_info[MAX_SCSI_UNITS];
int			num_found_drives = 0;
DWORD		scsi_err = NO_ERROR;
SCSIADDR	scsi_addr = { 0xFF, 0xFF, 0xFF, '\0', };
eReadMode	scsi_rmode = ReadMode_Auto;
eSubQMode	scsi_subQmode = SubQMode_Auto;

// Static sector buffer for no-cache mode reads (2 sectors)
static BYTE	sector_buffer[2][RAW_SECTOR_SIZE];
static unsigned int	sector_buffer_idx = 0;
static BOOL	cache_last_read_esito = FALSE;

// *_StartReadSector/*_WaitEndReadSector status data:
BOOL		pending_read_running = FALSE;

// Drive status data / flags:
BOOL		drive_is_ready = FALSE;
BOOL		plg_is_open = FALSE;
BOOL		play_in_progress = FALSE;
int			prev_spindown_timer = -1;

// Layer access function pointers:
IF_LAYER_CMDS	if_layer;

// Drive access function pointers:
IF_DRIVE_CMDS	if_drive;

//------------------------------------------------------------------------------
// Dummy interface functions:
//------------------------------------------------------------------------------
// layer-specific 'dummy' functions:
BOOL	dummy_LoadLayer(void) { return TRUE; }
void	dummy_UnloadLayer(void) {}
BOOL	dummy_OpenDrive(const SCSIADDR *addr) { scsi_addr = *addr; return TRUE; }
void	dummy_CloseDrive(void) {}
BOOL	dummy_QueryAvailDrives(void) { return TRUE; }
BOOL	dummy_DetectReadMode(eReadMode *rmode) { *rmode = ReadMode_Raw; return TRUE; }
BOOL	dummy_InitReadMode(eReadMode rmode) { scsi_rmode = rmode; return TRUE; }
BOOL	dummy_DetectSubQMode(eSubQMode *subQmode) { *subQmode = MAKE_SUBQMODE(ReadMode_Raw, SubQMode_SubQ); return TRUE; }
BOOL	dummy_InitSubQMode(eSubQMode subQmode) { scsi_subQmode = subQmode; return TRUE; }
BOOL	dummy_ReadToc(void) { return FALSE; }
BOOL	dummy_StartReadSector(BYTE *rbuf, int loc, int nsec) { return FALSE; }
BOOL	dummy_WaitEndReadSector(void) { return FALSE; }
BOOL	dummy_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc) { return FALSE; }
BOOL	dummy_PlayAudioTrack(const MSF *start, const MSF *end) { return TRUE; }
BOOL	dummy_StopAudioTrack(void) { return TRUE; }
BOOL	dummy_TestUnitReady(BOOL do_retry) { return TRUE; }
BOOL	dummy_GetPlayLocation(MSF *play_loc, BYTE *audio_status) { return FALSE; }
BOOL	dummy_LoadEjectUnit(BOOL load) { return TRUE; }
BOOL	dummy_SetCdSpeed(int speed) { return TRUE; }
BOOL	dummy_SetCdSpindown(int timer, int *prev_timer) { return TRUE; }
BOOL	dummy_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries) { return FALSE; }
// cache-specific 'dummy' functions:
BOOL	dummy_ReadToc(BOOL use_cache) { return FALSE; }
BOOL	dummy_StartReadSector(int loc, BYTE **sector_buf) { return FALSE; }
void	dummy_FinishAsyncReadsPending(void) { }

//------------------------------------------------------------------------------
// Misc support functions:
//------------------------------------------------------------------------------

#ifdef	_DEBUG
void DbgPrint(const char *fmt, ...)
{
	va_list	args;
	char	buffer[500];

	va_start(args, fmt);
	_vsnprintf(buffer, sizeof(buffer)-1, fmt, args);
	va_end(args);
	fprintf(stdout,"%s", buffer); fflush(stdout);
	OutputDebugString(buffer);
}
#endif//_DEBUG

int get_if_types_auto(const eInterfaceType **if_types_ary, BOOL from_scanAvailDrives)
{
	// Warning: DO NOT change the following orders, otherwise scan_AvailDrives() ...
	// ... doesn't return all the informations as needed (see aspi_GetDriveInfo()).
	// The correct order must be: SPTI/ASPI under WinNT, and ASPI/MSCDEX under Win9x.
	static const eInterfaceType	if_types_NT_scanDrv[] = {
		InterfaceType_NtSpti, InterfaceType_Aspi };
	//static const eInterfaceType	if_types_9x_scanDrv[] = {
	//	InterfaceType_Aspi, InterfaceType_Mscdex };
	static const eInterfaceType	if_types_NT[] = {
		InterfaceType_NtSpti, InterfaceType_Aspi, InterfaceType_NtRaw };
	static const eInterfaceType	if_types_9x[] = {
		InterfaceType_Aspi, InterfaceType_Mscdex };

	get_OsVersion();
	switch (os_version.dwPlatformId) {
	case VER_PLATFORM_WIN32_NT:			// WinNT, Win2K, WinXP, etc.
		if (from_scanAvailDrives) {
			*if_types_ary = if_types_NT_scanDrv;
			return sizeof(if_types_NT_scanDrv) / sizeof(if_types_NT_scanDrv[0]);
		}
		*if_types_ary = if_types_NT;
		return sizeof(if_types_NT) / sizeof(if_types_NT[0]);
	case VER_PLATFORM_WIN32_WINDOWS:	// Win95, Win98, WinMe, etc.
//		if (from_scanAvailDrives) {
//			*if_types_ary = if_types_9x_scanDrv;
//			return sizeof(if_types_9x_scanDrv) / sizeof(if_types_9x_scanDrv[0]);
//		}
		*if_types_ary = if_types_9x;
		return sizeof(if_types_9x) / sizeof(if_types_9x[0]);
	case VER_PLATFORM_WIN32s:			// Win32s
	default:
		break;
	}
	return 0;
}

int get_rd_modes_auto(const eReadMode **rd_modes_ary, eInterfaceType if_type)
{
	static const eReadMode	rd_modes_SCSI[] = {
		ReadMode_Auto, ReadMode_Scsi_BE, ReadMode_Scsi_28,
		ReadMode_Scsi_D8, ReadMode_Scsi_D4 };
	static const eReadMode	rd_modes_RAW[]  = {
		ReadMode_Raw };

	switch (if_type) {
	case InterfaceType_Auto:
	case InterfaceType_Aspi:
	case InterfaceType_NtSpti:
		*rd_modes_ary = rd_modes_SCSI;
		return sizeof(rd_modes_SCSI) / sizeof(rd_modes_SCSI[0]);
	case InterfaceType_NtRaw:
	case InterfaceType_Mscdex:
		*rd_modes_ary = rd_modes_RAW;
		return sizeof(rd_modes_RAW) / sizeof(rd_modes_RAW[0]);
	default:
		return 0;
	}
}

int get_cd_speeds_auto(const int **cd_speeds_ary, eInterfaceType if_type)
{
	static const int	cd_speeds_SCSI[] = {
		0, 2, 4, 6, 8, 12, 16, 0xFFFF };	// 0 = Default, 0xFFFF = MAX
	static const int	cd_speeds_RAW[]  = {
		0 };

	switch (if_type) {
	case InterfaceType_Auto:
	case InterfaceType_Aspi:
	case InterfaceType_NtSpti:
		*cd_speeds_ary = cd_speeds_SCSI;
		return sizeof(cd_speeds_SCSI) / sizeof(cd_speeds_SCSI[0]);
	case InterfaceType_NtRaw:
	case InterfaceType_Mscdex:
		*cd_speeds_ary = cd_speeds_RAW;
		return sizeof(cd_speeds_RAW) / sizeof(cd_speeds_RAW[0]);
	default:
		return 0;
	}
}

int get_cd_spindowns_auto(const int **cd_spindowns_ary, eInterfaceType if_type)
{
	static const int	cd_spindowns_SCSI[] = {
		INACTIVITY_TIMER_VENDOR_SPECIFIC,	// 0 = Default
		INACTIVITY_TIMER_32S,
		INACTIVITY_TIMER_1MIN,
		INACTIVITY_TIMER_2MIN,
		INACTIVITY_TIMER_4MIN,
		INACTIVITY_TIMER_8MIN,
		INACTIVITY_TIMER_16MIN, };
	static const int	cd_spindowns_RAW[]  = {
		0 };

	switch (if_type) {
	case InterfaceType_Auto:
	case InterfaceType_Aspi:
	case InterfaceType_NtSpti:
		*cd_spindowns_ary = cd_spindowns_SCSI;
		return sizeof(cd_spindowns_SCSI) / sizeof(cd_spindowns_SCSI[0]);
	case InterfaceType_NtRaw:
	case InterfaceType_Mscdex:
		*cd_spindowns_ary = cd_spindowns_RAW;
		return sizeof(cd_spindowns_RAW) / sizeof(cd_spindowns_RAW[0]);
	default:
		return 0;
	}
}

int get_subq_modes_auto(const eSubQMode **subq_modes_ary, eInterfaceType if_type)
{
	static const eSubQMode	subq_modes_SCSI[] = {
		SubQMode_Auto,
		MAKE_SUBQMODE(ReadMode_Scsi_BE, SubQMode_Read_PW),
		MAKE_SUBQMODE(ReadMode_Scsi_BE, SubQMode_Read_Q),
		MAKE_SUBQMODE(ReadMode_Scsi_D8, SubQMode_Read_PW),
		MAKE_SUBQMODE(ReadMode_Scsi_D8, SubQMode_Read_Q),
		MAKE_SUBQMODE(ReadMode_Scsi_28, SubQMode_Read_PW),
		MAKE_SUBQMODE(ReadMode_Scsi_28, SubQMode_Read_Q),
		MAKE_SUBQMODE(ReadMode_Raw, SubQMode_SubQ),
		SubQMode_Disabled };
	static const eSubQMode	subq_modes_RAW[]  = {
		MAKE_SUBQMODE(ReadMode_Raw, SubQMode_SubQ) };

	switch (if_type) {
	case InterfaceType_Auto:
	case InterfaceType_Aspi:
	case InterfaceType_NtSpti:
		*subq_modes_ary = subq_modes_SCSI;
		return sizeof(subq_modes_SCSI) / sizeof(subq_modes_SCSI[0]);
	case InterfaceType_NtRaw:
	case InterfaceType_Mscdex:
		*subq_modes_ary = subq_modes_RAW;
		return sizeof(subq_modes_RAW) / sizeof(subq_modes_RAW[0]);
	default:
		return 0;
	}
}

//------------------------------------------------------------------------------
// INIT support functions:
//------------------------------------------------------------------------------

void get_OsVersion(void)
{
	if (bHaveOsVersionFlg)
		return;
	memset(&os_version, 0, sizeof(os_version));
	os_version.dwOSVersionInfoSize = sizeof(os_version);
	GetVersionEx(&os_version);
	bHaveOsVersionFlg = TRUE;
}

BOOL scan_AvailDrives(void)
{
	BOOL	res = FALSE;
	const eInterfaceType	*if_types_ary;
	int		num_if_types, i;

	num_if_types = get_if_types_auto(&if_types_ary, TRUE);
	for (i = 0; i < num_if_types; i++)
		if (setup_IfType(if_types_ary[i]) && if_layer.QueryAvailDrives())
			return TRUE;	// Note: if OK, exit leaving I/F type unchanged !!!
	setup_IfType(InterfaceType_Auto);	// if KO, reset I/F type to Auto
	return FALSE;
}

BOOL setup_IfType(eInterfaceType if_type)
{
	static eInterfaceType	last_if_type = InterfaceType_Auto;

	if (if_type != last_if_type && if_layer.UnloadLayer != NULL)
		if_layer.UnloadLayer();		// unload previous loaded interface layer

	// Setup layer access function pointers
	memset(&if_layer, 0, sizeof(if_layer));
	switch (if_type)
	{
	case InterfaceType_Aspi:
		if_layer.LoadLayer = aspi_LoadLayer;
		if_layer.UnloadLayer = aspi_UnloadLayer;
		if_layer.OpenDrive = aspi_OpenDrive;
		if_layer.CloseDrive = dummy_CloseDrive;
		if_layer.QueryAvailDrives = aspi_QueryAvailDrives;
		if_layer.ReadToc = scsi_ReadToc;
		if_layer.DetectReadMode = scsi_DetectReadMode;
		if_layer.InitReadMode = scsi_InitReadMode;
		if_layer.DetectSubQMode = scsi_DetectSubQMode;
		if_layer.InitSubQMode = scsi_InitSubQMode;
		if_layer.StartReadSector = aspi_StartReadSector;
		if_layer.WaitEndReadSector = aspi_WaitEndReadSector;
		if_layer.ReadSubQ = scsi_ReadSubQ;
		if_layer.PlayAudioTrack = scsi_PlayAudioTrack;
		if_layer.StopAudioTrack = scsi_StopAudioTrack;
		if_layer.TestUnitReady = scsi_TestUnitReady;
		if_layer.GetPlayLocation = scsi_GetPlayLocation;
		if_layer.LoadEjectUnit = scsi_LoadEjectUnit;
		if_layer.SetCdSpeed = scsi_SetCdSpeed;
		if_layer.SetCdSpindown = scsi_SetCdSpindown;
		if_layer.ExecScsiCmd = aspi_ExecScsiCmd;
		break;
	case InterfaceType_NtSpti:
	case InterfaceType_NtRaw:
		if_layer.LoadLayer = nt_LoadLayer;
		if_layer.UnloadLayer = dummy_UnloadLayer;
		if_layer.OpenDrive = nt_OpenDrive;
		if_layer.CloseDrive = nt_CloseDrive;
		if_layer.QueryAvailDrives = nt_QueryAvailDrives;
		if_layer.ReadToc = nt_ReadToc;
		if (if_type == InterfaceType_NtSpti) {
			if_layer.DetectReadMode = scsi_DetectReadMode;
			if_layer.InitReadMode = scsi_InitReadMode;
			if_layer.DetectSubQMode = scsi_DetectSubQMode;
			if_layer.InitSubQMode = scsi_InitSubQMode;
			if_layer.StartReadSector = nt_spti_StartReadSector;
			if_layer.ReadSubQ = scsi_ReadSubQ;
			if_layer.SetCdSpeed = scsi_SetCdSpeed;
			if_layer.SetCdSpindown = scsi_SetCdSpindown;
		} else {	// InterfaceType_NtRaw
			if_layer.DetectReadMode = dummy_DetectReadMode;
			if_layer.InitReadMode = dummy_InitReadMode;
			if_layer.DetectSubQMode = dummy_DetectSubQMode;
			if_layer.InitSubQMode = dummy_InitSubQMode;
			if_layer.StartReadSector = nt_raw_StartReadSector;
			if_layer.ReadSubQ = nt_ReadSubQ;
			if_layer.SetCdSpeed = dummy_SetCdSpeed;
			if_layer.SetCdSpindown = dummy_SetCdSpindown;
		}
		if_layer.WaitEndReadSector = dummy_WaitEndReadSector;
		if_layer.PlayAudioTrack = nt_PlayAudioTrack;
		if_layer.StopAudioTrack = nt_StopAudioTrack;
		if_layer.TestUnitReady = nt_TestUnitReady;
		if_layer.GetPlayLocation = nt_GetPlayLocation;
		if_layer.LoadEjectUnit = nt_LoadEjectUnit;
		if_layer.ExecScsiCmd = nt_spti_ExecScsiCmd;
		break;
	case InterfaceType_Mscdex:
		if_layer.LoadLayer = mscd_LoadLayer;
		if_layer.UnloadLayer = mscd_UnloadLayer;
		if_layer.OpenDrive = dummy_OpenDrive;
		if_layer.CloseDrive = dummy_CloseDrive;
		if_layer.QueryAvailDrives = mscd_QueryAvailDrives;
		if_layer.ReadToc = mscd_ReadToc;
		if_layer.DetectReadMode = dummy_DetectReadMode;
		if_layer.InitReadMode = dummy_InitReadMode;
		if_layer.DetectSubQMode = dummy_DetectSubQMode;
		if_layer.InitSubQMode = dummy_InitSubQMode;
		if_layer.StartReadSector = mscd_StartReadSector;
		if_layer.WaitEndReadSector = dummy_WaitEndReadSector;
		if_layer.ReadSubQ = mscd_ReadSubQ;
		if_layer.PlayAudioTrack = mscd_PlayAudioTrack;
		if_layer.StopAudioTrack = mscd_StopAudioTrack;
		if_layer.TestUnitReady = mscd_TestUnitReady;
		if_layer.GetPlayLocation = mscd_GetPlayLocation;
		if_layer.LoadEjectUnit = mscd_LoadEjectUnit;
		if_layer.SetCdSpeed = dummy_SetCdSpeed;
		if_layer.SetCdSpindown = dummy_SetCdSpindown;
		if_layer.ExecScsiCmd = dummy_ExecScsiCmd;
		break;
	case InterfaceType_Auto:		// fall back into the 'default' case
	default:
		if_layer.LoadLayer = dummy_LoadLayer;
		if_layer.UnloadLayer = dummy_UnloadLayer;
		if_layer.OpenDrive = dummy_OpenDrive;
		if_layer.CloseDrive = dummy_CloseDrive;
		if_layer.QueryAvailDrives = dummy_QueryAvailDrives;
		if_layer.ReadToc = dummy_ReadToc;
		if_layer.DetectReadMode = dummy_DetectReadMode;
		if_layer.InitReadMode = dummy_InitReadMode;
		if_layer.DetectSubQMode = dummy_DetectSubQMode;
		if_layer.InitSubQMode = dummy_InitSubQMode;
		if_layer.StartReadSector = dummy_StartReadSector;
		if_layer.WaitEndReadSector = dummy_WaitEndReadSector;
		if_layer.ReadSubQ = dummy_ReadSubQ;
		if_layer.PlayAudioTrack = dummy_PlayAudioTrack;
		if_layer.StopAudioTrack = dummy_StopAudioTrack;
		if_layer.TestUnitReady = dummy_TestUnitReady;
		if_layer.GetPlayLocation = dummy_GetPlayLocation;
		if_layer.LoadEjectUnit = dummy_LoadEjectUnit;
		if_layer.SetCdSpeed = dummy_SetCdSpeed;
		if_layer.SetCdSpindown = dummy_SetCdSpindown;
		if_layer.ExecScsiCmd = dummy_ExecScsiCmd;
		break;
	}
	// Setup 'cache-specific' drive access function pointers
	memset(&if_drive, 0, sizeof(if_drive));
	if_drive.ReadToc = dummy_ReadToc;
	if_drive.ReadSubQ = dummy_ReadSubQ;
	if_drive.StartReadSector = dummy_StartReadSector;
	if_drive.WaitEndReadSector = dummy_WaitEndReadSector;
	if_drive.FinishAsyncReadsPending = dummy_FinishAsyncReadsPending;
	// Setup 'layer-specific' drive access function pointers
	if_drive.PlayAudioTrack = if_layer.PlayAudioTrack;
	if_drive.StopAudioTrack = if_layer.StopAudioTrack;
	if_drive.TestUnitReady = if_layer.TestUnitReady;
	if_drive.GetPlayLocation = if_layer.GetPlayLocation;
	if_drive.LoadEjectUnit = if_layer.LoadEjectUnit;

	if (!if_layer.LoadLayer())
	{
		setup_IfType(InterfaceType_Auto);	// restore default values
		return FALSE;
	}
	last_if_type = if_type;			// save last i/f type
	return TRUE;
}

void setup_CacheLevel(eCacheLevel cache_level)
{
	// Setup 'cache-specific' drive access function pointers
	switch (cache_level)
	{
	case CacheLevel_Disabled:
		if_drive.ReadToc = cache_ReadToc;
		if_drive.ReadSubQ = cache_ReadSubQ;
		if_drive.StartReadSector = nocache_StartReadSector;
		if_drive.WaitEndReadSector = nocache_WaitEndReadSector;
		break;
	case CacheLevel_ReadOne:
	case CacheLevel_ReadMulti:
	case CacheLevel_ReadAsync:
	default:
		if_drive.ReadToc = cache_ReadToc;
		if_drive.ReadSubQ = cache_ReadSubQ;
		if_drive.StartReadSector = cache_StartReadSector;
		if_drive.WaitEndReadSector = cache_WaitEndReadSector;
		if_drive.FinishAsyncReadsPending = cache_FinishAsyncReadsPending;
		break;
	}
}

BOOL open_Driver(void)
{
	const eInterfaceType	*cur_if_type;
	int		num_if_types;
	BOOL	autodetect_if_type;
	const SCSIADDR	*cur_drive;
	int		num_drives;
	BOOL	autodetect_drive;
	const int	*cur_cd_speed, *cur_cd_spindown;
	int		num_cd_speeds, num_cd_spindowns;
	int		tmp_cd_speed, tmp_cd_spindown;
	const eReadMode	*cur_rd_mode;
	int		num_rd_modes;
	eReadMode	tmp_rd_mode;
	const eSubQMode	*cur_subq_mode;
	int		num_subq_modes;
	eSubQMode	tmp_subq_mode;
	int		idx, i, j;
	static const char	msgbox_title[] = PLUGIN_NAME;

	// check if the cfg_data values for if_type & drive_id are ok (if not ok, use autodetection)
	num_if_types = get_if_types_auto(&cur_if_type, FALSE);
	for (idx = 0, autodetect_if_type = TRUE; idx < num_if_types; idx++)
		if (cfg_data.interface_type == cur_if_type[idx]) {
			cur_if_type = &cfg_data.interface_type; num_if_types = 1;
			autodetect_if_type = FALSE; break; }
	cur_drive = &found_drives[0];
	num_drives = num_found_drives;
	for (idx = 0, autodetect_drive = TRUE; idx < num_drives; idx++)
		if (SCSIADDR_EQ(cfg_data.drive_id, cur_drive[idx])) {
			cur_drive = &cfg_data.drive_id; num_drives = 1;
			autodetect_drive = FALSE; break; }

	for (i = 0; i < num_if_types; i++) {
		if (!setup_IfType(cur_if_type[i]))	// Setup 'Interface Type'
			continue;
		for (j = 0, idx = -1; j < num_drives; j++) {
			if (!if_layer.OpenDrive(&cur_drive[j]))	// Open Drive
				continue;
			if (idx == -1) idx = j;		// save first opened ok drive idx
			// issue a spare 'test unit read' command to cleanup 'disk changed' sense errors
			if_layer.TestUnitReady(TRUE);
			if (!autodetect_drive || num_drives == 1 ||
				if_layer.TestUnitReady(TRUE))		// test unit ready
				break;
			if_layer.CloseDrive();
		}
		if (j < num_drives || (idx != -1 && if_layer.OpenDrive(&cur_drive[j = idx])))
			break;
	}
	if (i >= num_if_types) {
		setup_IfType(InterfaceType_Auto);
		MessageBox(NULL, "can't open cdrom unit", msgbox_title, MB_ICONERROR | MB_OK);
		return FALSE;
	}
	cfg_data.interface_type = cur_if_type[i];	// save the value for 'if_type'
	cfg_data.drive_id = cur_drive[j];	// save the value for 'drive_id'
	PRINTF(("Selected Drive Id  = [%d:%d:%d] %c\n", (int)cfg_data.drive_id.haid, (int)cfg_data.drive_id.target, (int)cfg_data.drive_id.lun, (char)cfg_data.drive_id.drive_letter));
	PRINTF(("Selected If Type   = %s\n", if_type_name(cfg_data.interface_type)));

	// check if the cfg_data value for rd_mode is ok (if not ok, use autodetection)
	num_rd_modes = get_rd_modes_auto(&cur_rd_mode, cfg_data.interface_type);
	for (idx = 0, tmp_rd_mode = cur_rd_mode[0]; idx < num_rd_modes; idx++)
		if (cfg_data.read_mode == cur_rd_mode[idx]) {
			tmp_rd_mode = cfg_data.read_mode; break;
		}
	if (tmp_rd_mode == ReadMode_Auto) {	// try to detect read mode from drive capabilities
		if (!if_layer.DetectReadMode(&tmp_rd_mode))
		{	// if can't detected read mode, try every single mode manually...
			for (i = 0; i < num_rd_modes; i++)
				if (if_layer.InitReadMode(cur_rd_mode[i]) &&
					(test_ReadMode() ||
					 scsi_err == ERROR_NOT_READY))	// if 'no disk' error, assume read mode ok
					break;
			if (i >= num_rd_modes) {
				if_layer.InitReadMode(ReadMode_Auto);	// if failed, reset block size ...
				if_layer.CloseDrive();		// ... to default and return with error
				setup_IfType(InterfaceType_Auto);
				MessageBox(NULL, "can't detect read mode", msgbox_title, MB_ICONERROR | MB_OK);
				return FALSE;
			}
			tmp_rd_mode = cur_rd_mode[i];	// set detected read mode value
		}
	}
	if (!if_layer.InitReadMode(tmp_rd_mode)) {	// try to initialize the read mode (if needed, issue a mode select to change the block size)
		if_layer.InitReadMode(ReadMode_Auto);	// if failed, reset block size ...
		if_layer.CloseDrive();			// ... to default and return with error
		setup_IfType(InterfaceType_Auto);
		MessageBox(NULL, "can't initialize read mode", msgbox_title, MB_ICONERROR | MB_OK);
		return FALSE;
	}
	cfg_data.read_mode = tmp_rd_mode;	// save the value for 'rd_mode'
	PRINTF(("Selected Read Mode = %s\n", rd_mode_name(tmp_rd_mode)));

	// check if the cfg_data value for subq_mode is ok (if not ok, try autodetection)
	num_subq_modes = get_subq_modes_auto(&cur_subq_mode, cfg_data.interface_type);
	for (idx = 0, tmp_subq_mode = cur_subq_mode[0]; idx < num_subq_modes; idx++)
		if (cfg_data.subq_mode == cur_subq_mode[idx]) {
			tmp_subq_mode = cfg_data.subq_mode; break;
		}
	if (tmp_subq_mode == SubQMode_Auto)	// try to detect subchannel read mode
		if (if_layer.DetectSubQMode(&tmp_subq_mode)) {	// (if failed, retry on first subchannel read)
		}
	cfg_data.subq_mode = tmp_subq_mode;	// save the value for 'subq_mode'
	if_layer.InitSubQMode(cfg_data.subq_mode);
	PRINTF(("Selected SubQ Mode = %s\n", subq_mode_name(tmp_subq_mode)));

	// check if the cfg_data value for cd_speed is ok (if not ok, use default)
	num_cd_speeds = get_cd_speeds_auto(&cur_cd_speed, cfg_data.interface_type);
	for (idx = 0, tmp_cd_speed = cur_cd_speed[0]; idx < num_cd_speeds; idx++)
		if (cfg_data.cdrom_speed == cur_cd_speed[idx]) {
			tmp_cd_speed = cfg_data.cdrom_speed; break;
		}
	cfg_data.cdrom_speed = tmp_cd_speed;	// save the value for 'cd_speed'
	if (cfg_data.cdrom_speed != 0)		// 0 = Default (no change)
		if_layer.SetCdSpeed(cfg_data.cdrom_speed);

	// check if the cfg_data value for cd_spindown is ok (if not ok, use default)
	num_cd_spindowns = get_cd_spindowns_auto(&cur_cd_spindown, cfg_data.interface_type);
	for (idx = 0, tmp_cd_spindown = cur_cd_spindown[0]; idx < num_cd_spindowns; idx++)
		if (cfg_data.cdrom_spindown == cur_cd_spindown[idx]) {
			tmp_cd_spindown = cfg_data.cdrom_spindown; break;
		}
	cfg_data.cdrom_spindown = tmp_cd_spindown;	// save the value for 'cd_spindown'
	if (cfg_data.cdrom_spindown != 0)	// 0 = Default (no change)
		if_layer.SetCdSpindown(cfg_data.cdrom_spindown, &prev_spindown_timer);

	// allocate cache buffers
	if (cache_Alloc(&cfg_data))
		setup_CacheLevel(cfg_data.cache_level);
	else {								// if cache allocation failed ...
		MessageBox(NULL, "cache allocation failed:\ncontinuing with cache disabled", msgbox_title, MB_ICONWARNING | MB_OK);
		setup_CacheLevel(CacheLevel_Disabled);	// ... continue with cache disabled
	}
	cache_FlushBuffers();				// ensure that no cached data is present
	return TRUE;
}

BOOL drive_IsReady(void)
{
	if (drive_is_ready) 
		return TRUE;
	if (!if_layer.TestUnitReady(TRUE))
		return FALSE;
	drive_is_ready = TRUE;
	// TODO: spostare qui scansione drives disponibili (se 'Autodetect Drive' mode)
	{ int __TODO__move_here_scan_cur_drive_if_autodetect_enabled__; }

	// if required, read boot file name from 'system.cnf' file:
	if (cfg_data.use_subfile) {
		char	psx_boot_file_name[20+1], file_buff[COOKED_SECTOR_SIZE+1], *p, *q;
		int		file_len = sizeof(file_buff)-1;

		if_layer.WaitEndReadSector();	// ensure no other read operations pending
		psx_boot_file_name[0] = '\0';
		if (iso9660_read_file("/system.cnf", file_buff, &file_len) && file_len > 0) {
			file_buff[file_len] = '\0';
			for (p = file_buff; *p; p = q) {
				if (sscanf(p, "BOOT = cdrom%*[:\\]%20[^; \r\n]", psx_boot_file_name) == 1)
					break;				//scan for line 'BOOT=cdrom:\\nnnn_nnn.nn;1' or 'BOOT=cdrom:nnnn_nnn.nn;1'
				if ((q = strchr(p, '\n')) != NULL)
					q++;
			}
		}
		cache_LoadSubFile(psx_boot_file_name);
	}
	return drive_is_ready;
}

void close_Driver(void)
{
	if (pending_read_running)			// finish any pending read operation
		if_layer.WaitEndReadSector();
	cache_Free();						// free cache buffers & stop async read thread
	setup_CacheLevel(CacheLevel_Disabled);	// reset fn_cache* ptrs to nocache_*
	if_layer.InitSubQMode(SubQMode_Auto);	// reset subchannel read mode
	if_layer.InitReadMode(ReadMode_Auto);	// reset default block size
	if (cfg_data.cdrom_speed != 0)
		if_layer.SetCdSpeed(0xFFFF);	// if changed, reset cdrom speed
	if (cfg_data.cdrom_spindown != 0 && prev_spindown_timer >= 0)
		if_layer.SetCdSpindown(prev_spindown_timer, NULL);// if changed, reset cdrom spindown
	prev_spindown_timer = -1;
	if_layer.CloseDrive();				// close drive
	setup_IfType(InterfaceType_Auto);	// reset if_* ptrs to dummy_*
}

BOOL test_ReadMode(void)
{
	int		i;
	BYTE	read_buffer[2][RAW_SECTOR_SIZE];	// attempt to reads 2 RAW sectors
	static const BYTE	sync_field_pattern[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	const int	loc = 16;

	memset(read_buffer, 0, sizeof(read_buffer));
	if (!if_layer.StartReadSector(read_buffer[0], loc, sizeof(read_buffer) / sizeof(read_buffer[0])) ||
		(pending_read_running && !if_layer.WaitEndReadSector()))
		return FALSE;
	for (i = 0; i < (sizeof(read_buffer) / sizeof(read_buffer[0])); i++)
		if (memcmp(&read_buffer[i][0], sync_field_pattern, 12) ||
			MSFBCD2INT((MSF *)&read_buffer[i][12]) != (loc + i))
			return FALSE;
	return TRUE;
}

BOOL nocache_StartReadSector(int loc, BYTE **sector_buf)
{
	PRINTF((" R%d", loc));
	if (sector_buffer_idx >= (sizeof(sector_buffer) / sizeof(sector_buffer[0])))
		sector_buffer_idx = 0;
	if_drive.WaitEndReadSector();	// ensure no missing '*_WaitEndReadSector()' call
	*sector_buf = sector_buffer[sector_buffer_idx++];
	if (!if_layer.StartReadSector(*sector_buf, loc, 1))
		return cache_last_read_esito = FALSE;
	return cache_last_read_esito = TRUE;
}

BOOL nocache_WaitEndReadSector(void)
{
	if (pending_read_running && !if_layer.WaitEndReadSector())
		return cache_last_read_esito = FALSE;
	return cache_last_read_esito;
}

