
#ifndef __MSCDEX_H__
#define	__MSCDEX_H__

//******************************************************************************
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//          WARNING: ALL THE DATA STRUCTURES MUST BE 1 BYTE ALIGNED          !!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// override default data alignement to 1 byte
#pragma pack(1)

//******************************************************************************

// DOS device driver request commands used by MSCDEX (INT 2F, AX=1510):
typedef enum {
//	DEV_INIT				= 0x00,		// INIT
//	DEV_MEDIA_CHECK			= 0x01,		// MEDIA CHECK (block devices)
//	DEV_BUILD_BPB			= 0x02,		// BUILD BPB (block devices)
	DEV_IOCTL_INPUT			= 0x03,		// IOCTL INPUT
//	DEV_INPUT				= 0x04,		// INPUT
//	DEV_INPUT_NOWAIT		= 0x05,		// NONDESTRUCTIVE INPUT, NO WAIT (character devices)
//	DEV_INPUT_STATUS		= 0x06,		// INPUT STATUS (character devices)
	DEV_INPUT_FLUSH			= 0x07,		// INPUT FLUSH (character devices)
//	DEV_OUTPUT				= 0x08,		// OUTPUT
//	DEV_OUTPUT_VERIFY		= 0x09,		// OUTPUT WITH VERIFY
//	DEV_OUTPUT_STATUS		= 0x0A,		// OUTPUT STATUS (character devices)
	DEV_OUTPUT_FLUSH		= 0x0B,		// OUTPUT FLUSH (character devices)
	DEV_IOCTL_OUTPUT		= 0x0C,		// IOCTL OUTPUT
	DEV_OPEN				= 0x0D,		// DEVICE OPEN
	DEV_CLOSE				= 0x0E,		// DEVICE CLOSE
//	DEV_REMOVABLE_MEDIA		= 0x0F,		// REMOVABLE MEDIA (block devices)
//	DEV_OUTPUT_UNTIL_BUSY	= 0x10,		// OUTPUT UNTIL BUSY
	DEV_READ_LONG			= 0x80,		// (CD-ROM) READ LONG
//	DEV_RESERVED			= 0x81,		// RESERVED
	DEV_READ_LONG_PREFETCH	= 0x82,		// (CD-ROM) READ LONG PREFETCH
	DEV_SEEK				= 0x83,		// (CD-ROM) SEEK
	DEV_PLAY_AUDIO			= 0x84,		// (CD-ROM) PLAY AUDIO
	DEV_STOP_AUDIO			= 0x85,		// (CD-ROM) STOP AUDIO
	DEV_WRITE_LONG			= 0x86,		// (CD-ROM) WRITE LONG
	DEV_WRITE_LONG_VERIFY	= 0x87,		// (CD-ROM) WRITE LONG VERIFY
	DEV_RESUME_AUDIO		= 0x88,		// (CD-ROM) RESUME AUDIO
} DEVICE_REQUEST_COMMAND;

// ioctl input codes used by MSCDEX (DEV_IOCTL_INPUT):
typedef enum {
	DEVICE_DRIVER_ADDRESS	= 0x00,		// return address of device header
	HEAD_LOCATION			= 0x01,		// location of head
//	RESERVED				= 0x02,		// RESERVED
//	ERROR_STATISTIC			= 0x03,		// error statistics
	AUDIO_CHANNEL_INFO		= 0x04,		// audio channel control
//	DRIVE_BYTES				= 0x05,		// read drive bytes
	DEVICE_STATUS			= 0x06,		// device status
	SECTOR_SIZE				= 0x07,		// return sector size
	VOLUME_SIZE				= 0x08,		// return volume size
	MEDIA_CHANGED			= 0x09,		// media changed
	AUDIO_DISK_INFO			= 0x0A,		// audio disk info
	AUDIO_TRACK_INFO		= 0x0B,		// audio track info
	AUDIO_Q_CHANNEL_INFO	= 0x0C,		// audio Q-Channel info
	AUDIO_SUBCHANNEL_INFO	= 0x0D,		// audio Sub-Channel info
	UPC_CODE				= 0x0E,		// UPC code
	AUDIO_STATUS_INFO		= 0x0F,		// audio status info
} IOCTL_INPUT_CODES;

// ioctl output codes used by MSCDEX (DEV_IOCTL_OUTPUT):
typedef enum {
	EJECT_DISK				= 0x00,		// eject disk
//	LOCK_UNLOCK_DOOR		= 0x01,		// lock/unlock door
	RESET_DRIVE				= 0x02,		// reset drive
	AUDIO_CHANNEL_CONTROL	= 0x03,		// audio channel control
//	CONTROL_STRING			= 0x04,		// write device control string
	CLOSE_TRAY				= 0x05,		// close tray
} IOCTL_OUTPUT_CODES;

//******************************************************************************

// MSCDEX 'DEVICE_REQUEST_HEADER.status' data format:
typedef struct {
	BYTE	errorcode:8;	// error code (if error bit = 1)
	BYTE	done:1;			// done
	BYTE	busy:1;			// busy (1 = audio play in progress)
	BYTE	reserved0:5;	// unused
	BYTE	error:1;		// error bit (1 = error)
} REQUEST_STATUS;

// MSCDEX 'IOCTL_INPUT_DEVICE_STATUS.parms' data format:
typedef struct {
	BYTE	open:1;			// 1 = door open, 0 = door closed
	BYTE	unlocked:1;		// 1 = door unlocked, 0 = door locked
	BYTE	rawread:1;		// 1 = supports cooked and raw reading, 0 = supports only cooked reading
	BYTE	cdrw:1;			// 1 = read/write, 0 = read only
	BYTE	playback:1;		// 1 = data read and plays audio/video tracks, 0 = data read only
	BYTE	interleaving:1;	// 1 = supports interleaving, 0 = no interleaving
	BYTE	reserved0:1;	// reserved
	BYTE	prefetch:1;		// 1 = supports prefetching requests, 0 = no prefetching
	BYTE	audiochannel:1;	// 1 = supports audio channel manipulation, 0 = no audio channel manipulation
	BYTE	redbookaddr:1;	// 1 = supports HSG and Red Book addressing modes, 0 = supports HSG addressing mode
	BYTE	reserved1:1;	// reserved
	BYTE	diskabsent:1;	// (reserved) 1 = disk absent, 0 = disk present
	BYTE	rwsubchannel:1;	// (reserved) 1 = support RW Sub Channels, 0 = no RW Sub Channels
	BYTE	reserved2:3;	// reserved
	BYTE	reserved3[2];	// reserved
} DRIVE_STATUS;

// MSCDEX 'Red Book addressing' data format:
typedef struct {
	BYTE	frame;
	BYTE	second;
	BYTE	minute;
	BYTE	reserved;
} RB_ADDR;

typedef struct {
	BYTE	channel;		// input channel: 0-3
	BYTE	volume;			// volume control: 0 (off) to 0FFH (full blast)
} AUDIO_CHANNEL;


//******************************************************************************

// DOS device driver request structs:
typedef struct {
	BYTE	len;		// length of request header
	BYTE	unit;		// subunit within device driver
	BYTE	cmd;		// command code
	REQUEST_STATUS	status;	// status (filled in by device driver)
	DWORD	reserved[2];// reserved
} DEVICE_REQUEST_HEADER;

typedef struct {
	DEVICE_REQUEST_HEADER	header;
} DEVICE_REQUEST_OPEN, DEVICE_REQUEST_CLOSE, DEVICE_REQUEST_STOP_AUDIO, DEVICE_REQUEST_RESUME_AUDIO;

typedef struct {
	DEVICE_REQUEST_HEADER	header;
	BYTE	media;		// media descriptor (block devices only)
	DWORD	addr;		// transfer address: address of a control block
	WORD	len;		// (call) number of bytes to read/write, (ret) actual number of bytes read or written
	WORD	ssn;		// starting sector number
	DWORD	pvolid;		// pointer to requested volume ID on error code 0FH
} DEVICE_REQUEST_IOCTL_INPUT, DEVICE_REQUEST_IOCTL_OUTPUT;

typedef struct {
	DEVICE_REQUEST_HEADER	header;
	BYTE	addr_mode;	// addressing mode: 00h HSG (default), 01h Phillips/Sony Red Book
	DWORD	addr;		// transfer address (ignored for command 82h)
	WORD	nsec;		// number of sectors to read (if 0 for command 82h, request is an advisory seek)
	DWORD	loc;		// starting sector number: logical sector number in HSG mode, frame/second/minute/unused in Red Book mode (HSG sector = minute * 4500 + second * 75 + frame - 150)
	BYTE	rd_mode;	// data read mode: 00h COOKED (2048 bytes per frame), 01h RAW (2352 bytes per frame, including EDC/ECC)
	BYTE	il_size;	// interleave size (number of sectors stored consecutively)
	BYTE	il_skip;	// interleave skip factor (number of sectors between consecutive portions)
} DEVICE_REQUEST_READ_LONG, DEVICE_REQUEST_READ_LONG_PREFETCH;

typedef struct {
	DEVICE_REQUEST_HEADER	header;
	BYTE	addr_mode;	// addressing mode (see above)
	DWORD	loc;		// starting sector number (see also above)
	DWORD	nsec;		// number of sectors to play
} DEVICE_REQUEST_PLAY_AUDIO;

typedef union {
	DEVICE_REQUEST_IOCTL_INPUT			ioctl_input;
	DEVICE_REQUEST_IOCTL_OUTPUT			ioctl_output;
	DEVICE_REQUEST_OPEN					open;
	DEVICE_REQUEST_CLOSE				close;
	DEVICE_REQUEST_READ_LONG			read_long;
	DEVICE_REQUEST_READ_LONG_PREFETCH	read_long_prefetch;
	DEVICE_REQUEST_PLAY_AUDIO			play_audio;
	DEVICE_REQUEST_STOP_AUDIO			stop_audio;
	DEVICE_REQUEST_RESUME_AUDIO			resume_audio;
} DEVICE_REQUEST;

//******************************************************************************

// MSCDEX IOCTL_INPUT & IOCTL_OUTPUT request control blocks data format:
typedef struct {
	BYTE	cmd;		// (in) control block code
} IOCTL_HEADER;

typedef struct {
	IOCTL_HEADER	header;
	DWORD	address;	// (out)address of device header
} IOCTL_INPUT_DEVICE_DRIVER_ADDRESS;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	mode;		// (in) addressing mode: 0 = COOKED, 1 = RAW
	DWORD	loc;		// (out)location of drive head
} IOCTL_INPUT_HEAD_LOCATION;

typedef struct {
	IOCTL_HEADER	header;
	AUDIO_CHANNEL	channels[4];	// output channel 0 to 3
} IOCTL_INPUT_AUDIO_CHANNEL_INFO, IOCTL_OUTPUT_AUDIO_CHANNEL_CONTROL;

typedef struct {
	IOCTL_HEADER	header;
	DRIVE_STATUS	parms;		// (out)device parameters
} IOCTL_INPUT_DEVICE_STATUS;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	mode;		// (in) addressing mode: 0 = COOKED, 1 = RAW
	WORD	size;		// (out)sector size: 2048 for cooked, 2352 for raw
} IOCTL_INPUT_SECTOR_SIZE;

typedef struct {
	IOCTL_HEADER	header;
	DWORD	size;		// (out)volume size: value of the lead-out track converted to binary
} IOCTL_INPUT_VOLUME_SIZE;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	state;		// (out)state: 0FFH (media has changed), 1 (media has not changed), 0 (not sure)
} IOCTL_INPUT_MEDIA_CHANGED;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	first;		// (out)lowest track number
	BYTE	last;		// (out)highest track number
	RB_ADDR	leadout;	// (out)starting point of lead-out track: F/S/M
} IOCTL_INPUT_AUDIO_DISK_INFO;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	track;		// (in) track number
	RB_ADDR	loc;		// (out)starting point of the track: F/S/M
	BYTE	ctrlinfo;	// (out)track control information
} IOCTL_INPUT_AUDIO_TRACK_INFO;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	ctrladr;	// (out)CONTROL and ADR byte
	BYTE	track;		// (out)track number
	BYTE	pointidx;	// (out)point or index
	MSF		runtrack;	// (out)running time within track: M/S/F
	BYTE	reserved;	// (out)reserved
	MSF		rundisk;	// (out)running time on the disk: M/S/F
} IOCTL_INPUT_AUDIO_Q_CHANNEL_INFO;

typedef struct {
	IOCTL_HEADER	header;
	RB_ADDR	loc;		// (in) starting frame address: F/S/M
	DWORD	addr;		// (in) transfer address: 96 bytes per sector
	DWORD	nsec;		// (in) number of sectors to read
} IOCTL_INPUT_AUDIO_SUBCHANNEL_INFO;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	ctrladr;	// (out)CONTROL and ADR byte
	BYTE	code[7];	// (out)UPC/EAN code
	BYTE	reserved;	// (out)unused
	BYTE	aframe;		// (out)
} IOCTL_INPUT_UPC_CODE;

typedef struct {
	IOCTL_HEADER	header;
	WORD	status;		// (out)audio status bits: bit 0 is set if audio is paused, bits 1-15 are reserved
	RB_ADDR	start;		// (out)starting location: start address of last play request
	RB_ADDR	end;		// (out)ending location: end address of last play request
} IOCTL_INPUT_AUDIO_STATUS_INFO;

typedef struct {
	IOCTL_HEADER	header;
} IOCTL_OUTPUT_EJECT_DISK, IOCTL_OUTPUT_RESET_DRIVE, IOCTL_OUTPUT_CLOSE_TRAY;

typedef struct {
	IOCTL_HEADER	header;
	BYTE	lock;		// lock function: 0 = unlock the door, 1 = lock the door
} IOCTL_OUTPUT_LOCK_UNLOCK_DOOR;

typedef union {
	IOCTL_INPUT_DEVICE_DRIVER_ADDRESS	device_driver_address;
	IOCTL_INPUT_HEAD_LOCATION			head_location;
	IOCTL_INPUT_AUDIO_CHANNEL_INFO		audio_channel_info;
	IOCTL_INPUT_DEVICE_STATUS			device_status;
	IOCTL_INPUT_SECTOR_SIZE				sector_size;
	IOCTL_INPUT_VOLUME_SIZE				volume_size;
	IOCTL_INPUT_MEDIA_CHANGED			media_changed;
	IOCTL_INPUT_AUDIO_DISK_INFO			audio_disk_info;
	IOCTL_INPUT_AUDIO_TRACK_INFO		audio_track_info;
	IOCTL_INPUT_AUDIO_Q_CHANNEL_INFO	audio_q_channel_info;
	IOCTL_INPUT_AUDIO_SUBCHANNEL_INFO	audio_subchannel_info;
	IOCTL_INPUT_UPC_CODE				upc_code;
	IOCTL_INPUT_AUDIO_STATUS_INFO		audio_status_info;
	IOCTL_OUTPUT_EJECT_DISK				eject_disk;
	IOCTL_OUTPUT_LOCK_UNLOCK_DOOR		lock_unlock_door;
	IOCTL_OUTPUT_RESET_DRIVE			reset_drive;
	IOCTL_OUTPUT_AUDIO_CHANNEL_CONTROL	audio_channel_control;
	IOCTL_OUTPUT_CLOSE_TRAY				close_tray;
} CONTROL_BLOCK;

//******************************************************************************
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//          WARNING: ALL THE DATA STRUCTURES MUST BE 1 BYTE ALIGNED          !!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// restore default data alignement value
#pragma pack()

//******************************************************************************

#endif //__MSCDEX_H__
