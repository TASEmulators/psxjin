#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdriso.h"

void LoadConf() {
	FILE *f;
	char cfg[255];

	sprintf(cfg, "%s/cdriso.cfg", getenv("HOME"));
	f = fopen(cfg, "r");
	if (f == NULL) {
		strcpy(IsoFile, DEV_DEF);
		strcpy(CdDev, CDDEV_DEF);
		return;
	}
	fscanf(f, "IsoFile = %[^\n]\n", IsoFile);
	fscanf(f, "CdDev   = %[^\n]\n", CdDev);
	if (!strncmp(IsoFile, "CdDev   =", 9)) *IsoFile = 0; // quick fix
	if (*CdDev == 0) strcpy(CdDev, CDDEV_DEF);
	fclose(f);
}

void SaveConf() {
	FILE *f;
	char cfg[255];

	sprintf(cfg, "%s/cdriso.cfg", getenv("HOME"));
	f = fopen(cfg, "w");
	if (f == NULL)
		return;
	fprintf(f, "IsoFile = %s\n", IsoFile);
	fprintf(f, "CdDev   = %s\n", CdDev);
	fclose(f);
}

