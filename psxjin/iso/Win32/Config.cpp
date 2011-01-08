#include <stdio.h>
#include <windows.h>

#include "../cdriso.h"

//adelikat: Changed from storing into the registry to saving into a config file
void SaveConf()
{
	char Conf_File[1024] = ".\\psxjin.ini";	//TODO: make a global for other files
	
	WritePrivateProfileString("ISO", "IsoFile", IsoFile, Conf_File);

}

void LoadConf()
{
	char Conf_File[1024] = ".\\psxjin.ini";	//TODO: make a global for other files

	GetPrivateProfileString("ISO", "IsoFile", "", &IsoFile[0], 256, Conf_File);
}