#include <windows.h>
#include "winioctl.h"
#include "ntddcdrm.h"
#include "ntddmmc.h"
// Plugin Defs
#include "Locals.h"

static BOOL	scsi_SelectReadMode(eReadMode rmode, eSubQMode subQmode);
static int prev_spindown_timer = -1;

//------------------------------------------------------------------------------
// SCSI common interface functions:
//------------------------------------------------------------------------------

BOOL scsi_Inquiry(INQUIRYDATA *inquiry)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildInquiryCDB(&cdb, sizeof(*inquiry));
	return if_layer.ExecScsiCmd(inquiry, sizeof(*inquiry), &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2);
}

BOOL scsi_ModeSense(int page_code, char *sense_buf, int sense_len, BOOL dbd, BOOL use6Byte)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildModeSenseCDB(&cdb, page_code, sense_len, dbd, use6Byte);
	return if_layer.ExecScsiCmd(sense_buf, sense_len, &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2);
}

BOOL scsi_DetectReadMode(eReadMode *rmode)
{
	CDB		cdb;
	BYTE	cdblen;
	BOOL	use6Byte;
	char	sense_buf[1024];
	const void	*mode_pages_ary[256];
	int		num_mode_pages, cnt, i;
	BOOL	use_read_cd = FALSE;
//	BOOL	use_nec_read_cdda = FALSE;
//	BOOL	use_plxtr_read_cdda = FALSE;
//	const INQUIRYDATA *inquiry;
// - ricercare idx su found_drives[] di *addr tramite SCSIADDR_EQ(*addr, found_drives[idx])
// - leggere inquiry da drive_info[idx]


	// The purpose of this function is to identify the drive capabilities.
	// (mainly, to check if the drive supports the READ_CD command)
	// NOTE: The identification sequence is 'very similar' to the one
	// 		 used from the CDROM driver sample of the 'Win2K DDK'.

	// check inquiry.VendorId[] == "NEC" / "PLEXTOR "
//	if (!strncmp(inquiry->VendorId, "NEC", 3))
//		capab->use_nec_read_cdda = 1;	// use NEC specific command to read CDDA
//	if (!strncmp(inquiry->VendorId, "PLEXTOR ", 8))
//		capab->use_plxtr_read_cdda = 1;	// use PLEXTOR specific command to read CDDA

	// check if the drive is a DVD
	for (cnt = 0; cnt < 2; cnt++) {
		use6Byte = (cnt == 0) ? TRUE : FALSE;
		memset(&sense_buf, 0, sizeof(sense_buf));
		if (!scsi_ModeSense(MODE_PAGE_CAPABILITIES, sense_buf, sizeof(sense_buf), TRUE, use6Byte))
			continue;
		num_mode_pages = sizeof(mode_pages_ary) / sizeof(mode_pages_ary[0]);
		scsi_ParseModeSenseBuffer(sense_buf, NULL, NULL, mode_pages_ary, &num_mode_pages, use6Byte);
		for (i = 0; i < num_mode_pages; i++) {
			CDVD_CAPABILITIES_PAGE *capPage = (CDVD_CAPABILITIES_PAGE *)mode_pages_ary[i];
			if (capPage->PageCode == MODE_PAGE_CAPABILITIES &&
				capPage->PageLength == (sizeof(CDVD_CAPABILITIES_PAGE)-2) &&
				(capPage->DVDROMRead || capPage->DVDRRead ||
				 capPage->DVDRAMRead || capPage->DVDRWrite ||
				 capPage->DVDRAMWrite)) {
				*rmode = ReadMode_Scsi_BE;	// DVD drives always support READ_CD command
				return TRUE;
			}
		}
		break;
	}

	// check if the drive is MMC capable
	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildGetConfigurationCDB(&cdb, SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL, FeatureProfileList, sizeof(GET_CONFIGURATION_HEADER));
	if (if_layer.ExecScsiCmd(sense_buf, sizeof(GET_CONFIGURATION_HEADER), &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2)) {
		*rmode = ReadMode_Scsi_BE;		// MMC drives always support READ_CD command
		return TRUE;
	}
//	if (scsi_err != ERROR_INVALID_FUNCTION && scsi_err != ERROR_NOT_READY &&
//		scsi_err == ERROR_IO_DEVICE && scsi_err != ERROR_SEM_TIMEOUT)
//		return FALSE;

	// try issuing directly a READ_CD command to the driver
	for (cnt = 0; cnt < 2; cnt++) {
		use6Byte = (cnt == 0) ? TRUE : FALSE;
		if (!scsi_ModeSense(MODE_PAGE_ERROR_RECOVERY, sense_buf, sizeof(sense_buf), FALSE, use6Byte))
			continue;
		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildReadSectorCDB(&cdb, 0, 0, &scsi_addr, ReadMode_Scsi_BE, SubQMode_Disabled);
		if (if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2) ||
			scsi_err == ERROR_NOT_READY ||
			scsi_err == ERROR_SECTOR_NOT_FOUND ||
			scsi_err == ERROR_UNRECOGNIZED_MEDIA) {
			*rmode = ReadMode_Scsi_BE;	// drive support READ_CD command
			return TRUE;
		}
	}
	// drive doesn't support READ_CD, defaulting read mode to READ_10
	*rmode = ReadMode_Scsi_28;
	return TRUE;
}

BOOL scsi_InitReadMode(eReadMode rmode)
{
	// N.B.: save current READ MODE !!! (see *ReadSector() functions)
	scsi_rmode = rmode;
	// N.B.: always use subQmode == SubQMode_Disabled for reads (sector size = 2352) !!!
	return scsi_SelectReadMode(rmode, SubQMode_Disabled);
}

static BOOL scsi_SelectReadMode(eReadMode rmode, eSubQMode subQmode)
{
	char	sense_buf[255];
	int		cnt, sense_len, block_size;
	BOOL	use6Byte;
	CDB		cdb;
	BYTE	cdblen;
	static int	last_block_size = COOKED_SECTOR_SIZE;

	block_size = (rmode == ReadMode_Auto) ? COOKED_SECTOR_SIZE :
		(subQmode == SubQMode_Read_PW) ? RAW_PW_SECTOR_SIZE :
		(subQmode == SubQMode_Read_Q) ? RAW_Q_SECTOR_SIZE :
		RAW_SECTOR_SIZE;

	switch (rmode)
	{
	case ReadMode_Auto:		// set block size = 2048
	case ReadMode_Scsi_28:	// READ_10: set block size = 2352 / 2368 / 2448
		break;
	case ReadMode_Scsi_D4:	// NEC_READ_CDDA: nothing to do
		if (subQmode == SubQMode_Read_Q && subQmode == SubQMode_Read_PW)
			return FALSE;				// no subchannel read mode support
	case ReadMode_Scsi_D8:	// PLXTR_READ_CDDA: nothing to do
	case ReadMode_Scsi_BE:	// READ_CD: nothing to do
		return TRUE;					// no MODE_SELECT needed
	case ReadMode_Raw:
	default:
		return FALSE;					// not a valid SCSI read mode
	}

	// If the sector block size was already set as espected, don't need to send the MODE_SELECT command again.
	// Note: The drive address is not saved and MUST NOT CHANGE until a ReadMode_Auto was issued !!!
	if (block_size == last_block_size)
		return TRUE;				// no MODE_SELECT needed
	for (cnt = 0; cnt < 2; cnt++)
	{
		use6Byte = (cnt == 0) ? TRUE : FALSE;
		sense_len = sizeof(sense_buf);
		scsi_BuildModeSelectDATA(sense_buf, &sense_len, block_size, NULL, 0, use6Byte);
		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildModeSelectCDB(&cdb, sense_len, use6Byte);
		if (if_layer.ExecScsiCmd(sense_buf, sense_len, &cdb, cdblen, SCSI_IOCTL_DATA_OUT, 2))
		{
			last_block_size = block_size;	// save last block size sent to the drive
			return TRUE;
		}
	}
	return FALSE;
}

BOOL scsi_InitSubQMode(eSubQMode subQmode)
{
	// N.B.: save current SUBCHANNEL READ MODE !!! (see *ReadSubQ() functions)
	scsi_subQmode = subQmode;
	return TRUE;
}

BOOL scsi_DetectSubQMode(eSubQMode *subQmode)
{
	static eReadMode	rmodes[] = { ReadMode_Scsi_BE, ReadMode_Scsi_D8, ReadMode_Scsi_28, };
	static eSubQMode	qmodes[] = { SubQMode_Read_PW, SubQMode_Read_Q };
	int		i, j, loc;
	Q_SUBCHANNEL_DATA	rbuf;

	for (i = 0; i < (sizeof(rmodes) / sizeof(rmodes[0])); i++)
		for (j = 0; j < (sizeof(qmodes) / sizeof(qmodes[0])); j++) {
			if_layer.InitSubQMode(*subQmode = MAKE_SUBQMODE(rmodes[i], qmodes[j]));
			for (loc = 16; loc < (16 + 3); loc++) {
				memset(&rbuf, 0, sizeof(rbuf));
				if (if_layer.ReadSubQ(&rbuf, loc)) {
					// check if 'rbuf' contains a valid subQ data
					if (!scsi_ValidateSubQData(&rbuf)) {
#ifdef	_DEBUG
						if (rbuf.ADR == ADR_ENCODES_CURRENT_POSITION) {
							int		n;
							BYTE	*p;
							PRINTF(("Warning: Invalid subchannel data returned:\n"));
							PRINTF(("SubQ Mode = '%s'\n", subq_mode_name(*subQmode)));
							PRINTF(("Data ="));
							for (n = 0, p = (BYTE *)&rbuf; n < 16; n++, p++)
								PRINTF((" %02X", *p));
							PRINTF(("\n"));
						}
#endif//_DEBUG
						continue;	// if not valid, retry with next sector (max. 3 times)
					}
				{ int __TODO__check_se_tornato_address_in_BCD__; }
					//@@@ TO DO: PROVARE ANCHE CON FORMATO NON-BCD !!!
					return TRUE;
				} else if (scsi_err == ERROR_NOT_READY) {
					// if 'Unit not Ready' error, abort mode detection
					if_layer.InitSubQMode(*subQmode = SubQMode_Auto);
					return FALSE;
				}
				// if other scsi error, continue with next mode
				break;
			}
		}
	// as last resort, use the 'SEEK & READ SUBCHANNEL' mode (absolutely not reliable !!!)
	if_layer.InitSubQMode(*subQmode = MAKE_SUBQMODE(ReadMode_Raw, SubQMode_SubQ));
	return TRUE;
}

BOOL scsi_ReadToc(void)
{
	CDB		cdb;
	BYTE	cdblen;

// TOC Flag:
// - toc.TrackData[].Control & AUDIO_DATA_TRACK == 0 -> AUDIO TRACK
// - toc.TrackData[].Control & AUDIO_DATA_TRACK == 1 -> DATA TRACK
// Read sector Flag (DATA track only):
// sector_data.data_header.data_mode (data+15) == 0 / 1 / 2 (Sector Mode 0 / 1 / 2)

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildReadTocCDB(&cdb, sizeof(toc.TrackData));
	if (!if_layer.ExecScsiCmd(&toc, sizeof(toc), &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2))
		return FALSE;
	return TRUE;
}

BOOL scsi_PlayAudioTrack(const MSF *start, const MSF *end)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildPlayAudioMSFCDB(&cdb, start, end);
	return if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, 2);
}

BOOL scsi_StopAudioTrack(void)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildPauseResumeCDB(&cdb, CDB_AUDIO_PAUSE, &scsi_addr);
	return if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, 2);
}

BOOL scsi_TestUnitReady(BOOL do_retry)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildTestUnitReadyCDB(&cdb, &scsi_addr);
	return if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, do_retry ? 2 : 0);	// arg6: nr. of retries
}

BOOL scsi_GetPlayLocation(MSF *play_loc, BYTE *audio_status)
{
	CDB		cdb;
	BYTE	cdblen;
	SUB_Q_CHANNEL_DATA	data;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildReadQChannelCDB(&cdb, IOCTL_CDROM_CURRENT_POSITION);
	if (!if_layer.ExecScsiCmd(&data, sizeof(data), &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2))
		return FALSE;
	play_loc->minute = data.CurrentPosition.AbsoluteAddress[1];
	play_loc->second = data.CurrentPosition.AbsoluteAddress[2];
	play_loc->frame = data.CurrentPosition.AbsoluteAddress[3];
	// audio_status: AUDIO_STATUS_IN_PROGRESS=play, AUDIO_STATUS_PAUSED=paused, AUDIO_STATUS_NO_STATUS/other=no play
	*audio_status = data.CurrentPosition.Header.AudioStatus;
	return TRUE;
}

BOOL scsi_LoadEjectUnit(BOOL load)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildStartStopCDB(&cdb, load, TRUE, &scsi_addr);
	return if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, 2);
}

BOOL scsi_SetCdSpeed(int speed)
{
	CDB		cdb;
	BYTE	cdblen;

	memset(&cdb, 0, sizeof(cdb));
	cdblen = scsi_BuildSetCdSpeedCDB(&cdb, speed);
	return if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, 2);
}

BOOL scsi_SetCdSpindown(int timer, int *prev_timer)
{
	CDROM_PARAMETERS_PAGE	mode_pages_data;
	char	sense_buf[1024];
	const void	*mode_pages_ary[1];
	int		num_mode_pages, i;
	int		cnt, sense_len;
	BOOL	use6Byte;
	CDB		cdb;
	BYTE	cdblen;

	memset(&mode_pages_data, 0, sizeof(mode_pages_data));
	mode_pages_data.PageCode = MODE_PAGE_CDROM_PARAMETERS;
	mode_pages_data.PageLength = sizeof(CDROM_PARAMETERS_PAGE)-2;
	mode_pages_data.InactivityTimerMultiplier = timer;
	//mode_pages_data.NumberOfSUnitsPerMUnits[0] = 0;
	mode_pages_data.NumberOfSUnitsPerMUnits[1] = 60;
	//mode_pages_data.NumberOfFUnitsPerSUnits[0] = 0;
	mode_pages_data.NumberOfFUnitsPerSUnits[1] = 75;

	for (cnt = 0; cnt < 2; cnt++)
	{
		use6Byte = (cnt == 0) ? TRUE : FALSE;
		memset(&sense_buf, 0, sizeof(sense_buf));
		if (!scsi_ModeSense(MODE_PAGE_CDROM_PARAMETERS, sense_buf, sizeof(sense_buf), TRUE, use6Byte))
			continue;
		num_mode_pages = sizeof(mode_pages_ary) / sizeof(mode_pages_ary[0]);
		scsi_ParseModeSenseBuffer(sense_buf, NULL, NULL, mode_pages_ary, &num_mode_pages, use6Byte);
		for (i = 0; i < num_mode_pages; i++) {
			CDROM_PARAMETERS_PAGE *cdpPage = (CDROM_PARAMETERS_PAGE *)mode_pages_ary[i];
			if (cdpPage->PageCode != MODE_PAGE_CDROM_PARAMETERS ||
				cdpPage->PageLength != (sizeof(CDROM_PARAMETERS_PAGE)-2))
				continue;
			if (prev_timer != NULL)
				*prev_timer = cdpPage->InactivityTimerMultiplier;
			PRINTF(("Default Spindown Timer = %s\n", cd_spindown_name(cdpPage->InactivityTimerMultiplier)));
			break;
		}
		if (i == num_mode_pages)
			continue;
		sense_len = sizeof(sense_buf);
		scsi_BuildModeSelectDATA(sense_buf, &sense_len, 0, &mode_pages_data, sizeof(mode_pages_data), use6Byte);
		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildModeSelectCDB(&cdb, sense_len, use6Byte);
		if (if_layer.ExecScsiCmd(sense_buf, sense_len, &cdb, cdblen, SCSI_IOCTL_DATA_OUT, 2))
			return TRUE;
	}
	return FALSE;
}

BOOL scsi_ReadSubQ(Q_SUBCHANNEL_DATA *rbuf, int loc)
{
	CDB		cdb;
	BYTE	cdblen;

	if (SUBQ_MODE(scsi_subQmode) == SubQMode_Auto) {
		eSubQMode	tmp_subQmode;
		if (!if_layer.DetectSubQMode(&tmp_subQmode))
			return FALSE;
		PRINTF(("Detected SubQ Mode = %s\n", subq_mode_name(tmp_subQmode)));
		if_layer.InitSubQMode(tmp_subQmode);
	}

	switch (SUBQ_MODE(scsi_subQmode))
	{
	case SubQMode_Read_Q:
	case SubQMode_Read_PW: {
		BYTE	data[RAW_PW_SECTOR_SIZE];
		BOOL	res;
		int		tmp_scsi_err;

		if (!scsi_SelectReadMode(SUBQ_RMODE(scsi_subQmode), SUBQ_MODE(scsi_subQmode)))	// adjust block size (for ReadMode_Scsi_28)
			return FALSE;
		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildReadSectorCDB(&cdb, loc, 1, &scsi_addr, SUBQ_RMODE(scsi_subQmode), SUBQ_MODE(scsi_subQmode));
		memset(&data[RAW_SECTOR_SIZE], 0, sizeof(data) - RAW_SECTOR_SIZE);
		res = if_layer.ExecScsiCmd(data, (SUBQ_MODE(scsi_subQmode) == SubQMode_Read_Q) ? RAW_Q_SECTOR_SIZE : RAW_PW_SECTOR_SIZE, &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2);
		tmp_scsi_err = scsi_err;	// save result & 'scsi_err' value
		scsi_SelectReadMode(scsi_rmode, SubQMode_Disabled);	// restore block size (for ReadMode_Scsi_28)
		scsi_err = tmp_scsi_err;	// restore the 'scsi_err'value returned from 'ExecScsiCmd' call
		if (!res)
			return FALSE;
		if (SUBQ_MODE(scsi_subQmode) == SubQMode_Read_Q) {
			memcpy(rbuf, &data[RAW_SECTOR_SIZE], 16);
			{ int __TODO__check_se_convertire_address_in_BCD__; }
			// se necessario, convertire TrackRelativeAddress[] & AbsoluteAddress[] in BCD
		} else {
			scsi_DecodeSubPWtoQ(rbuf, &data[RAW_SECTOR_SIZE]);
		}
		} break;
	case SubQMode_SubQ: {
		SUB_Q_CHANNEL_DATA	data;

		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildSeekCDB(&cdb, loc, &scsi_addr);
		if (!if_layer.ExecScsiCmd(NULL, 0, &cdb, cdblen, SCSI_IOCTL_DATA_UNSPECIFIED, 2))
			return FALSE;
		memset(&cdb, 0, sizeof(cdb));
		cdblen = scsi_BuildReadQChannelCDB(&cdb, IOCTL_CDROM_CURRENT_POSITION);
		memset(&data, 0, sizeof(data));
		if (!if_layer.ExecScsiCmd(&data, sizeof(data), &cdb, cdblen, SCSI_IOCTL_DATA_IN, 2))
			return FALSE;
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
		} break;
	case SubQMode_Auto:
	case SubQMode_Disabled:
	default:
		return FALSE;
	}
	return TRUE;
}
