#include <windows.h>
#include "winioctl.h"
#include "ntddcdrm.h"
// Plugin Defs
#include "Locals.h"

// Other SCSI commands (not included in "scsi.h")
#define	SCSIOP_NEC_READ_CDDA	0xD4
#define	SCSIOP_PLXTR_READ_CDDA	0xD8

//------------------------------------------------------------------------------
// SCSI CDB support functions:
//------------------------------------------------------------------------------

BYTE scsi_BuildInquiryCDB(CDB *cdb, int inquiry_len)
{
	cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
	//cdb->CDB6INQUIRY.LogicalUnitNumber = 0;	// RESERVED
	cdb->CDB6INQUIRY.AllocationLength = inquiry_len;
	return sizeof(cdb->CDB6INQUIRY);
}

BYTE scsi_BuildReadTocCDB(CDB *cdb, int toc_len)
{
	cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;
	cdb->READ_TOC.Msf = CDB_USE_MSF;
	cdb->READ_TOC.AllocationLength[0] = toc_len >> 8;
	cdb->READ_TOC.AllocationLength[1] = toc_len;
	return sizeof(cdb->READ_TOC);
}

BYTE scsi_BuildModeSenseCDB(CDB *cdb, int page_code, int sense_len, BOOL dbd, BOOL use6Byte)
{
	if (use6Byte) {
		if (sense_len > 255) sense_len = 255;	// max. 255 bytes
		cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
		cdb->MODE_SENSE.Dbd = dbd;	// TRUE = don't return any block descriptors
		cdb->MODE_SENSE.PageCode = page_code;
		cdb->MODE_SENSE.AllocationLength = sense_len;
		return sizeof(cdb->MODE_SENSE);
	} else {
		cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
		cdb->MODE_SENSE10.PageCode = page_code;
		cdb->MODE_SENSE10.Dbd = dbd;// TRUE = don't return any block descriptors
		cdb->MODE_SENSE10.AllocationLength[0] = (BYTE)(sense_len >> 8);
		cdb->MODE_SENSE10.AllocationLength[1] = (BYTE)sense_len;
		return sizeof(cdb->MODE_SENSE10);
	}
}

BYTE scsi_BuildModeSelectCDB(CDB *cdb, int sense_len, BOOL use6Byte)
{
	if (use6Byte) {
		cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
		cdb->MODE_SELECT.PFBit = 1;
		cdb->MODE_SELECT.ParameterListLength = (BYTE)sense_len;
		return sizeof(cdb->MODE_SELECT);
	} else {
		cdb->MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
		cdb->MODE_SELECT10.PFBit = 1;
		cdb->MODE_SELECT10.ParameterListLength[0] = (BYTE)(sense_len >> 8);
		cdb->MODE_SELECT10.ParameterListLength[1] = (BYTE)sense_len;
		return sizeof(cdb->MODE_SELECT10);
	}
}

BYTE scsi_BuildReadSectorCDB(CDB *cdb, int loc, int nsec, const SCSIADDR *addr, eReadMode rmode, eSubQMode subQmode)
{
	switch (rmode)
	{
	case ReadMode_Scsi_28:
		cdb->CDB10.OperationCode = SCSIOP_READ;
		cdb->CDB10.LogicalUnitNumber = addr->lun;
		//cdb->CDB10.LogicalBlockByte0 = loc >> 24;
		cdb->CDB10.LogicalBlockByte1 = loc >> 16;
		cdb->CDB10.LogicalBlockByte2 = loc >> 8;
		cdb->CDB10.LogicalBlockByte3 = loc;
		//cdb->CDB10.TransferBlocksMsb = nsec >> 8;
		cdb->CDB10.TransferBlocksLsb = nsec;
		return sizeof(cdb->CDB10);
	case ReadMode_Scsi_BE:
		cdb->READ_CD.OperationCode = SCSIOP_READ_CD;
		//cdb->READ_CD.ExpectedSectorType = CD_EXPECTED_SECTOR_ANY;
		//cdb->READ_CD.Lun = 0;					// RESERVED
		//cdb->READ_CD.StartingLBA[0] = loc >> 24;
		cdb->READ_CD.StartingLBA[1] = loc >> 16;
		cdb->READ_CD.StartingLBA[2] = loc >> 8;
		cdb->READ_CD.StartingLBA[3] = loc;
		//cdb->READ_CD.TransferBlocks[0] = nsec >> 16;
		//cdb->READ_CD.TransferBlocks[1] = nsec >> 8;
		cdb->READ_CD.TransferBlocks[2] = nsec;
		cdb->READ_CD.IncludeEDC = 1;
		cdb->READ_CD.IncludeUserData = 1;
		cdb->READ_CD.HeaderCode = 3;
		cdb->READ_CD.IncludeSyncData = 1;
		cdb->READ_CD.SubChannelSelection =	//0=none, 1=P-W, 2=Q, 4=R-W
			(subQmode == SubQMode_Read_Q) ? 2 :
			(subQmode == SubQMode_Read_PW) ? 1 : 0;
		return sizeof(cdb->READ_CD);
	case ReadMode_Scsi_D4:
		cdb->NEC_READ_CDDA.OperationCode = SCSIOP_NEC_READ_CDDA;
		//cdb->NEC_READ_CDDA.LogicalBlockByte0 = loc >> 24;
		cdb->NEC_READ_CDDA.LogicalBlockByte1 = loc >> 16;
		cdb->NEC_READ_CDDA.LogicalBlockByte2 = loc >> 8;
		cdb->NEC_READ_CDDA.LogicalBlockByte3 = loc;
		//cdb->NEC_READ_CDDA.TransferBlockByte0 = nsec >> 8;
		cdb->NEC_READ_CDDA.TransferBlockByte1 = nsec;
		return sizeof(cdb->NEC_READ_CDDA);
	case ReadMode_Scsi_D8:
		cdb->PLXTR_READ_CDDA.OperationCode = SCSIOP_PLXTR_READ_CDDA;
		cdb->PLXTR_READ_CDDA.LogicalUnitNumber = addr->lun;
		//cdb->PLXTR_READ_CDDA.LogicalBlockByte0 = loc >> 24;
		cdb->PLXTR_READ_CDDA.LogicalBlockByte1 = loc >> 16;
		cdb->PLXTR_READ_CDDA.LogicalBlockByte2 = loc >> 8;
		cdb->PLXTR_READ_CDDA.LogicalBlockByte3 = loc;
		//cdb->PLXTR_READ_CDDA.TransferBlockByte0 = 0;
		//cdb->PLXTR_READ_CDDA.TransferBlockByte1 = 0;
		//cdb->PLXTR_READ_CDDA.TransferBlockByte2 = nsec >> 8;
		cdb->PLXTR_READ_CDDA.TransferBlockByte3 = nsec;
		cdb->PLXTR_READ_CDDA.SubCode =		//0=none, 1=Q, 2=P-W, 3=P-W only (no sector data)
			(subQmode == SubQMode_Read_Q) ? 1 :
			(subQmode == SubQMode_Read_PW) ? 2 : 0;
		return sizeof(cdb->PLXTR_READ_CDDA);
	default:
		return 0;
	}
}

BYTE scsi_BuildPlayAudioMSFCDB(CDB *cdb, const MSF *start, const MSF *end)
{
	cdb->PLAY_AUDIO_MSF.OperationCode = SCSIOP_PLAY_AUDIO_MSF;
	//cdb->PLAY_AUDIO_MSF.LogicalUnitNumber = 0;	// RESERVED
	cdb->PLAY_AUDIO_MSF.StartingM = start->minute;
	cdb->PLAY_AUDIO_MSF.StartingS = start->second;
	cdb->PLAY_AUDIO_MSF.StartingF = start->frame;
	cdb->PLAY_AUDIO_MSF.EndingM = end->minute;
	cdb->PLAY_AUDIO_MSF.EndingS = end->second;
	cdb->PLAY_AUDIO_MSF.EndingF = end->frame;
	return sizeof(cdb->PLAY_AUDIO_MSF);
}

BYTE scsi_BuildStartStopCDB(CDB *cdb, BOOL start, BOOL load_eject, const SCSIADDR *addr)
{	// load_eject: 0 -> start/stop, 1 -> load/unload
	cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
	cdb->START_STOP.Immediate = 1;
	cdb->START_STOP.Start = start;
	cdb->START_STOP.LoadEject = load_eject;
	return sizeof(cdb->START_STOP);
}

BYTE scsi_BuildPauseResumeCDB(CDB *cdb, BYTE action, const SCSIADDR *addr)
{
	cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
	cdb->PAUSE_RESUME.LogicalUnitNumber = addr->lun;
	cdb->PAUSE_RESUME.Action = action;
	return sizeof(cdb->PAUSE_RESUME);
}

BYTE scsi_BuildTestUnitReadyCDB(CDB *cdb, const SCSIADDR *addr)
{
	cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
	cdb->CDB6GENERIC.LogicalUnitNumber = addr->lun;
	return sizeof(cdb->CDB6GENERIC);
}

BYTE scsi_BuildGetConfigurationCDB(CDB *cdb, BYTE type, int feature, int len)
{
    cdb->GET_CONFIGURATION.OperationCode = SCSIOP_GET_CONFIGURATION;
    cdb->GET_CONFIGURATION.RequestType = type;
    cdb->GET_CONFIGURATION.StartingFeature[0] = (BYTE)(feature >> 8);
    cdb->GET_CONFIGURATION.StartingFeature[1] = (BYTE)feature;
    cdb->GET_CONFIGURATION.AllocationLength[0] = (BYTE)(len >> 8);
    cdb->GET_CONFIGURATION.AllocationLength[1] = (BYTE)len;
	return sizeof(cdb->GET_CONFIGURATION);
}

BYTE scsi_BuildReadQChannelCDB(CDB *cdb, BYTE format/*, const SCSIADDR *addr*/)
{
	int		data_len = ((format == IOCTL_CDROM_CURRENT_POSITION) ? sizeof(SUB_Q_CURRENT_POSITION) :
		(format == IOCTL_CDROM_MEDIA_CATALOG) ? sizeof(SUB_Q_MEDIA_CATALOG_NUMBER) :
		sizeof(SUB_Q_TRACK_ISRC)) - sizeof(SUB_Q_HEADER);
	cdb->SUBCHANNEL.OperationCode = SCSIOP_READ_SUB_CHANNEL;
	cdb->SUBCHANNEL.Msf = CDB_USE_MSF;
	//cdb->SUBCHANNEL.LogicalUnitNumber = addr->lun;
	cdb->SUBCHANNEL.SubQ = CDB_SUBCHANNEL_BLOCK;
	cdb->SUBCHANNEL.Format = format;
	//cdb->SUBCHANNEL.TrackNumber = 0;
	//cdb->SUBCHANNEL.AllocationLength[0] = (BYTE)(data_len >> 8);
	cdb->SUBCHANNEL.AllocationLength[1] = (BYTE)data_len;
	return sizeof(cdb->SUBCHANNEL);
}

BYTE scsi_BuildSeekCDB(CDB *cdb, int loc, const SCSIADDR *addr)
{
	cdb->SEEK.OperationCode = SCSIOP_SEEK;
	cdb->SEEK.LogicalUnitNumber = addr->lun;
	//cdb->SEEK.LogicalBlockAddress[0] = loc >> 24;
	cdb->SEEK.LogicalBlockAddress[1] = loc >> 16;
	cdb->SEEK.LogicalBlockAddress[2] = loc >> 8;
	cdb->SEEK.LogicalBlockAddress[3] = loc;
	return sizeof(cdb->SEEK);
}

BYTE scsi_BuildSetCdSpeedCDB(CDB *cdb, int speed)
{
	if (speed != 0xFFFF)
		speed = (speed * 1764) / 10;	// 1x = (75*2352)/1000
	cdb->SET_CD_SPEED.OperationCode = SCSIOP_SET_CD_SPEED;
	cdb->SET_CD_SPEED.ReadSpeed[0] = speed >> 8;
	cdb->SET_CD_SPEED.ReadSpeed[1] = speed;
	cdb->SET_CD_SPEED.WriteSpeed[0] = 0xFF;
	cdb->SET_CD_SPEED.WriteSpeed[1] = 0xFF;
	return sizeof(cdb->SET_CD_SPEED);
}

//------------------------------------------------------------------------------
// SCSI other support functions:
//------------------------------------------------------------------------------

BOOL scsi_BuildModeSelectDATA(char *sense_buf, int *sense_len, int mode_block_size, void *mode_pages_data, int mode_pages_len, BOOL use6Byte)
{
	char	*p;

//	ASSERT(*sense_len > (sizeof(MODE_PARAMETER_HEADER10) + sizeof(MODE_PARAMETER_BLOCK) + mode_pages_len))
	memset(p = sense_buf, 0, *sense_len);

	if (mode_block_size) {
		if (use6Byte) {
			((MODE_PARAMETER_HEADER *)p)->BlockDescriptorLength = MODE_BLOCK_DESC_LENGTH;
		} else {
			//((MODE_PARAMETER_HEADER10 *)p)->BlockDescriptorLength[0] = (BYTE)(MODE_BLOCK_DESC_LENGTH >> 8);
			((MODE_PARAMETER_HEADER10 *)p)->BlockDescriptorLength[1] = (BYTE)MODE_BLOCK_DESC_LENGTH;
		}
	}
	p += ((use6Byte) ? sizeof(MODE_PARAMETER_HEADER) : sizeof(MODE_PARAMETER_HEADER10));
	if (mode_block_size) {
		//((MODE_PARAMETER_BLOCK *)p)->DensityCode = 0;
		//((MODE_PARAMETER_BLOCK *)p)->BlockLength[0] = (BYTE)(mode_block_size >> 16);
		((MODE_PARAMETER_BLOCK *)p)->BlockLength[1] = (BYTE)(mode_block_size >> 8);
		((MODE_PARAMETER_BLOCK *)p)->BlockLength[2] = (BYTE)mode_block_size;
		p += sizeof(MODE_PARAMETER_BLOCK);
	}
	if (mode_pages_len) {
		memcpy(p, mode_pages_data, mode_pages_len);
		p += mode_pages_len;
	}

	*sense_len = (int)(p - sense_buf);
	return TRUE;
}

BOOL scsi_ParseModeSenseBuffer(const char *sense_buf, const void *parameter_block_ary[], int *num_parameter_blocks, const void *mode_pages_ary[], int *num_mode_pages, BOOL use6Byte)
{
	int		len, data_len, block_len;
	const char	*p = sense_buf;
	int		max_parameter_blocks = (num_parameter_blocks) ? *num_parameter_blocks : 0;
	int		max_mode_pages = (num_mode_pages) ? *num_mode_pages : 0;

	if (num_parameter_blocks) *num_parameter_blocks = 0;
	if (num_mode_pages) *num_mode_pages = 0;

	if (use6Byte) {
		data_len = ((MODE_PARAMETER_HEADER *)p)->ModeDataLength + 1;
		block_len = ((MODE_PARAMETER_HEADER *)p)->BlockDescriptorLength;
		len = sizeof(MODE_PARAMETER_HEADER);
	} else {
		data_len = ((((MODE_PARAMETER_HEADER10 *)p)->ModeDataLength[0] << 8) | ((MODE_PARAMETER_HEADER10 *)p)->ModeDataLength[1]) + 2;
		block_len = (((MODE_PARAMETER_HEADER10 *)p)->BlockDescriptorLength[0] << 8) | ((MODE_PARAMETER_HEADER10 *)p)->BlockDescriptorLength[1];
		len = sizeof(MODE_PARAMETER_HEADER10);
	}
	p += len;
	data_len -= len;
	while (block_len > 0 && data_len > 0) {
		if (num_parameter_blocks && *num_parameter_blocks < max_parameter_blocks) {
			if (parameter_block_ary) *parameter_block_ary++ = (const void *)p;
			(*num_parameter_blocks)++;
		}
		p += sizeof(MODE_PARAMETER_BLOCK);
		block_len -= sizeof(MODE_PARAMETER_BLOCK);
		data_len -= sizeof(MODE_PARAMETER_BLOCK);
	}
	if (data_len < 0 || block_len != 0)
		return FALSE;
	while (data_len > 0) {
		len = ((MODE_READ_RECOVERY_PAGE *)p)->PageLength + 2;
		if (num_mode_pages && len <= data_len && *num_mode_pages < max_mode_pages) {
			if (mode_pages_ary) *mode_pages_ary++ = (const void *)p;
			(*num_mode_pages)++;
		}
		p += len;
		data_len -= len;
	}
	if (data_len < 0)
		return FALSE;
	return TRUE;
}

DWORD scsi_RemapSenseDataError(const SENSE_DATA *sense_area, BOOL *retry, DWORD *delay)
{
	DWORD	status = NO_ERROR;

	*retry = FALSE;						// default: do not retry command
	*delay = 0;
	switch (sense_area->SenseKey)
	{
	case SCSI_SENSE_RECOVERED_ERROR:	// Recovered Error
		//status = NO_ERROR;
		if (sense_area->IncorrectLength)
			status = ERROR_INVALID_BLOCK_LENGTH;
		break;
	case SCSI_SENSE_NOT_READY:			// Not Ready
		cache_FlushBuffers();			// media changed: empty cache buffers !!!
		status = ERROR_NOT_READY;
		*retry = TRUE;
		switch (sense_area->AdditionalSenseCode) {
		case SCSI_ADSENSE_LUN_NOT_READY:
			switch (sense_area->AdditionalSenseCodeQualifier) {
			case SCSI_SENSEQ_OPERATION_IN_PROGRESS:
			case SCSI_SENSEQ_BECOMING_READY:
				*delay = 10;
				break;
			case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:
			case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS:
			case SCSI_SENSEQ_FORMAT_IN_PROGRESS:
			case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE:
			case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
			default:
				*retry = FALSE;
				break;
			}
			break;
		case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE:
			*retry = FALSE;
			break;
		}
		break;
	case SCSI_SENSE_MEDIUM_ERROR:		// Medium Error
		status = ERROR_CRC;
		if (sense_area->AdditionalSenseCode == SCSI_ADSENSE_INVALID_MEDIA) {
			switch (sense_area->AdditionalSenseCodeQualifier) {
			case SCSI_SENSEQ_UNKNOWN_FORMAT:
			case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED:
				status = ERROR_UNRECOGNIZED_MEDIA;
				break;
			}
		}
		break;
	case SCSI_SENSE_ILLEGAL_REQUEST:	// Illegal Request
		status = ERROR_INVALID_FUNCTION;
		switch (sense_area->AdditionalSenseCode) {
		case SCSI_ADSENSE_ILLEGAL_BLOCK:
			status = ERROR_SECTOR_NOT_FOUND;
			break;
		case SCSI_ADSENSE_INVALID_LUN:
			status = ERROR_FILE_NOT_FOUND;
			break;
		case SCSI_ADSENSE_ILLEGAL_COMMAND:
		case SCSI_ADSENSE_MUSIC_AREA:
		case SCSI_ADSENSE_DATA_AREA:
		case SCSI_ADSENSE_VOLUME_OVERFLOW:
		case SCSI_ADSENSE_COPY_PROTECTION_FAILURE:
		case SCSI_ADSENSE_INVALID_CDB:
			break;
		}
		break;
	case SCSI_SENSE_UNIT_ATTENTION:		// Unit Attention
		cache_FlushBuffers();			// media changed: empty cache buffers !!!
		status = ERROR_MEDIA_CHANGED;//ERROR_IO_DEVICE
		*retry = TRUE;
		switch (sense_area->AdditionalSenseCode) {
		case SCSI_ADSENSE_MEDIUM_CHANGED:
		case SCSI_ADSENSE_BUS_RESET:
			break;
		case SCSI_ADSENSE_OPERATOR_REQUEST:
			switch (sense_area->AdditionalSenseCodeQualifier) {
			case SCSI_SENSEQ_MEDIUM_REMOVAL:
			case SCSI_SENSEQ_WRITE_PROTECT_ENABLE:
			case SCSI_SENSEQ_WRITE_PROTECT_DISABLE:
				break;
			}
			break;
		}
		break;
	case SCSI_SENSE_BLANK_CHECK:		// Blank Check
		status = ERROR_NO_DATA_DETECTED;
		break;
	case SCSI_SENSE_NO_SENSE:			// No Sense
		if (sense_area->IncorrectLength) {
			status = ERROR_INVALID_BLOCK_LENGTH;
		} else {
			status = ERROR_IO_DEVICE;
			*retry = TRUE;
		}
		break;
	case SCSI_SENSE_DATA_PROTECT:		// Data Protect
		status = ERROR_WRITE_PROTECT;
		break;
	case SCSI_SENSE_ABORTED_COMMAND:	// Aborted Command
		*delay = 1;
	case SCSI_SENSE_HARDWARE_ERROR:		// Hardware Error
	case SCSI_SENSE_UNIQUE:				// Vendor Specific
	case SCSI_SENSE_VOL_OVERFLOW:		// Volume Overflow
	case SCSI_SENSE_COPY_ABORTED:		// Copy Abort
	case SCSI_SENSE_EQUAL:				// Equal (Search)
	case SCSI_SENSE_MISCOMPARE:			// Miscompare (Search)
	case SCSI_SENSE_RESERVED:			// Reserved
	default:
		status = ERROR_IO_DEVICE;
		*retry = TRUE;
		break;
	}
	PRINTF(("scsi_RemapSenseDataError: ErrorCode=%02x SenseKey=%02x ASC=%02x ASCQ=%02x\n",
		sense_area->ErrorCode,
		sense_area->SenseKey,
		sense_area->AdditionalSenseCode,
		sense_area->AdditionalSenseCodeQualifier));
	return status;
}

