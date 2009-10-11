#include <stdio.h>
#include <windows.h>

#include "../cdriso.h"

#define GetKeyV(name, var, s, t) \
	size = s; type = t; \
	RegQueryValueEx(myKey, name, 0, &type, (LPBYTE) var, &size);

#define GetKeyVdw(name, var) \
	GetKeyV(name, var, 4, REG_DWORD);

#define SetKeyV(name, var, s, t) \
	RegSetValueEx(myKey, name, 0, t, (LPBYTE) var, s);

#define SetKeyVdw(name, var) \
	SetKeyV(name, var, 4, REG_DWORD);

void SaveConf() {
	HKEY myKey;
	DWORD myDisp;

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\PCSX-RR\\ISO", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);

	SetKeyV("IsoFile", IsoFile, sizeof(IsoFile), REG_BINARY);

	RegCloseKey(myKey);
}

void LoadConf() {
	HKEY myKey;
	DWORD type, size;

	memset(IsoFile, 0, sizeof(IsoFile));

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\PCSX-RR\\ISO", 0, KEY_ALL_ACCESS, &myKey)!=ERROR_SUCCESS) {
		SaveConf(); return;
	}

	GetKeyV("IsoFile", IsoFile, sizeof(IsoFile), REG_BINARY);

	RegCloseKey(myKey);
}
