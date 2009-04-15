// Plugin Defs
#include "Locals.h"
#include "mscdex.h"

// DPMI interrupt registers:
typedef struct {
	DWORD	RealMode_EDI;
	DWORD	RealMode_ESI;
	DWORD	RealMode_EBP;
	DWORD	RealMode_res0;
	DWORD	RealMode_EBX;
	DWORD	RealMode_EDX;
	DWORD	RealMode_ECX;
	DWORD	RealMode_EAX;
	WORD	RealMode_Flags;
	WORD	RealMode_ES;
	WORD	RealMode_DS;
	WORD	RealMode_FS;
	WORD	RealMode_GS;
	WORD	RealMode_IP;
	WORD	RealMode_CS;
	WORD	RealMode_SP;
	WORD	RealMode_SS;
} REAL_MODE_CALL_REGS;

// DOS device driver request structs:
#pragma pack(1)

#pragma pack()

static DWORD	VxDCall;									//KERNEL32.1/9
static DWORD 	(WINAPI *LoadLibrary16)(char *);			//KERNEL32.35
static void 	(WINAPI *FreeLibrary16)(DWORD);				//KERNEL32.36
static DWORD	(WINAPI *GetProcAddress16)(DWORD, char *);	//KERNEL32.37
static DWORD	GlobalDosAlloc, GlobalDosFree, QT_Thunk;	//from KERNEL.EXE

// VWin32.vxd service numbers
static DWORD	VWin32_Get_Version		= 0x002A0000;
static DWORD	VWin32_Int21Dispatch	= 0x002A0010;
static DWORD	VWin32_Int31Dispatch	= 0x002A0029;

#define	DOS_MEM_DATA_BLOCK_SIZE			\
	(((sizeof(DEVICE_REQUEST) + sizeof(CONTROL_BLOCK) + 0x0F) & ~0x0F) + (MAX_SECTORS_PER_READ * RAW_SECTOR_SIZE))
#define	DOS_MEM_DEVICE_REQUEST_OFFSET	0
#define	DOS_MEM_CONTROL_BLOCK_OFFSET	sizeof(DEVICE_REQUEST)
#define	DOS_MEM_SECTOR_BUFFER_OFFSET	((sizeof(DEVICE_REQUEST) + sizeof(CONTROL_BLOCK) + 0x0F) & ~0x0F)

// macros to convert DOS <-> WIN32 address:
#define	DOS_SEGADDR(win32_addr)	(((((DWORD)win32_addr) & ~0x0F) << 12) | (((DWORD)win32_addr) & 0x0F))
#define	WIN32_ADDR(dos_seg)		(((DWORD)dos_seg) << 4)

static DWORD	dos_mem_data_block = 0;		// DOS allocated memory block descriptor (RM/PM memory segments)
static DWORD	dos_mem_data_segment = 0;	// DOS memory data segment

static DEVICE_REQUEST	*device_request;	// device driver request ptr.
static CONTROL_BLOCK	*control_block;		// control block ptr.
static BYTE				*sector_buffer;		// sector read buffer ptr.

static BOOL		MSCDEX_CHECK_STATUS(REQUEST_STATUS rq_status, DWORD *scsi_err);
static BOOL		mscdex_DriveCheck(BYTE drive_letter);
static void		mscdex_DeviceRequest(BYTE drive_letter);
static void		mscdex_IoctlInput(int cblen, BYTE drive_letter);
static void		mscdex_IoctlOutput(int cblen, BYTE drive_letter);
static void		get_vwin32_services(void);
static void		call_INT2F(REAL_MODE_CALL_REGS *regs);
static DWORD	call_GlobalDosAlloc(DWORD);
static void		call_GlobalDosFree(DWORD);

//------------------------------------------------------------------------------
// MSCDEX interface functions (Win9x):
//------------------------------------------------------------------------------

BOOL mscd_LoadLayer(void)
{
	HMODULE				hKernel32Dll;
	DWORD				hKernel16Exe;
	char				*base_address;
	IMAGE_DOS_HEADER	*dos_header;
	IMAGE_NT_HEADERS	*nt_header;
	IMAGE_EXPORT_DIRECTORY	*export_dir;
	DWORD				*exports_by_ordinal;
	int					i;//, num_exports;

	if (dos_mem_data_block != 0)
		return TRUE;

	// Note: The standard method to get the KERNEL32 export addresses by ordinal thru ...
	// ... GetProcAddress() doesn't work under Win9x (GetProcAddress(hk32, (LPCSTR)0x0001) ...
	// ... returns NULL, and GetLastError() returns ERROR_NOT_SUPPORTED) !!!
	// The only 'working' method is reading directly from the KERNEL32 memory space the PE header ...
	// ... and getting the wanted ordinal address values from the PE export directory.
	hKernel32Dll = GetModuleHandle("kernel32.dll");
	base_address = (char *)hKernel32Dll;
	dos_header = (IMAGE_DOS_HEADER *)base_address;
	nt_header = (IMAGE_NT_HEADERS *)(base_address + dos_header->e_lfanew);
	export_dir = (IMAGE_EXPORT_DIRECTORY *)(base_address + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	exports_by_ordinal = (DWORD *)(base_address + (DWORD)export_dir->AddressOfFunctions);
	//num_exports = export_dir->NumberOfFunctions;
	// Note: The first 9 entries of KERNEL32.DLL must need to have the same address ...
	// ... value (_VxDCall1@8 to _VxDCall9@40), otherwise there is something of wrong.
	for (i = 1; i < 9; i++)
		if (exports_by_ordinal[i] != exports_by_ordinal[0])
			return FALSE;
	// get the wanted kernel32.dll ordinal entry points:
	if ((VxDCall = (DWORD)(base_address + exports_by_ordinal[0x0001 - 1])) == 0
		|| (LoadLibrary16 = (DWORD (WINAPI *)(char *))(base_address + exports_by_ordinal[0x0023 - 1])) == NULL
		|| (FreeLibrary16 = (void (WINAPI *)(DWORD))(base_address + exports_by_ordinal[0x0024 - 1])) == NULL
		|| (GetProcAddress16 = (DWORD (WINAPI *)(DWORD, char *))(base_address + exports_by_ordinal[0x0025 - 1])) == NULL
		|| (QT_Thunk = (DWORD)GetProcAddress(hKernel32Dll, "QT_Thunk")) == 0)
		return FALSE;
	// check the VWin32.vxd version to set the right VWIN32 service numbers
	get_vwin32_services();
	// get GlobalDosAlloc / GlobalDosFree functions from KERNEL.EXE
	if ((hKernel16Exe = LoadLibrary16("KERNEL.EXE")) < 32)
		return FALSE;
	FreeLibrary16(hKernel16Exe);
	if ((GlobalDosAlloc = GetProcAddress16(hKernel16Exe, "GlobalDosAlloc")) == 0 ||
		(GlobalDosFree = GetProcAddress16(hKernel16Exe, "GlobalDosFree")) == 0)
		return FALSE;
	// allocate a single DOS memory block for both device request, control block & sector data buffer
	if ((dos_mem_data_block = call_GlobalDosAlloc(DOS_MEM_DATA_BLOCK_SIZE)) == 0)
		return FALSE;
	// setup ptrs to the real/virtual address of the DOS memory data block
	dos_mem_data_segment = (DWORD)dos_mem_data_block >> 16;	// DOS memory data segment
	device_request = (DEVICE_REQUEST *)(WIN32_ADDR(dos_mem_data_segment) + DOS_MEM_DEVICE_REQUEST_OFFSET);
	control_block = (CONTROL_BLOCK *)(WIN32_ADDR(dos_mem_data_segment) + DOS_MEM_CONTROL_BLOCK_OFFSET);
	sector_buffer = (BYTE *)(WIN32_ADDR(dos_mem_data_segment) + DOS_MEM_SECTOR_BUFFER_OFFSET);
	return TRUE;
}

void mscd_UnloadLayer(void)
{
	if (dos_mem_data_block == 0)
		return;
	call_GlobalDosFree(dos_mem_data_block);
	dos_mem_data_block = 0;
	device_request = NULL;
	control_block = NULL;
	sector_buffer = NULL;
}

BOOL mscd_QueryAvailDrives(void)
{
	SCSIADDR	addr;

	// reset any previous drive info
	memset(found_drives, 0, sizeof(found_drives));
	memset(&drive_info, 0, sizeof(drive_info));
	num_found_drives = 0;

	for (addr.drive_letter = 'A'; addr.drive_letter <= 'Z'; addr.drive_letter++)	// 'C' - 'Z'
	{
		if (!mscdex_DriveCheck(addr.drive_letter))
			continue;
		// no SCSI address available with MSCDEX, fill at FF
		addr.haid = addr.target = addr.lun = 0xFF;
		// no inquiry data available with MSCDEX, leave 'drive_info' empty
		if (num_found_drives >= MAX_SCSI_UNITS)
			continue;
		found_drives[num_found_drives] = addr;
		num_found_drives++;
	}
	return TRUE;
}

BOOL mscd_ReadToc(void)
{
	int		idx;

	memset(&toc, 0, sizeof(toc));

	memset(control_block, 0, sizeof(*control_block));
	control_block->audio_disk_info.header.cmd = AUDIO_DISK_INFO;
	mscdex_IoctlInput(sizeof(control_block->audio_disk_info), scsi_addr.drive_letter);
	if (!MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err))
		return FALSE;
	toc.FirstTrack = control_block->audio_disk_info.first;
	toc.LastTrack = control_block->audio_disk_info.last;
	//toc.TrackData[toc.LastTrack].TrackNumber = 0xAA;
	//toc.TrackData[toc.LastTrack].Address[0] = control_block->audio_disk_info.leadout.reserved;
	toc.TrackData[toc.LastTrack].Address[1] = control_block->audio_disk_info.leadout.minute;
	toc.TrackData[toc.LastTrack].Address[2] = control_block->audio_disk_info.leadout.second;
	toc.TrackData[toc.LastTrack].Address[3] = control_block->audio_disk_info.leadout.frame;
	for (idx = toc.FirstTrack; idx <= toc.LastTrack; idx++)
	{
		memset(control_block, 0, sizeof(*control_block));
		control_block->audio_track_info.header.cmd = AUDIO_TRACK_INFO;
		control_block->audio_track_info.track = idx;
		mscdex_IoctlInput(sizeof(control_block->audio_track_info), scsi_addr.drive_letter);
		if (!MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err))
			return FALSE;
		toc.TrackData[idx - toc.FirstTrack].Control = control_block->audio_track_info.ctrlinfo >> 4;
		//toc.TrackData[idx - toc.FirstTrack].Adr = control_block->audio_track_info.ctrlinfo & 0x0F;
		toc.TrackData[idx - toc.FirstTrack].TrackNumber = idx;
		//toc.TrackData[idx - toc.FirstTrack].Address[0] = control_block->audio_track_info.loc.reserved;
		toc.TrackData[idx - toc.FirstTrack].Address[1] = control_block->audio_track_info.loc.minute;
		toc.TrackData[idx - toc.FirstTrack].Address[2] = control_block->audio_track_info.loc.second;
		toc.TrackData[idx - toc.FirstTrack].Address[3] = control_block->audio_track_info.loc.frame;
	}
	return TRUE;
}

BOOL mscd_StartReadSector(BYTE *rbuf, int loc, int nsec)
{
	if (loc < 16) {	// Warning: MSCDEX can't read sectors 0 - 15 !!!
		for (; loc < 16 && nsec > 0; loc++, nsec--, rbuf += RAW_SECTOR_SIZE) {
			//MSF		msf;
			memset(rbuf, 0, RAW_SECTOR_SIZE);
			//INT2MSF(&msf, loc);				// Header (sector nr.)
			//rbuf[12] = INT2BCD(msf.minute);
			//rbuf[13] = INT2BCD(msf.second);
			//rbuf[14] = INT2BCD(msf.frame);
			rbuf[13] = 2;
			rbuf[14] = INT2BCD((BYTE)loc);
			rbuf[15] = 2;					// Header (sector type: 2=MODE2)
			rbuf[18] = rbuf[22] = 0x08;		// SubHeader (Data)
		}
		if (nsec == 0) {	// if all the sectors are < 16, simulate a read OK
			pending_read_running = FALSE;
			return TRUE;
		}
	}
	memset(device_request, 0, sizeof(*device_request));
	device_request->read_long.header.len = sizeof(device_request->read_long);
	device_request->read_long.header.cmd = DEV_READ_LONG;
	//device_request->read_long.addr_mode = 0;	// 0 = HSG (default)
	device_request->read_long.addr = DOS_SEGADDR(sector_buffer);
	device_request->read_long.nsec = nsec;
	device_request->read_long.loc = loc;
	device_request->read_long.rd_mode = 1;	// 1 = RAW (2352 bytes per frame, including EDC/ECC)
	mscdex_DeviceRequest(scsi_addr.drive_letter);
	if (!MSCDEX_CHECK_STATUS(device_request->read_long.header.status, &scsi_err))
		return FALSE;
	// copy readed data from dos memory block to target buffer
	memcpy(rbuf, sector_buffer, nsec * RAW_SECTOR_SIZE);
	// Warning: the flag 'pending_read_running' indicate if the read operation is done ...
	// ... from the *_StartReadSector() (FALSE) or from the *_WaitEndReadSector() (TRUE)
	pending_read_running = FALSE;		// read operation already finished, no *_WaitEndReadSector() needed
	return TRUE;
}

BOOL mscd_TestUnitReady(BOOL do_retry)
{
	memset(control_block, 0, sizeof(*control_block));
	control_block->device_status.header.cmd = DEVICE_STATUS;
	mscdex_IoctlInput(sizeof(control_block->device_status), scsi_addr.drive_letter);
	if (!MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err))
		return FALSE;
	return (control_block->device_status.parms.diskabsent) ? FALSE : TRUE;
}

BOOL mscd_PlayAudioTrack(const MSF *start, const MSF *end)
{
	int		start_loc = MSF2INT(start);
	int		end_loc = MSF2INT(end);

	memset(device_request, 0, sizeof(*device_request));
	device_request->play_audio.header.len = sizeof(device_request->play_audio);
	device_request->play_audio.header.cmd = DEV_PLAY_AUDIO;
	//device_request->play_audio.addr_mode = 0;	// 0 = HSG (default)
	device_request->play_audio.loc = start_loc;
	device_request->play_audio.nsec = end_loc - start_loc;
	mscdex_DeviceRequest(scsi_addr.drive_letter);
	return MSCDEX_CHECK_STATUS(device_request->play_audio.header.status, &scsi_err);
}

BOOL mscd_StopAudioTrack(void)
{
	memset(device_request, 0, sizeof(*device_request));
	device_request->stop_audio.header.len = sizeof(device_request->stop_audio);
	device_request->stop_audio.header.cmd = DEV_STOP_AUDIO;
	mscdex_DeviceRequest(scsi_addr.drive_letter);
	return MSCDEX_CHECK_STATUS(device_request->stop_audio.header.status, &scsi_err);
}

BOOL mscd_GetPlayLocation(MSF *play_loc, BYTE *audio_status)
{
	memset(control_block, 0, sizeof(*control_block));
	control_block->audio_q_channel_info.header.cmd = AUDIO_Q_CHANNEL_INFO;
	mscdex_IoctlInput(sizeof(control_block->audio_q_channel_info), scsi_addr.drive_letter);
	if (!MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err))
		return FALSE;
	*play_loc = control_block->audio_q_channel_info.rundisk;
	*audio_status = (device_request->ioctl_input.header.status.busy) ?
		AUDIO_STATUS_IN_PROGRESS : AUDIO_STATUS_NO_STATUS;
	return TRUE;
}

BOOL mscd_ReadSubQ(Q_SUBCHANNEL_DATA *data, int loc)
{
	MSF		msf;

	INT2MSF(&msf, loc);
	memset(control_block, 0, sizeof(*control_block));
	control_block->audio_subchannel_info.header.cmd = AUDIO_SUBCHANNEL_INFO;
	control_block->audio_subchannel_info.loc.frame = msf.frame;
	control_block->audio_subchannel_info.loc.second = msf.second;
	control_block->audio_subchannel_info.loc.minute = msf.minute;
	control_block->audio_subchannel_info.addr = DOS_SEGADDR(sector_buffer);
	control_block->audio_subchannel_info.nsec = 1;
	mscdex_IoctlInput(sizeof(control_block->audio_subchannel_info), scsi_addr.drive_letter);
	// Fix for Win9x CDVSD.VXD error handling bug: change error 02 (not ready) in 03 (bad command)
	if (device_request->ioctl_input.header.status.error && device_request->ioctl_input.header.status.errorcode == 0x02)
		device_request->ioctl_input.header.status.errorcode = 0x03;
	if (!MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err))
		return FALSE;
	scsi_DecodeSubPWtoQ(data, sector_buffer);	// decode subchannel data (96 -> 12 bytes)
	return TRUE;
}

BOOL mscd_LoadEjectUnit(BOOL load)
{
	// Note: close_tray & eject_disk control block structs are the same
	memset(control_block, 0, sizeof(*control_block));
	control_block->close_tray.header.cmd = (load) ? CLOSE_TRAY : EJECT_DISK;
	mscdex_IoctlOutput(sizeof(control_block->close_tray), scsi_addr.drive_letter);
	return MSCDEX_CHECK_STATUS(device_request->ioctl_output.header.status, &scsi_err);
}

//------------------------------------------------------------------------------
// MSCDEX other support functions:
//------------------------------------------------------------------------------
static BOOL MSCDEX_CHECK_STATUS(REQUEST_STATUS rq_status, DWORD *scsi_err)
{
	DWORD	status = NO_ERROR;
/*	static const DWORD mscdex_devicerequest_errorcodes_00to0F[16] = {
		ERROR_WRITE_PROTECT,			// 00 = write-protect violation
		ERROR_BAD_UNIT,					// 01 = unknown unit
		ERROR_NOT_READY,				// 02 = drive not ready
		ERROR_BAD_COMMAND,				// 03 = unknown command
		ERROR_CRC,						// 04 = CRC error
		ERROR_BAD_LENGTH,				// 05 = bad drive request structure length
		ERROR_SEEK,						// 06 = seek error
		ERROR_NOT_DOS_DISK,				// 07 = unknown media
		ERROR_SECTOR_NOT_FOUND,			// 08 = sector not found
		ERROR_OUT_OF_PAPER,				// 09 = printer out of paper
		ERROR_WRITE_FAULT,				// 0A = write fault
		ERROR_READ_FAULT,				// 0B = read fault
		ERROR_GEN_FAILURE,				// 0C = general failure
		ERROR_SHARING_VIOLATION,		// 0D = reserved
		ERROR_LOCK_VIOLATION,			// 0E = reserved
		ERROR_WRONG_DISK,				// 0F = invalid disk change
	};*/ // (this table is for reference only, see below)

	if (rq_status.error) {
		// Note: MSCDEX error codes 0x00 to 0x0F are mapped directly on WIN32 ...
		// ... error codes from 'ERROR_WRITE_PROTECT' to 'ERROR_WRONG_DISK' !!!
		status = (rq_status.errorcode <= 0x0F) ?
			rq_status.errorcode + ERROR_WRITE_PROTECT : ERROR_IO_DEVICE;
	}
	if (status == ERROR_NOT_READY || status == ERROR_WRONG_DISK)
		cache_FlushBuffers();			// media changed: empty cache buffers !!!
	*scsi_err = status;
	if (status == NO_ERROR)
		return TRUE;
	PRINTF(("MSCDEX_CHECK_STATUS: winerr=%d\n", status));
	return FALSE;
}

static BOOL mscdex_DriveCheck(BYTE drive_letter)
{
	REAL_MODE_CALL_REGS	regs;

	memset(&regs, 0, sizeof(regs));
	regs.RealMode_EAX = 0x150B;
	regs.RealMode_ECX = drive_letter - 'A';
	call_INT2F(&regs);
	if (LOWORD(regs.RealMode_EBX) != 0xADAD ||	// BX = ADADh if MSCDEX.EXE installed
		LOWORD(regs.RealMode_EAX) == 0x0000)	// AX = 0000h if drive not supported
		return FALSE;
	return TRUE;
}

/*
static WORD mscdex_GetVersion(void)
{
	REAL_MODE_CALL_REGS	regs;

	memset(&regs, 0, sizeof(regs));
	regs.RealMode_EAX = 0x150C;
	call_INT2F(&regs);
	// return: BH = major version, BL = minor version
	return LOWORD(regs.RealMode_EBX);
}

static BOOL mscdex_AbsDiskRead(int loc)
{
	REAL_MODE_CALL_REGS	regs;

	memset(&regs, 0, sizeof(regs));
	regs.RealMode_EAX = 0x1508;
	regs.RealMode_ECX = scsi_addr.drive_letter - 'A';
	regs.RealMode_EDX = 1;				// numer of sectors: always 1 !!!
	regs.RealMode_ES = (WORD)dos_mem_data_segment;
	regs.RealMode_EBX = DOS_MEM_SECTOR_BUFFER_OFFSET;
	regs.RealMode_ESI = HIWORD(loc);
	regs.RealMode_EDI = LOWORD(loc);
	call_INT2F(&regs);
	if (regs.RealMode_Flags & 0x0001)	// test Carry flag
	{
		// AX is already mapped on WIN32 error codes
		scsi_err = LOWORD(regs.RealMode_EAX);
		return FALSE;
	}
	return TRUE;
}

BOOL mscdex_GetDeviceStatus(void)
{
	memset(control_block, 0, sizeof(*control_block));
	control_block->device_status.header.cmd = DEVICE_STATUS;

	mscdex_IoctlInput(sizeof(control_block->device_status), scsi_addr.drive_letter);
	return MSCDEX_CHECK_STATUS(device_request->ioctl_input.header.status, &scsi_err);
}
*/

//------------------------------------------------------------------------------
// MSCDEX 'low level' functions:
//------------------------------------------------------------------------------

static void mscdex_DeviceRequest(BYTE drive_letter)
{
	REAL_MODE_CALL_REGS	regs;

	memset(&regs, 0, sizeof(regs));
	regs.RealMode_EAX = 0x1510;
	regs.RealMode_ECX = drive_letter - 'A';
	regs.RealMode_ES = (WORD)dos_mem_data_segment;
	regs.RealMode_EBX = DOS_MEM_DEVICE_REQUEST_OFFSET;
	call_INT2F(&regs);
}

static void mscdex_IoctlInput(int cblen, BYTE drive_letter)
{
	memset(device_request, 0, sizeof(*device_request));
	device_request->ioctl_input.header.len = sizeof(device_request->ioctl_input);
	device_request->ioctl_input.header.cmd = DEV_IOCTL_INPUT;
	//device_request->ioctl_input.media = 0;
	device_request->ioctl_input.addr = DOS_SEGADDR(control_block);
	device_request->ioctl_input.len = cblen;
	mscdex_DeviceRequest(drive_letter);
}

static void mscdex_IoctlOutput(int cblen, BYTE drive_letter)
{
	memset(device_request, 0, sizeof(*device_request));
	device_request->ioctl_output.header.len = sizeof(device_request->ioctl_output);
	device_request->ioctl_output.header.cmd = DEV_IOCTL_OUTPUT;
	//device_request->ioctl_output.media = 0;
	device_request->ioctl_output.addr = DOS_SEGADDR(control_block);
	device_request->ioctl_output.len = cblen;
	mscdex_DeviceRequest(drive_letter);
}

static void get_vwin32_services(void)
{
	WORD		vwin32_version;

__asm {
	push	VWin32_Get_Version
	call	VxDCall
	mov		[vwin32_version], ax
	}
	if (vwin32_version <= 0x0103 || vwin32_version >= 0x0400) {
		//Win95 				(4.00.950)	VWin32_Version = 0x0102
		//Win95 OSR2			(4.00.1111)	VWin32_Version = 0x0103
		//Win95 OSR2.5			(4.00.1111)	VWin32_Version = 0x0103
		//Win98 				(4.10.1998)	VWin32_Version = 0x040A
		//Win98 SE				(4.10.2222)	VWin32_Version = 0x040A
		VWin32_Int21Dispatch = 0x002A0010;
		VWin32_Int31Dispatch = 0x002A0029;
	} else {
		//Win95 OSR2.1 USB SUPP	(4.03.1212)	VWin32_Version = 0x0104
		VWin32_Int21Dispatch = 0x002A000D;
		VWin32_Int31Dispatch = 0x002A0020;
	}
}

// call INT 2F (MSCDEX) thru DPMI Service (VxDCall VWin32_Int31Dispatch):
static void call_INT2F(REAL_MODE_CALL_REGS *regs)
{
	WORD	errcode = 0;
__asm {
	mov		edi,[regs]		//register buffer
	mov		ebx,0x0000002F	//BL=int-no, BH=flags(0)
	push	0x00000000		//ECX (# of stack-words)
	push	0x00000300		//EAX (DPMI function #0300h: Simulate Real Mode Interrupt)
	push	dword ptr [VWin32_Int31Dispatch]
	call	dword ptr [VxDCall]
	jnc		done			//jump when success
	mov		[errcode],ax	//AX=DPMI error code
	done:
	}
}

// call GlobalDosAlloc:
static DWORD call_GlobalDosAlloc(DWORD alloc_size)
{
__asm {
	mov		eax,alloc_size
	mov		edx,[GlobalDosAlloc]		// unsigned long GlobalDosAlloc(unsigned long);
	sub		esp,0x40
	push	eax
	call	dword ptr [QT_Thunk]
	add		esp,0x40
	shl		eax,16						// ax = protected mode memory segment
	shrd	eax,edx,16					// dx = real mode memory segment
	mov		alloc_size,eax
	}
	return alloc_size;
}

// call GlobalDosFree:
static void call_GlobalDosFree(DWORD mem_ptr)
{
__asm {
	mov		eax,mem_ptr
	mov		edx,[GlobalDosFree]			// unsigned short GlobalDosFree(unsigned short);
	sub		esp,0x40
	push	ax							// ax = protected mode memory segment
	call	dword ptr [QT_Thunk]
	add		esp,0x40
	}
}
