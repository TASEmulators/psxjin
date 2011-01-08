
#include <windows.h>
#include <stdio.h>
#include <process.h>
// SCSI commands & CDB structs
#include "scsi.h"

//******************************************************************************

// PLUGIN Name & Version (Version values must be in BCD format):
#define	PLUGIN_REVISION	0
#define	PLUGIN_BUILD	3
#define	PLUGIN_NAME		"TAS CD-ROM Plugin"	// for user I/F (MessageBox, About dialog, etc)
#define	FPSE_PLUGIN_AUTHOR		"SaPu"		// for FPSE About info
#define	FPSE_PLUGIN_NAME		"CdrSaPu"	// for FPSE About info
#define	FPSE_PLUGIN_CFGNAME		"CDRSAPU"	// for FPSE config data (GetPrivateProfileString, etc)
#define	PSEMU_PLUGIN_CFGNAME	"CDR"		// for PSEMU config data (RegOpenKey, etc)

#ifdef	_DEBUG
#define	PRINTF(a)	DbgPrint a
extern void DbgPrint(const char *fmt, ...);
#else //_DEBUG
#define	PRINTF(a)
#endif//_DEBUG

// Misc defines (not included in "scsi.h")
#define	COOKED_SECTOR_SIZE		2048
#define	RAW_SECTOR_SIZE			2352
#define	RAW_Q_SECTOR_SIZE		2368
#define	RAW_PW_SECTOR_SIZE		2448
#define	SCSI_CDROM_TIMEOUT		10
#define	MAX_SCSI_UNITS			64	// same as MAX_SCSI_LUNS in "scsidefs.h"

#define	MAX_SECTORS_PER_READ	(65535/RAW_SECTOR_SIZE)	// ASPI/MSCDEX limit is 64K (27*2352) !!!

#define MSF_OFFSET		(2 * 75)	// MSF offset of first frame

//******************************************************************************

// Interface layer types:
typedef enum {
	InterfaceType_Auto = 0,
	InterfaceType_Aspi,			// ASPI
	InterfaceType_NtSpti,		// NT SPTI
	InterfaceType_NtRaw,		// NT RAW
	InterfaceType_Mscdex,		// MSCDEX
} eInterfaceType;

// Sector read mode types:
typedef enum {
	ReadMode_Auto = 0,
	ReadMode_Raw,
	ReadMode_Scsi_28,			// SCSIOP_READ
	ReadMode_Scsi_BE,			// SCSIOP_READ_CD
	ReadMode_Scsi_D4,			// SCSIOP_NEC_READ_CDDA
	ReadMode_Scsi_D8,			// SCSIOP_PLXTR_READ_CDDA
} eReadMode;

// Subchannel read mode types:
typedef enum {
	SubQMode_Auto = 0,
	SubQMode_Disabled,			// don't read subchannel from drive
	SubQMode_Read_Q,			// read subchannel Q (2368 bytes) from drive
	SubQMode_Read_PW,			// read subchannel P-W (2448 bytes) from drive
	SubQMode_SubQ,				// read subQ only (12 bytes) from drive
} eSubQMode;
// Note: The value 'subq_mode' is composed from both sector & subchannel read modes
#define	MAKE_SUBQMODE(rmode, qmode)	((eSubQMode)(((int)rmode << 8) | (int)qmode))
#define	SUBQ_MODE(subQmode)		((eSubQMode)((int)subQmode & 0xff))
#define	SUBQ_RMODE(subQmode)	((eReadMode)(((int)subQmode >> 8) & 0xff))

// Sector caching levels:
typedef enum {
	CacheLevel_Disabled = 0,	// no cache
	CacheLevel_ReadOne,			// cache enabled, read 1 sector/time
	CacheLevel_ReadMulti,		// as before, but read 27 sectors/time
	CacheLevel_ReadAsync,		// as before, with additional async reads
} eCacheLevel;

//******************************************************************************

typedef	struct {
	BYTE	minute;
	BYTE	second;
	BYTE	frame;
} MSF;

typedef struct {
	BYTE	haid;
	BYTE	target;
	BYTE	lun;
	BYTE	drive_letter;
} SCSIADDR;
//#define	SCSIADDR_EQ(addr1, addr2)	(addr1.haid == addr2.haid && addr1.target == addr2.target && addr1.lun == addr2.lun && addr1.drive_letter == addr1.drive_letter)
#define	SCSIADDR_EQ(addr1, addr2)	(*((DWORD *)&addr1) == *((DWORD *)&addr2))

// Configuration data struct:
typedef struct {
	SCSIADDR		drive_id;
	eInterfaceType	interface_type;
	eReadMode		read_mode;
	int				cdrom_speed;
	int				cdrom_spindown;
	eCacheLevel		cache_level;
	int				cache_asynctrig;
	BOOL			track_fsys;
	eSubQMode		subq_mode;
	BOOL			use_subfile;
} CFG_DATA;

typedef struct {
	int		start_loc;
	int		num_sectors;
} CACHE_LOC;

// Cache buffer struct:
typedef struct CACHE_BUFFER {
	CACHE_BUFFER	*prev;		// linked list to prev. buffer
	CACHE_BUFFER	*next;		// linked list to next. buffer
	int		num_loc_slots;		// nr. of used data_loc[] entries
	int		num_data_slots;		// nr. of used data[] buffers
	CACHE_LOC	data_loc[MAX_SECTORS_PER_READ];
	BYTE	data[MAX_SECTORS_PER_READ][RAW_SECTOR_SIZE];
} CACHE_BUFFER;

// Cache buffer list header struct:
typedef struct {
	CACHE_BUFFER	*top;
	CACHE_BUFFER	*bottom;
} CACHE_BUFFER_LIST;


// Formatted Q Sub-channel response data struct:
typedef struct {
	BYTE	ADR:4;
	BYTE	Control:4;
	BYTE	TrackNumber;
	BYTE	IndexNumber;
	BYTE	TrackRelativeAddress[3];	// BCD encoded
	BYTE	Filler;
	BYTE	AbsoluteAddress[3];			// BCD encoded
	BYTE	Crc[2];
	BYTE	Reserved[3];
	BYTE	Reserved1:7;
	BYTE	PSubCode:1;
} Q_SUBCHANNEL_DATA;

#ifndef	_NTDDCDRM_	// if not yet included "ntddcdrm.h"...
// Note: The following section was copied from "ntddcdrm.h" (to avoid NT-specific includes):
// Maximum CD Rom size
#define MAXIMUM_NUMBER_TRACKS 100
// Audio Status Codes
#define AUDIO_STATUS_NOT_SUPPORTED  0x00
#define AUDIO_STATUS_IN_PROGRESS    0x11
#define AUDIO_STATUS_PAUSED         0x12
#define AUDIO_STATUS_PLAY_COMPLETE  0x13
#define AUDIO_STATUS_PLAY_ERROR     0x14
#define AUDIO_STATUS_NO_STATUS      0x15
// ADR Sub-channel Q Field
#define ADR_NO_MODE_INFORMATION     0x0
#define ADR_ENCODES_CURRENT_POSITION 0x1
#define ADR_ENCODES_MEDIA_CATALOG   0x2
#define ADR_ENCODES_ISRC            0x3
// Sub-channel Q Control Bits
#define AUDIO_WITH_PREEMPHASIS      0x1
#define DIGITAL_COPY_PERMITTED      0x2
#define AUDIO_DATA_TRACK            0x4
#define TWO_FOUR_CHANNEL_AUDIO      0x8

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;
typedef struct _CDROM_TOC {
    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;
    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;
#endif//_NTDDCDRM_
#ifndef	_NTDDSCSIH_	// if not yet included "ntddscsi.h"...
// Note: The following section was copied from "ntddscsi.h" (to avoid NT specific includes):
#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2
#endif//_NTDDSCSIH_

// Layer access function pointers:
typedef struct {
	BOOL	(*LoadLayer)(void);
	void	(*UnloadLayer)(void);
	BOOL	(*OpenDrive)(const SCSIADDR *addr);
	void	(*CloseDrive)(void);
	BOOL	(*QueryAvailDrives)(void);
	BOOL	(*DetectReadMode)(eReadMode *rmode);
	BOOL	(*InitReadMode)(eReadMode rmode);
	BOOL	(*DetectSubQMode)(eSubQMode *subQmode);
	BOOL	(*InitSubQMode)(eSubQMode subQmode);
	BOOL	(*ReadToc)(void);
	BOOL	(*StartReadSector)(BYTE *rbuf, int loc, int nsec);
	BOOL	(*WaitEndReadSector)(void);
	BOOL	(*ReadSubQ)(Q_SUBCHANNEL_DATA *rbuf, int loc);
	BOOL	(*PlayAudioTrack)(const MSF *start, const MSF *end);
	BOOL	(*StopAudioTrack)(void);
	BOOL	(*TestUnitReady)(BOOL do_retry);
	BOOL	(*GetPlayLocation)(MSF *play_loc, BYTE *audio_status);
	BOOL	(*LoadEjectUnit)(BOOL load);
	BOOL	(*SetCdSpeed)(int speed);
	BOOL	(*SetCdSpindown)(int timer, int *prev_timer);
	BOOL	(*ExecScsiCmd)(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries);
} IF_LAYER_CMDS;

// Drive access function pointers:
typedef struct {
	// cache-specific functions:
	BOOL	(*ReadToc)(BOOL use_cache);
	BOOL	(*ReadSubQ)(Q_SUBCHANNEL_DATA *subq_buf, int loc);
	BOOL	(*StartReadSector)(int loc, BYTE **sector_buf);
	BOOL	(*WaitEndReadSector)(void);
	void	(*FinishAsyncReadsPending)(void);
	// layer-specific functions:
	BOOL	(*PlayAudioTrack)(const MSF *start, const MSF *end);
	BOOL	(*StopAudioTrack)(void);
	BOOL	(*TestUnitReady)(BOOL do_retry);
	BOOL	(*GetPlayLocation)(MSF *play_loc, BYTE *audio_status);
	BOOL	(*LoadEjectUnit)(BOOL load);
} IF_DRIVE_CMDS;

//******************************************************************************

// INIT support functions:
void	get_OsVersion(void);
BOOL	scan_AvailDrives(void);
BOOL	setup_IfType(eInterfaceType if_type);
BOOL	open_Driver(void);
BOOL	drive_IsReady(void);
void	close_Driver(void);
BOOL	test_ReadMode(void);

// SCSI CDB support functions:
BYTE	scsi_BuildInquiryCDB(CDB *cdb, int inquiry_len);
BYTE	scsi_BuildReadTocCDB(CDB *cdb, int data_len);
BYTE	scsi_BuildModeSenseCDB(CDB *cdb, int page_code, int sense_len, BOOL dbd, BOOL use6Byte);
BYTE	scsi_BuildModeSelectCDB(CDB *cdb, int sense_len, BOOL use6Byte);
BYTE	scsi_BuildReadSectorCDB(CDB *cdb, int loc, int nsec, const SCSIADDR *addr, eReadMode rmode, eSubQMode subQmode);
BYTE	scsi_BuildPlayAudioMSFCDB(CDB *cdb, const MSF *start, const MSF *end);
BYTE	scsi_BuildStartStopCDB(CDB *cdb, BOOL start, BOOL load_eject, const SCSIADDR *addr);
BYTE	scsi_BuildPauseResumeCDB(CDB *cdb, BYTE action, const SCSIADDR *addr);
BYTE	scsi_BuildTestUnitReadyCDB(CDB *cdb, const SCSIADDR *addr);
BYTE	scsi_BuildGetConfigurationCDB(CDB *cdb, BYTE type, int feature, int len);
BYTE	scsi_BuildReadQChannelCDB(CDB *cdb, BYTE format);
BYTE	scsi_BuildSeekCDB(CDB *cdb, int loc, const SCSIADDR *addr);
BYTE	scsi_BuildSetCdSpeedCDB(CDB *cdb, int speed);

// SCSI common interface functions (ASPI/SPTI):
BOOL	scsi_Inquiry(INQUIRYDATA *inquiry);
BOOL	scsi_ModeSense(int page_code, char *sense_buf, int sense_len, BOOL dbd, BOOL use6Byte);
BOOL	scsi_DetectReadMode(eReadMode *rmode);
BOOL	scsi_InitReadMode(eReadMode rmode);
BOOL	scsi_DetectSubQMode(eSubQMode *subQmode);
BOOL	scsi_InitSubQMode(eSubQMode subQmode);
BOOL	scsi_ReadToc(void);
BOOL	scsi_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc);
BOOL	scsi_PlayAudioTrack(const MSF *start, const MSF *end);
BOOL	scsi_StopAudioTrack(void);
BOOL	scsi_TestUnitReady(BOOL do_retry);
BOOL	scsi_GetPlayLocation(MSF *play_loc, BYTE *audio_status);\
BOOL	scsi_LoadEjectUnit(BOOL load);
BOOL	scsi_SetCdSpeed(int speed);
BOOL	scsi_SetCdSpindown(int timer, int *prev_timer);

// SCSI other support functions:
BOOL	scsi_BuildModeSelectDATA(char *sense_buf, int *sense_len, int mode_block_size, void *mode_pages_data, int mode_pages_len, BOOL use6Byte);
BOOL	scsi_ParseModeSenseBuffer(const char *sense_buf, const void *parameter_block_ary[], int *num_parameter_blocks, const void *mode_pages_ary[], int *num_mode_pages, BOOL use6Byte);
DWORD	scsi_RemapSenseDataError(const SENSE_DATA *sense_area, BOOL *retry, DWORD *delay);

// SCSI misc support functions:
int		MSF2INT(const MSF *msf);
void	INT2MSF(MSF *msf, int i);
int		MSFBCD2INT(const MSF *msf);
void	INT2MSFBCD(MSF *msf, int i);
extern const BYTE	int2bcd_table[256];
extern const BYTE	bcd2int_table[256];
#define	INT2BCD(a)	int2bcd_table[a]
#define	BCD2INT(a)	bcd2int_table[a]
void	scsi_DecodeSubPWtoQ(Q_SUBCHANNEL_DATA *q_sub, const BYTE *pw_sub);
WORD	scsi_ComputeSubQCrc(const Q_SUBCHANNEL_DATA *q_sub);
BOOL	scsi_ValidateSubQData(Q_SUBCHANNEL_DATA *q_sub);
void	scsi_BuildSubQData(Q_SUBCHANNEL_DATA *q_sub, int loc);

// ASPI interface functions (Win9x/NT):
BOOL	aspi_LoadLayer(void);
void	aspi_UnloadLayer(void);
BOOL	aspi_OpenDrive(const SCSIADDR *addr);
BOOL	aspi_QueryAvailDrives(void);	// (used for Win9x only)
BOOL	aspi_GetDriveType(BYTE *drv_type, const SCSIADDR *addr);
BOOL	aspi_GetDriveInfo(void);
BOOL	aspi_StartReadSector(BYTE *rbuf, int loc, int nsec);
BOOL	aspi_WaitEndReadSector(void);
BOOL	aspi_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries);

// SPTI/RAW interface functions (WinNT/2K):
BOOL	nt_LoadLayer(void);
BOOL	nt_OpenDrive(const SCSIADDR *addr);
void	nt_CloseDrive(void);
BOOL	nt_QueryAvailDrives(void);
BOOL	nt_GetDriveType(BYTE *drv_type, const SCSIADDR *addr);
BOOL	nt_GetDriveInfo(void);
BOOL	nt_Inquiry(INQUIRYDATA *inquiry);
BOOL	nt_InitReadMode(eReadMode rmode);
BOOL	nt_ReadToc(void);
BOOL	nt_spti_StartReadSector(BYTE *rbuf, int loc, int nsec);
BOOL	nt_raw_StartReadSector(BYTE *rbuf, int loc, int nsec);
BOOL	nt_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc);
BOOL	nt_PlayAudioTrack(const MSF *start, const MSF *end);
BOOL	nt_StopAudioTrack(void);
BOOL	nt_TestUnitReady(BOOL do_retry);
BOOL	nt_GetPlayLocation(MSF *play_loc, BYTE *audio_status);
BOOL	nt_LoadEjectUnit(BOOL load);
BOOL	nt_spti_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries);

// MSCDEX interface functions (Win9x only):
BOOL	mscd_LoadLayer(void);
void	mscd_UnloadLayer(void);
BOOL	mscd_QueryAvailDrives(void);
BOOL	mscd_ReadToc(void);
BOOL	mscd_StartReadSector(BYTE *rbuf, int loc, int nsec);
BOOL	mscd_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc);
BOOL	mscd_PlayAudioTrack(const MSF *start, const MSF *end);
BOOL	mscd_StopAudioTrack(void);
BOOL	mscd_TestUnitReady(BOOL do_retry);
BOOL	mscd_GetPlayLocation(MSF *play_loc, BYTE *audio_status);
BOOL	mscd_LoadEjectUnit(BOOL load);

// Dummy interface functions:
BOOL	dummy_LoadLayer(void);
void	dummy_UnloadLayer(void);
BOOL	dummy_OpenDrive(const SCSIADDR *addr);
void	dummy_CloseDrive(void);
BOOL	dummy_QueryAvailDrives(void);
BOOL	dummy_DetectReadMode(eReadMode *rmode);
BOOL	dummy_InitReadMode(eReadMode rmode);
BOOL	dummy_DetectSubQMode(eSubQMode *subQmode);
BOOL	dummy_InitSubQMode(eSubQMode subQmode);
BOOL	dummy_ReadToc(void);
BOOL	dummy_StartReadSector(BYTE *rbuf, int loc, int nsec);
BOOL	dummy_WaitEndReadSector(void);
void	dummy_FinishAsyncReadsPending(void);
BOOL	dummy_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc);
BOOL	dummy_PlayAudioTrack(const MSF *start, const MSF *end);
BOOL	dummy_StopAudioTrack(void);
BOOL	dummy_TestUnitReady(BOOL do_retry);
BOOL	dummy_GetPlayLocation(MSF *play_loc, BYTE *audio_status);
BOOL	dummy_LoadEjectUnit(BOOL load);
BOOL	dummy_SetCdSpeed(int speed);
BOOL	dummy_SetCdSpindown(int timer, int *prev_timer);
BOOL	dummy_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries);

// Cache & Read Buffering functions:
BOOL	cache_Alloc(const CFG_DATA *cfg);
void	cache_FlushBuffers(void);
void	cache_Free(void);
BOOL	cache_ReadToc(BOOL use_cache);
BOOL	cache_StartReadSector(int loc, BYTE **sector_buf);
BOOL	cache_WaitEndReadSector(void);
void	cache_FinishAsyncReadsPending(void);
BOOL	cache_ReadSubQ(Q_SUBCHANNEL_DATA *subq_buf, int loc);
void	cache_LoadSubFile(const char *psx_boot_file_name);
BOOL	nocache_StartReadSector(int loc, BYTE **sector_buf);
BOOL	nocache_WaitEndReadSector(void);

// Iso9660 Fsys Tracking functions:
BOOL	iso9660_Alloc(void);
void	iso9660_Free(void);
BOOL	iso9660_get_sector(int loc, int *start, int *end);
void	iso9660_put_sector(const BYTE *buffer, int loc);
BOOL	iso9660_read_file(const char *file_path, char *data_buff, int *data_size);

// Generic plugin functions:
void	cfg_SetReadWriteCfgFn(void (*readCfgFn)(CFG_DATA *), void (*writeCfgFn)(const CFG_DATA *));
void	cfg_InitDefaultValue(CFG_DATA *cfg);

// Plugin Config / About dialogs:
void	config_Dialog(HWND hwnd);
void	about_Dialog(HWND hwnd);

// Plugin Config / About dialog support functions:
const char	*if_type_name(eInterfaceType if_type);
const char	*rd_mode_name(eReadMode rmode);
const char	*cd_speed_name(int speed);
const char	*cd_spindown_name(int timer);
const char	*subq_mode_name(eSubQMode subQmode);
const char	*cache_level_name(eCacheLevel cache_level);
const char	*about_line_text(int nline);

// Misc support functions:
int		get_if_types_auto(const eInterfaceType **if_types_ary, BOOL from_scanAvailDrives);
int		get_rd_modes_auto(const eReadMode **rd_modes_ary, eInterfaceType if_type);
int		get_cd_speeds_auto(const int **cd_speeds_ary, eInterfaceType if_type);
int		get_cd_spindowns_auto(const int **cd_spindowns_ary, eInterfaceType if_type);
int		get_subq_modes_auto(const eSubQMode **subq_modes_ary, eInterfaceType if_type);

//******************************************************************************

// Configuration data:
extern CFG_DATA	cfg_data;

// Misc support data:
extern OSVERSIONINFO	os_version;
extern BOOL	bHaveOsVersionFlg;
extern HINSTANCE	hInstance;
extern HANDLE	hProcessHeap;

// Drive data & flags:
extern CDROM_TOC	toc;
extern SCSIADDR		found_drives[MAX_SCSI_UNITS];
extern INQUIRYDATA	drive_info[MAX_SCSI_UNITS];
extern int			num_found_drives;
extern DWORD		scsi_err;
extern SCSIADDR		scsi_addr;		// current drive (set on fn_OpenDrive())
extern eReadMode	scsi_rmode;		// current read mode (set on fn_InitReadMode())
extern eSubQMode	scsi_subQmode;	// current subQ mode (set on fn_InitSubQMode())

// *_StartReadSector/*_WaitEndReadSector status data:
extern BOOL			pending_read_running;

// Drive status data / flags:
extern BOOL			drive_is_ready;	// current drive ready flag (used from drive_IsReady(), cleared when cache_FlushBuffers() called)
extern BOOL			plg_is_open;	// open_Driver() called (used from plugin functions only)
extern BOOL			play_in_progress;	// audio track play in progress (used from plugin functions only)

extern IF_LAYER_CMDS	if_layer;	// layer interface (use from if_* / scsi_* / cache_* files only)
extern IF_DRIVE_CMDS	if_drive;	// drive interface (use from plg_* / iso_* files only)

// Plugin specific functions (used from Config dialog):
extern void	(*fn_ReadCfg)(CFG_DATA *cfg);
extern void	(*fn_WriteCfg)(const CFG_DATA *cfg);


