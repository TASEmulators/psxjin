/*
	PSEmu Plugin Developer Kit Header definition

	(C)1998 Vision Thing

	This file can be used only to develop PSEmu Plugins
	Other usage is highly prohibited.
*/


// IMPORTANT!!!

// if you want to add return codes (any errors or warnings) just drop mail to
// plugin@psemu.com

#ifndef _PSEMU_PLUGIN_DEFS_H
#define _PSEMU_PLUGIN_DEFS_H


// header version
#define _PPDK_HEADER_VERSION		3

#define PLUGIN_VERSION				1

// plugin type returned by PSEgetLibType (types can be merged if plugin is multi type!)
#define PSE_LT_CDR					1
#define PSE_LT_GPU					2
#define PSE_LT_SPU					4
#define PSE_LT_PAD					8
#define PSE_LT_NET					16


// every function in DLL if completed sucessfully should return this value
#define PSE_ERR_SUCCESS				0

// undefined error but fatal one, that kills all functionality
#define PSE_ERR_FATAL				-1



// XXX_Init return values
// Those return values apply to all libraries
// currently obsolete - preserved for compatibilty


// initialization went OK
#define PSE_INIT_ERR_SUCCESS		0

// this driver is not configured
#define PSE_INIT_ERR_NOTCONFIGURED	-2

// this driver can not operate properly on this hardware or hardware is not detected
#define PSE_INIT_ERR_NOHARDWARE		-3



/*         GPU PlugIn          */


//  GPU_Test return values

// sucess, everything configured, and went OK.
#define PSE_GPU_ERR_SUCCESS			0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_GPU_ERR					-20


// this driver is not configured
#define PSE_GPU_ERR_NOTCONFIGURED	PSE_GPU_ERR - 1
// this driver failed Init
#define PSE_GPU_ERR_INIT			PSE_GPU_ERR - 2

// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_GPU_WARN				20




//  GPU_Query		- will be implemented soon

typedef struct
{
	unsigned long	flags;
	unsigned long	status;
	HWND			window;
	unsigned char reserved[100];
} gpuQueryS;

// gpuQueryS.flags
// if driver can operate in both modes it must support GPU_changeMode();
// this driver can operate in fullscreen mode
#define PSE_GPU_FLAGS_FULLSCREEN		1
// this driver can operate in windowed mode
#define PSE_GPU_FLAGS_WINDOWED			2


// gpuQueryS.status
// this driver cannot operate in this windowed mode
#define PSE_GPU_STATUS_WINDOWWRONG	1

//  GPU_Query	End	- will be implemented in v2




/*         CDR PlugIn          */

//	CDR_Test return values

// sucess, everything configured, and went OK.
#define PSE_CDR_ERR_SUCCESS			0

// general failure (error undefined)
#define PSE_CDR_ERR_FAILURE			-1

// ERRORS
#define PSE_CDR_ERR -40
// this driver is not configured
#define PSE_CDR_ERR_NOTCONFIGURED	PSE_CDR_ERR - 0
// if this driver is unable to read data from medium
#define PSE_CDR_ERR_NOREAD			PSE_CDR_ERR - 1

// WARNINGS
#define PSE_CDR_WARN 40
// if this driver emulates lame mode ie. can read only 2048 tracks and sector header is emulated
// this might happen to CDROMS that do not support RAW mode reading - surelly it will kill many games
#define PSE_CDR_WARN_LAMECD			PSE_CDR_WARN + 0




/*         SPU PlugIn          */

// some info retricted (now!)

// sucess, everything configured, and went OK.
#define PSE_SPU_ERR_SUCCESS 0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_SPU_ERR					-60

// this driver is not configured
#define PSE_SPU_ERR_NOTCONFIGURED	PSE_SPU_ERR - 1
// this driver failed Init
#define PSE_SPU_ERR_INIT			PSE_SPU_ERR - 2


// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_SPU_WARN				60






// might be used in port 1 (must support PADreadPort1() function)
#define PSE_PAD_USE_PORT1			1
// might be used in port 2 (must support PADreadPort2() function)
#define PSE_PAD_USE_PORT2			2



// MOUSE SCPH-1030
#define PSE_PAD_TYPE_MOUSE			1
// NEGCON - 16 button analog controller SLPH-00001
#define PSE_PAD_TYPE_NEGCON			2
// GUN CONTROLLER - gun controller SLPH-00014 from Konami
#define PSE_PAD_TYPE_GUN			3
// STANDARD PAD SCPH-1080, SCPH-1150
#define PSE_PAD_TYPE_STANDARD		4
// ANALOG JOYSTICK SCPH-1110
#define PSE_PAD_TYPE_ANALOGJOY		5
// GUNCON - gun controller SLPH-00034 from Namco
#define PSE_PAD_TYPE_GUNCON			6
// ANALOG CONTROLLER SCPH-1150
#define PSE_PAD_TYPE_ANALOGPAD		7
// NO CONTROLLER



// sucess, everything configured, and went OK.
#define PSE_PAD_ERR_SUCCESS			0
// general plugin failure (undefined error)
#define PSE_PAD_ERR_FAILURE			-1


// ERRORS
// this error might be returned as critical error but none of below
#define PSE_PAD_ERR					-80
// this driver is not configured
#define PSE_PAD_ERR_NOTCONFIGURED	PSE_PAD_ERR - 1
// this driver failed Init
#define PSE_PAD_ERR_INIT			PSE_PAD_ERR - 2


// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_PAD_WARN				80

/*         NET PlugIn v2       */
/* Added by linuzappz@pcsx.net */

/* Modes bits for NETsendData/NETrecvData */
#define PSE_NET_BLOCKING	0x00000000
#define PSE_NET_NONBLOCKING	0x00000001




#endif // _PSEMU_PLUGIN_DEFS_H
