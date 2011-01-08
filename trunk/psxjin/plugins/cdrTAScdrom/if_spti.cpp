#include <windows.h>
// SPTI/RAW interface
#include "winioctl.h"
#include "ntddscsi.h"
#include "ntddcdrm.h"
// Plugin Defs
#include "Locals.h"

typedef struct {
	SCSI_PASS_THROUGH_DIRECT	sptd;
	ULONG	Filler;		// realign buffer to double word boundary
	UCHAR	ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;


// SPTI/RAW interface data:
static HANDLE	hCD = INVALID_HANDLE_VALUE;

//------------------------------------------------------------------------------
// SPTI/RAW error handling (WinNT/2K):
//------------------------------------------------------------------------------

static BOOL NT_SPTI_CHECK_SENSE(const SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER *srb, DWORD *scsi_err, BOOL *retry, DWORD *delay)
{
	DWORD	status = NO_ERROR;

	*retry = FALSE;						// default: do not retry command
	if (srb->sptd.ScsiStatus != 0) {	// status error
		if (srb->sptd.SenseInfoLength != 0)	// check sense area
			status = scsi_RemapSenseDataError((const SENSE_DATA *)srb->ucSenseBuf, retry, delay);
		else							// no sense data available
			status = ERROR_IO_DEVICE;
	} else {							// status ok
		//status = NO_ERROR;
	}
	*scsi_err = status;
	if (status == NO_ERROR)
		return TRUE;
	PRINTF(("NT_SPTI_CHECK_SENSE: winerr=%d retry=%d delay=%d\n",
		status, *retry, (*retry) ? *delay : 0));
	return FALSE;
}

static BOOL NT_CHECK_ERROR_CODE(DWORD *scsi_err)
{
	DWORD	status;

	status = GetLastError();
	*scsi_err = status;
	if (status == ERROR_NOT_READY || status == ERROR_MEDIA_CHANGED)
		cache_FlushBuffers();			// media changed: empty cache buffers !!!
	PRINTF(("NT_CHECK_STATUS: winerr=%d\n", status));
	return FALSE;						// always return error (FALSE) !!!
}

#define	RESET_ERROR_CODE(scsi_err)	{ scsi_err = NO_ERROR; }

//------------------------------------------------------------------------------
// SPTI/RAW interface functions (WinNT/2K):
//------------------------------------------------------------------------------

BOOL nt_LoadLayer(void)
{
	// nothing really to initialize: check only if O.S. is valid
	get_OsVersion();
	if (os_version.dwPlatformId != VER_PLATFORM_WIN32_NT)
		return FALSE;
	return TRUE;
}

BOOL nt_OpenDrive(const SCSIADDR *addr)
{
	int			open_mode, err;
	char		buf[50];

	// save current SCSI address
	scsi_addr = *addr;

	get_OsVersion();
	if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT && os_version.dwMajorVersion >= 5)
		open_mode = GENERIC_READ | GENERIC_WRITE;
	else
		open_mode = GENERIC_READ;
	sprintf(buf, "\\\\.\\%c:", addr->drive_letter);
	if ((hCD = CreateFile(buf, open_mode, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL)) == INVALID_HANDLE_VALUE)
	{
		err = GetLastError();
		open_mode ^= GENERIC_WRITE;		// ???
		if ((hCD = CreateFile(buf, open_mode, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL)) == INVALID_HANDLE_VALUE)
			return FALSE;
	}
	return TRUE;
}

void nt_CloseDrive(void)
{
	if (hCD != INVALID_HANDLE_VALUE)
		CloseHandle(hCD);
	hCD = INVALID_HANDLE_VALUE;
}

BOOL nt_QueryAvailDrives(void)
{
	SCSIADDR	addr;
	BYTE	drv_type;
	int		old_error_mode;
	int		drive_open_errors = 0;

	// reset any previous drive info
	memset(found_drives, 0, sizeof(found_drives));
	memset(&drive_info, 0, sizeof(drive_info));
	num_found_drives = 0;

	// override default error mode to avoid 'drive not ready' messageboxes during open_drive()
	old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
	for (addr.drive_letter = 'A'; addr.drive_letter <= 'Z'; addr.drive_letter++)	// 'C' - 'Z'
	{
		INQUIRYDATA	inquiry;
		if (!nt_GetDriveType(&drv_type, &addr) ||
			drv_type != DRIVE_CDROM)
			continue;
		if (!if_layer.OpenDrive(&addr)) {
			drive_open_errors++;	// increase num. open drive errors
			continue;
		}
		memset(&inquiry, 0, sizeof(inquiry));
		if (nt_GetDriveInfo() &&	// read 'scsi_addr.haid/target/lun' fields
			nt_Inquiry(&inquiry))
		{
			PRINTF(("HaId=%d Target=%d Lun=%d: VendorId=%.8s ProductId=%.16s ProductRevisionLevel=%.4s\n",
				scsi_addr.haid, scsi_addr.target, scsi_addr.lun, inquiry.VendorId, inquiry.ProductId, inquiry.ProductRevisionLevel));
			if (num_found_drives < MAX_SCSI_UNITS) {
				memcpy(&drive_info[num_found_drives], &inquiry, sizeof(inquiry));
				found_drives[num_found_drives] = scsi_addr;	// Note: use 'scsi_addr' value ('addr' haid/target/lun fields are not set, see *_GetDriveInfo())
				num_found_drives++;
			}
		}
		if_layer.CloseDrive();
	}
	// restore default error mode
	SetErrorMode(old_error_mode);
	if (num_found_drives == 0 &&	// if no drive info available, but at least one ...
		drive_open_errors > 0)		// ... cdrom unit present, ret with error !!!
		return FALSE;				// (no WinNT/2K or user without Admin privileges ?)
	return TRUE;
}

BOOL nt_GetDriveType(BYTE *drv_type, const SCSIADDR *addr)
{
	int		res;
	char	buf[4];

	sprintf(buf, "%c:\\", addr->drive_letter);
	if ((res = GetDriveType(buf)) == DRIVE_NO_ROOT_DIR ||
		res == DRIVE_UNKNOWN)
		return FALSE;
	*drv_type = res;
	return TRUE;
}

BOOL nt_GetDriveInfo(void)
{
	SCSI_ADDRESS	sa;
	DWORD	dwBytesReturned;

// Warning: this functions modify the 'scsi_addr' value previously set on *_OpenDrive()

	memset(&sa, 0, sizeof(sa));
	sa.Length = sizeof(sa);
	if (!DeviceIoControl(hCD, IOCTL_SCSI_GET_ADDRESS, NULL, 0, &sa, sizeof(sa), &dwBytesReturned, NULL))
		return FALSE;
	scsi_addr.haid = /*sa.PathId*/sa.PortNumber;
	scsi_addr.target = sa.TargetId;
	scsi_addr.lun = sa.Lun;
	return TRUE;
}

BOOL nt_Inquiry(INQUIRYDATA *inquiry)
{
	int		i;
	UCHAR	DataBuffer[2048];
	DWORD	dwBytesReturned;
	SCSI_ADAPTER_BUS_INFO	*adapterInfo;
	SCSI_INQUIRY_DATA		*inquiryData;

	if (!DeviceIoControl(hCD, IOCTL_SCSI_GET_INQUIRY_DATA, NULL, 0, DataBuffer, sizeof(DataBuffer), &dwBytesReturned, NULL))
		return FALSE;
	adapterInfo = (SCSI_ADAPTER_BUS_INFO *)DataBuffer;
	for (i = 0; i < adapterInfo->NumberOfBuses; i++)
	{
		if (adapterInfo->BusData[i].InquiryDataOffset == 0)
			continue;
		inquiryData = (SCSI_INQUIRY_DATA *) (DataBuffer +
			adapterInfo->BusData[i].InquiryDataOffset);
		for (;;)
		{
			if (inquiryData->TargetId == scsi_addr.target &&
				inquiryData->Lun == scsi_addr.lun)
			{
				memcpy(inquiry, inquiryData->InquiryData,
					(inquiryData->InquiryDataLength <= sizeof(*inquiry)) ? inquiryData->InquiryDataLength : sizeof(*inquiry));
				return TRUE;
			}
			if (inquiryData->NextInquiryDataOffset == 0)
				break;
			inquiryData = (PSCSI_INQUIRY_DATA) (DataBuffer +
				inquiryData->NextInquiryDataOffset);
		}
	}
	return FALSE;
}

BOOL nt_ReadToc(void)
{
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl (hCD, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, CDROM_TOC_SIZE, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	return TRUE;
}

BOOL nt_spti_StartReadSector(BYTE *rbuf, int loc, int nsec)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildReadSectorCDB(&cdb, loc, nsec, &scsi_addr, scsi_rmode, SubQMode_Disabled);
	if (!if_layer.ExecScsiCmd(rbuf, nsec * RAW_SECTOR_SIZE, &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2))
		return FALSE;
	// Warning: the flag 'pending_read_running' indicate if the read operation is done ...
	// ... from the *_StartReadSector() (FALSE) or from the *_WaitEndReadSector() (TRUE)
	pending_read_running = FALSE;		// read operation already finished, no *_WaitEndReadSector() needed
	return TRUE;
}

BOOL nt_raw_StartReadSector(BYTE *rbuf, int loc, int nsec)
{
	RAW_READ_INFO	ri;
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
	ri.DiskOffset.HighPart = 0;
	ri.DiskOffset.LowPart = loc * COOKED_SECTOR_SIZE;
	ri.SectorCount = nsec;
	// RAW_READ can read only sectors of the type specified in 'TrackMode':
	//	CD-DA			-> CDDA			(audio)
	//	MODE1 			-> n/a			(2048 bytes of user data)
	//	MODE2 FORMLESS	-> YellowMode2	(2336 bytes of user data)
	//	MODE2 FORM1		-> n/a			(8 bytes of sub-header, 2048 bytes of user data)
	//	MODE2 FORM2		-> XAForm2		(8 bytes of sub-header, 2324 bytes of user data)
	// For the PSX cd's who can use only MODE2 FORM2, the RAW_READ is ok, but for
	//	the other ones who can use both MODE2 FORM2 (for movies & sounds) and MODE2 FORM1,
	//	(for normal data), the RAW_READ command doesn't work (ret ERROR_INVALID_FUNCTION) !!!
	ri.TrackMode = XAForm2;
	if (!DeviceIoControl(hCD, IOCTL_CDROM_RAW_READ, &ri, sizeof(ri), rbuf, nsec * RAW_SECTOR_SIZE, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	// Warning: the flag 'pending_read_running' indicate if the read operation is done ...
	// ... from the *_StartReadSector() (FALSE) or from the *_WaitEndReadSector() (TRUE)
	pending_read_running = FALSE;		// read operation already finished, no *_WaitEndReadSector() needed
	return TRUE;
}

BOOL nt_PlayAudioTrack(const MSF *start, const MSF *end)
{
	CDROM_PLAY_AUDIO_MSF	pa;
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
	pa.StartingM = start->minute;
	pa.StartingS = start->second;
	pa.StartingF = start->frame;
	pa.EndingM = end->minute;
	pa.EndingS = end->second;
	pa.EndingF = end->frame;
    if (!DeviceIoControl(hCD, IOCTL_CDROM_PLAY_AUDIO_MSF, &pa, sizeof(pa), NULL, 0, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	return TRUE;
}

BOOL nt_StopAudioTrack(void)
{
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl(hCD, IOCTL_CDROM_STOP_AUDIO, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	return TRUE;
}

BOOL nt_TestUnitReady(BOOL do_retry)
{
	ULONG	MediaChangeCount;
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl(hCD, IOCTL_CDROM_CHECK_VERIFY, NULL, 0, &MediaChangeCount, sizeof(MediaChangeCount), &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);	// ERROR_NOT_READY or ERROR_MEDIA_CHANGED
	return TRUE;
}

BOOL nt_GetPlayLocation(MSF *play_loc, BYTE *audio_status)
{
	CDROM_SUB_Q_DATA_FORMAT	rq;
	SUB_Q_CHANNEL_DATA		data;
	DWORD	dwBytesReturned;

	rq.Format = IOCTL_CDROM_CURRENT_POSITION;
	rq.Track = 0;

	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl(hCD, IOCTL_CDROM_READ_Q_CHANNEL, &rq, sizeof(rq), &data, sizeof(data), &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	play_loc->minute = data.CurrentPosition.AbsoluteAddress[1];
	play_loc->second = data.CurrentPosition.AbsoluteAddress[2];
	play_loc->frame = data.CurrentPosition.AbsoluteAddress[3];
	// audio_status: AUDIO_STATUS_IN_PROGRESS=play, AUDIO_STATUS_PAUSED=paused, AUDIO_STATUS_NO_STATUS/other=no play
	*audio_status = data.CurrentPosition.Header.AudioStatus;
	return TRUE;
}

BOOL nt_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc)
{
	CDROM_SEEK_AUDIO_MSF	sa;
	CDROM_SUB_Q_DATA_FORMAT	rq;
	SUB_Q_CHANNEL_DATA		data;
	DWORD	dwBytesReturned;
	MSF		msf;

	INT2MSF(&msf, loc);
	sa.M = msf.minute;
	sa.S = msf.second;
	sa.F = msf.frame;
	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl(hCD, IOCTL_CDROM_SEEK_AUDIO_MSF, &sa, sizeof(sa), NULL, 0, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	rq.Format = IOCTL_CDROM_CURRENT_POSITION;
	rq.Track = 0;
    if (!DeviceIoControl(hCD, IOCTL_CDROM_READ_Q_CHANNEL, &rq, sizeof(rq), &data, sizeof(data), &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	rbuf->ADR = data.CurrentPosition.ADR;
	rbuf->Control = data.CurrentPosition.Control;
	rbuf->TrackNumber = INT2BCD(data.CurrentPosition.TrackNumber);
	rbuf->IndexNumber = INT2BCD(data.CurrentPosition.IndexNumber);
	rbuf->TrackRelativeAddress[0] = INT2BCD(data.CurrentPosition.TrackRelativeAddress[1]);
	rbuf->TrackRelativeAddress[1] = INT2BCD(data.CurrentPosition.TrackRelativeAddress[2]);
	rbuf->TrackRelativeAddress[2] = INT2BCD(data.CurrentPosition.TrackRelativeAddress[3]);
	//rbuf->Filler = 0;
	rbuf->AbsoluteAddress[0] = INT2BCD(data.CurrentPosition.AbsoluteAddress[1]);
	rbuf->AbsoluteAddress[1] = INT2BCD(data.CurrentPosition.AbsoluteAddress[2]);
	rbuf->AbsoluteAddress[2] = INT2BCD(data.CurrentPosition.AbsoluteAddress[3]);
	//rbuf->Crc[0] = 0;
	//rbuf->Crc[1] = 0;
	return FALSE;
}

BOOL nt_LoadEjectUnit(BOOL load)
{
	DWORD	dwBytesReturned;

	RESET_ERROR_CODE(scsi_err);
    if (!DeviceIoControl(hCD, (load) ? IOCTL_STORAGE_LOAD_MEDIA : IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
		return NT_CHECK_ERROR_CODE(&scsi_err);
	return TRUE;
}

BOOL nt_spti_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries)
{
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER	sb;
	DWORD	dwBytesReturned;
	DWORD	delay;
	BOOL	retry;

	do {
		RESET_ERROR_CODE(scsi_err);
		memset(&sb, 0, sizeof(sb));
		sb.sptd.Length = sizeof(sb.sptd);
		sb.sptd.PathId = scsi_addr.haid;
		sb.sptd.TargetId = scsi_addr.target;
		sb.sptd.Lun = scsi_addr.lun;
		sb.sptd.DataIn = data_dir; // SCSI_IOCTL_DATA_IN / SCSI_IOCTL_DATA_OUT / SCSI_IOCTL_DATA_UNSPECIFIED
		sb.sptd.DataTransferLength = buflen;
		sb.sptd.DataBuffer = buf;
		memcpy(sb.sptd.Cdb, cdb, sb.sptd.CdbLength = cdblen);
		sb.sptd.TimeOutValue = SCSI_CDROM_TIMEOUT;
		sb.sptd.SenseInfoLength = SENSE_BUFFER_SIZE;
		sb.sptd.SenseInfoOffset = (int)((char *)&sb.ucSenseBuf - (char *)&sb);
		if (!DeviceIoControl(hCD, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sb, sizeof(sb), &sb, sizeof(sb), &dwBytesReturned, NULL))
			return NT_CHECK_ERROR_CODE(&scsi_err);
		if (NT_SPTI_CHECK_SENSE(&sb, &scsi_err, &retry, &delay))
			return TRUE;
		if (!retry) break;
		if (delay) Sleep(delay * 100);
	} while (retries-- > 0);
	return FALSE;
}

