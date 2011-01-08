// Plugin Defs
#include "Locals.h"

HINSTANCE	hInstance = (HINSTANCE)-1;	// Handle to DLL hInstance
HANDLE	hProcessHeap = (HANDLE)-1;	// Handle to Process Heap

#ifdef __GNUC__
BOOL WINAPI LibMain(HANDLE hInstance, DWORD fdwReason, PVOID fImpLoad)
#else
BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, PVOID fImpLoad)
#endif
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		::hInstance = (HINSTANCE)hInstance;
		hProcessHeap = GetProcessHeap();
		DisableThreadLibraryCalls(::hInstance);
		setup_IfType(InterfaceType_Auto);		// init if_* ptrs to dummy_*
		return TRUE;
	case DLL_PROCESS_DETACH:
		// Warning: Do NOT call any CRT library function here !!!
		close_Driver();
		return TRUE;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		return TRUE;
	}
	return FALSE;
}

