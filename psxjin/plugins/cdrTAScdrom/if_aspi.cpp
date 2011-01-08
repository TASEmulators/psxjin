// Plugin Defs
#include "Locals.h"
// ASPI interface
#include "wnaspi32.h"
#include "scsidefs.h"

// Warning: ALWAYS use the following 'union' to send ASPI commands: some ASPI
//	layers can write back more bytes than needed, and stack corruption occur
typedef union {
	SRB_HAInquiry		hainquiry;
	SRB_GDEVBlock		gdevblock;
	SRB_ExecSCSICmd		execscsicmd;
	SRB_Abort			abort;
	SRB_BusDeviceReset	busdevicereset;
	SRB_GetDiskInfo		getdiskinfo;
	SRB_RescanPort		rescanport;
	SRB_GetSetTimeouts	getsettimeouts;
} ASPI_SRB;

// ASPI interface data:
static DWORD	(*GetASPI32SupportInfo_fn)(void);
static DWORD	(*SendASPI32Command_fn)(ASPI_SRB *);
static DWORD	(*GetASPI32DLLVersion_fn)(void);
static HINSTANCE	wnaspi32_dll = NULL;
static HANDLE	aspi_wait_event = NULL;

// SRB & wait_event used only for async read (see aspi_StartReadSector/aspi_WaitEndReadSector):
static ASPI_SRB	read_srb;
static HANDLE	read_wait_event = NULL;
static DWORD	read_srbstatus;

//------------------------------------------------------------------------------
// ASPI support functions:
//------------------------------------------------------------------------------

static BOOL ASPI_CHECK_SENSE(const ASPI_SRB *srb, DWORD *scsi_err, BOOL *retry, DWORD *delay)
{
	DWORD	status = NO_ERROR;

	*retry = FALSE;						// default: do not retry command
	switch (srb->execscsicmd.SRB_Status)
	{
	case SS_COMP:								// SRB completed without error
		//status = NO_ERROR;
		break;
	case SS_ERR:								// SRB completed with error
		// check target status
		switch (srb->execscsicmd.SRB_TargStat)
		{
		case STATUS_CHKCOND:					// Check Condition
			// check sense area
			status = scsi_RemapSenseDataError((const SENSE_DATA *)srb->execscsicmd.SenseArea, retry, delay);
			break;
		case STATUS_GOOD:						// Status Good
			// check host adapter status
			switch (srb->execscsicmd.SRB_HaStat)
			{
			case HASTAT_SEL_TO:					// Selection Timeout
				status = ERROR_NOT_READY;
				break;
			case HASTAT_TIMEOUT:				// Timed out while SRB was waiting to be processed.
			case HASTAT_COMMAND_TIMEOUT:		// Adapter timed out processing SRB.
				status = ERROR_SEM_TIMEOUT;
				break;
			case HASTAT_OK:						// Host adapter did not detect an error
			case HASTAT_DO_DU:					// Data overrun data underrun
			case HASTAT_BUS_FREE:				// Unexpected bus free
			case HASTAT_PHASE_ERR:				// Target bus phase sequence failure
			case HASTAT_MESSAGE_REJECT:			// While processing SRB, the adapter received a MESSAGE
			case HASTAT_BUS_RESET:				// A bus reset was detected.
			case HASTAT_PARITY_ERROR:			// A parity error was detected.
			case HASTAT_REQUEST_SENSE_FAILED:	// The adapter failed in issuing
			default:
				status = ERROR_IO_DEVICE;
				break;
			}
			break;
		case STATUS_BUSY:						// Busy
			status = ERROR_NOT_READY;
			break;
		case STATUS_RESCONF:					// Reservation conflict
			status = ERROR_BUSY;
			break;
		default:
			status = ERROR_IO_DEVICE;
			break;
		}
		break;
	case SS_ABORTED:							// SRB aborted
		status = ERROR_SEM_TIMEOUT;
		*retry = TRUE;
		*delay = 1;
		break;
	case SS_INVALID_CMD:						// Invalid ASPI command
	case SS_INVALID_SRB:						// Invalid parameter set in SRB
		status = ERROR_INVALID_FUNCTION;
		break;
	case SS_INVALID_HA:							// Invalid host adapter number
	case SS_NO_DEVICE:							// SCSI device not installed
	case SS_NO_ADAPTERS:						// No host adapters to manage
		status = ERROR_FILE_NOT_FOUND;
		break;
	case SS_ABORT_FAIL:							// Unable to abort SRB
	case SS_BUFFER_ALIGN:						// Buffer not aligned (replaces OLD_MANAGER in Win32)
	case SS_ILLEGAL_MODE:						// Unsupported Windows mode
	case SS_NO_ASPI:							// No ASPI managers resident
	case SS_FAILED_INIT:						// ASPI for windows failed init
	case SS_ASPI_IS_BUSY:						// No resources available to execute cmd
	case SS_BUFFER_TO_BIG:						// Buffer size to big to handle!
	case SS_MISMATCHED_COMPONENTS:				// The DLLs/EXEs of ASPI don't version check
	default:
		status = ERROR_IO_DEVICE;
		break;
	}
	*scsi_err = status;
	if (status == NO_ERROR)
		return TRUE;
	PRINTF(("ASPI_CHECK_SENSE: winerr=%d retry=%d delay=%d\n",
		status, *retry, (*retry) ? *delay : 0));
	return FALSE;
}

//------------------------------------------------------------------------------
// ASPI interface functions:
//------------------------------------------------------------------------------

BOOL aspi_LoadLayer(void)
{
	if (wnaspi32_dll != NULL)
		return TRUE;

	if ((wnaspi32_dll = LoadLibrary("WNASPI32.DLL")) == NULL)
		return FALSE;
	GetASPI32SupportInfo_fn = (DWORD (*)(void))GetProcAddress(wnaspi32_dll, "GetASPI32SupportInfo");
	SendASPI32Command_fn = (DWORD (*)(ASPI_SRB *))GetProcAddress(wnaspi32_dll, "SendASPI32Command");
	GetASPI32DLLVersion_fn = (DWORD (*)(void))GetProcAddress(wnaspi32_dll, "GetASPI32DLLVersion");
	aspi_wait_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	read_wait_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (GetASPI32SupportInfo_fn != NULL && SendASPI32Command_fn != NULL &&
		GetASPI32DLLVersion_fn != NULL && aspi_wait_event != NULL &&
		read_wait_event != NULL &&
		((GetASPI32SupportInfo_fn() >> 8) & 0xff) == SS_COMP)
		return TRUE;
	aspi_UnloadLayer();
	return FALSE;
}

void aspi_UnloadLayer(void)
{
	if (wnaspi32_dll == NULL)
		return;
	// patch x Win9x default WNASPI32.DLL bug (access violation on FreeLibrary())
	if (GetASPI32DLLVersion_fn() != 0x00000001)
		FreeLibrary(wnaspi32_dll);
	wnaspi32_dll = NULL;
	GetASPI32SupportInfo_fn = NULL;
	SendASPI32Command_fn = NULL;
	GetASPI32DLLVersion_fn = NULL;
	if (aspi_wait_event) CloseHandle(aspi_wait_event);
	if (read_wait_event) CloseHandle(read_wait_event);
	aspi_wait_event = NULL;
	read_wait_event = NULL;
}

BOOL aspi_OpenDrive(const SCSIADDR *addr)
{
	ASPI_SRB	srb;

	// save current SCSI address
	scsi_addr = *addr;

	// setup SCSI timeout for the units to 10 seconds
	memset(&srb, 0, sizeof(srb));
	srb.getsettimeouts.SRB_Cmd = SC_GETSET_TIMEOUTS;
	srb.getsettimeouts.SRB_HaId = addr->haid;
	srb.getsettimeouts.SRB_Target = addr->target;
	srb.getsettimeouts.SRB_Lun = addr->lun;
	srb.getsettimeouts.SRB_Flags = SRB_DIR_OUT;		// set timeout
	srb.getsettimeouts.SRB_Timeout = SCSI_CDROM_TIMEOUT * 2;	// timeout value in 1/2 second units
	if (SendASPI32Command_fn(&srb) != SS_COMP ||
		srb.getsettimeouts.SRB_Status != SS_COMP) {
		// Ignore error code: SC_GETSET_TIMEOUT doesn't work for some ASPI layer !!!
		PRINTF(("ASPI SC_GETSET_TIMEOUTS command returned with error %x\n", srb.getsettimeouts.SRB_Status));
	}
	return TRUE;
}

BOOL aspi_QueryAvailDrives(void)
{
    DWORD	dwSupportInfo;
	BYTE	num_adapters, num_targets, drv_type;
	SCSIADDR	addr;

	// reset any previous drive info
	memset(found_drives, 0, sizeof(found_drives));
	memset(&drive_info, 0, sizeof(drive_info));
	num_found_drives = 0;

	dwSupportInfo = GetASPI32SupportInfo_fn();
	switch ((dwSupportInfo >> 8) & 0xff) {
	case SS_COMP:			break;
	case SS_NO_ADAPTERS:	return TRUE;
	default:				return FALSE;
	}
	num_adapters = (BYTE)(dwSupportInfo & 0xff);
	for (addr.haid = 0; addr.haid < num_adapters; addr.haid++)	// 0 ... MAX_NUM_HA
	{
		ASPI_SRB	srb;
		INQUIRYDATA	inquiry;
		memset(&srb, 0, sizeof(srb));
		srb.hainquiry.SRB_Cmd = SC_HA_INQUIRY;
		srb.hainquiry.SRB_HaId = addr.haid;
		if (SendASPI32Command_fn(&srb) != SS_COMP || srb.hainquiry.SRB_Status != SS_COMP)
			continue;
		// srb.hainquiry.HA_Unique[2] = residual count supported (0x01) / unsupported (0x00)
		if ((num_targets = srb.hainquiry.HA_Unique[3]) != 8 && num_targets != 16)
			num_targets = 8;
		for (addr.target = 0; addr.target < num_targets; addr.target++)	// 0 ... MAXTARG
		{
			for (addr.lun = 0; addr.lun < 8; addr.lun++)		// 0 ... MAXLUN
			{
				if (!aspi_GetDriveType(&drv_type, &addr) ||
					drv_type != DTYPE_CDROM)
					continue;
				if_layer.OpenDrive(&addr);
				memset(&inquiry, 0, sizeof(inquiry));
				// try to read 'addr.drive_letter' field (if supported from ASPI layer)
				aspi_GetDriveInfo();	// (don't check error code, it always fails under WinNT)
				if (scsi_Inquiry(&inquiry))
				{
					PRINTF(("HaId=%d Target=%d Lun=%d: VendorId=%.8s ProductId=%.16s ProductRevisionLevel=%.4s\n",
						addr.haid, addr.target, addr.lun, inquiry.VendorId, inquiry.ProductId, inquiry.ProductRevisionLevel));
					if (num_found_drives < MAX_SCSI_UNITS) {
						memcpy(&drive_info[num_found_drives], &inquiry, sizeof(inquiry));
						found_drives[num_found_drives] = scsi_addr;	// Note: use 'scsi_addr' value ('addr' drive_letter field is not set, see *_GetDriveInfo())
						num_found_drives++;
					}
				}
				if_layer.CloseDrive();
			}
		}
	}
	return TRUE;
}

BOOL aspi_GetDriveType(BYTE *drv_type, const SCSIADDR *addr)
{
	ASPI_SRB	srb;

	memset(&srb, 0, sizeof(srb));
	srb.gdevblock.SRB_Cmd = SC_GET_DEV_TYPE;
	srb.gdevblock.SRB_HaId = addr->haid;
	srb.gdevblock.SRB_Target = addr->target;
	srb.gdevblock.SRB_Lun = addr->lun;
	if (SendASPI32Command_fn(&srb) != SS_COMP ||
		srb.gdevblock.SRB_Status != SS_COMP)
		return FALSE;
	*drv_type = srb.gdevblock.SRB_DeviceType;
	return TRUE;
}

BOOL aspi_GetDriveInfo(void)
{
	ASPI_SRB	srb;

// Warning: this functions modify the 'scsi_addr' value previously set on *_OpenDrive()

	scsi_addr.drive_letter = 0;

	// This function works fine for Win9x ONLY. WinNT and some ASPI layer
	// implementations fails always with SRB_Status == SS_INVALID_CMD !!!
	memset(&srb, 0, sizeof(srb));
	srb.getdiskinfo.SRB_Cmd = SC_GET_DISK_INFO;
	srb.getdiskinfo.SRB_HaId = scsi_addr.haid;
	srb.getdiskinfo.SRB_Target = scsi_addr.target;
	srb.getdiskinfo.SRB_Lun = scsi_addr.lun;
	if (SendASPI32Command_fn(&srb) != SS_COMP ||
		srb.getdiskinfo.SRB_Status != SS_COMP)
		return FALSE;
	switch (srb.getdiskinfo.SRB_DriveFlags & 0x03)
	{
	case 0x01:	//DISK_INT13_AND_DOS
	case 0x02:	//DISK_INT13
		break;
	case 0x00:	//DISK_NOT_INT13
		// Adaptec documentation say that, in this case, SRB_Int13HDriveInfo field is to be
		// considered as invalid, but it seems to be ok: so, i can try to use it anyway...
		if (srb.getdiskinfo.SRB_Int13HDriveInfo != 0)
			break;
	default:
		return FALSE;
	}
	scsi_addr.drive_letter = srb.getdiskinfo.SRB_Int13HDriveInfo + 'A';
	return TRUE;
}

BOOL aspi_StartReadSector(BYTE *rbuf, int loc, int nsec)
{
	memset(&read_srb, 0, sizeof(read_srb));
	read_srb.execscsicmd.SRB_Cmd = SC_EXEC_SCSI_CMD;
	read_srb.execscsicmd.SRB_HaId = scsi_addr.haid;
	read_srb.execscsicmd.SRB_Target = scsi_addr.target;
	read_srb.execscsicmd.SRB_Lun = scsi_addr.lun;
	read_srb.execscsicmd.SRB_Flags = SRB_DIR_IN | SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
	read_srb.execscsicmd.SRB_BufLen = nsec * RAW_SECTOR_SIZE;
	read_srb.execscsicmd.SRB_BufPointer = rbuf;
	read_srb.execscsicmd.SRB_SenseLen = SENSE_LEN;
	read_srb.execscsicmd.SRB_CDBLen = scsi_BuildReadSectorCDB((CDB *)read_srb.execscsicmd.CDBByte, loc, nsec, &scsi_addr, scsi_rmode, SubQMode_Disabled);
	read_srb.execscsicmd.SRB_PostProc = read_wait_event;
	ResetEvent(read_wait_event);
	// save the returned read status value for the *_WaitEndReadSector() call
	read_srbstatus = SendASPI32Command_fn(&read_srb);
	// Warning: the flag 'pending_read_running' indicate if the read operation is done ...
	// ... from the *_StartReadSector() (FALSE) or from the *_WaitEndReadSector() (TRUE)
	pending_read_running = TRUE;		// read operation not yet finished, need to call *_WaitEndReadSector()
	return TRUE;
}

BOOL aspi_WaitEndReadSector(void)
{
	DWORD	delay;
	BOOL	retry;

	if (!pending_read_running)
		return FALSE;
	pending_read_running = FALSE;
	if (read_srbstatus == SS_PENDING)
		WaitForSingleObject(read_wait_event, INFINITE);
	return ASPI_CHECK_SENSE(&read_srb, &scsi_err, &retry, &delay);
}

BOOL aspi_ExecScsiCmd(void *buf, DWORD buflen, const CDB *cdb, BYTE cdblen, int data_dir, int retries)
{
	ASPI_SRB	srb;
	DWORD	delay;
	BOOL	retry;

	do {
		memset(&srb, 0, sizeof(srb));
		srb.execscsicmd.SRB_Cmd = SC_EXEC_SCSI_CMD;
		srb.execscsicmd.SRB_HaId = scsi_addr.haid;
		srb.execscsicmd.SRB_Target = scsi_addr.target;
		srb.execscsicmd.SRB_Lun = scsi_addr.lun;
		srb.execscsicmd.SRB_Flags = ((data_dir == SCSI_IOCTL_DATA_IN) ? SRB_DIR_IN : (data_dir == SCSI_IOCTL_DATA_OUT) ? SRB_DIR_OUT : SRB_DIR_SCSI) | SRB_EVENT_NOTIFY;
		srb.execscsicmd.SRB_BufLen = buflen;
		srb.execscsicmd.SRB_BufPointer = (BYTE *)buf;
		srb.execscsicmd.SRB_SenseLen = SENSE_LEN;
		memcpy(srb.execscsicmd.CDBByte, cdb, srb.execscsicmd.SRB_CDBLen = cdblen);
		srb.execscsicmd.SRB_PostProc = aspi_wait_event;
		ResetEvent(aspi_wait_event);
		if (SendASPI32Command_fn(&srb) == SS_PENDING)
			WaitForSingleObject(aspi_wait_event, INFINITE);
		if (ASPI_CHECK_SENSE(&srb, &scsi_err, &retry, &delay))
			return TRUE;
		if (!retry) break;
		if (delay && retries) Sleep(delay * 100);
	} while (retries-- > 0);
	return FALSE;
}

