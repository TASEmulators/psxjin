/*
 * Cdrom for Psemu Pro like Emulators
 *
 * By: linuzappz <linuzappz@hotmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <zlib.h>
#include <bzlib.h>

#include <string>

#include "cdriso.h"
#include "Config.h"

#include "PsxCommon.h"

std::string CDR_iso_fileToOpen;

char IsoFile[256];
char CdDev[256];

unsigned char cdbuffer[CD_FRAMESIZE_RAW * 10];
unsigned char *pbuffer;

int Zmode; // 1 Z - 2 bz2
int fmode;						// 0 - file / 1 - Zfile
char *Ztable;

FILE *cdHandle = NULL;
char *methods[] = {
	".Z  - compress faster",
	".bz - compress better"
};

char *LibName = "TAS ISO Plugin";



long CDRinit(void) {
	return 0;
}

long CDRshutdown(void) {
	return 0;
}

void UpdateZmode() {
	int len = strlen(IsoFile);

	if (len >= 2) {
		if (!strncmp(IsoFile+(len-2), ".Z", 2)) {
			Zmode = 1; return;
		}
	}
	if (len >= 3) {
		if (!strncmp(IsoFile+(len-3), ".bz", 2)) {
			Zmode = 2; return;
		}
	}

	Zmode = 0;
}

long CDRopen(void) {
	struct stat buf;

	if (cdHandle != NULL)
		return 0;				/* it's already open */

	LoadConf();

	if (*IsoFile == 0) {
		char temp[256];

		if(CDR_iso_fileToOpen != "") {
			LoadConf();
			strcpy(IsoFile,CDR_iso_fileToOpen.c_str());
			CDR_iso_fileToOpen = "";
		}
		else
		{
			CfgOpenFile();
			LoadConf();
		}

		strcpy(temp, IsoFile);
		*IsoFile = 0;
		SaveConf();
		strcpy(IsoFile, temp);
	}

	UpdateZmode();

	if (Zmode) {
		FILE *f;
		char table[256];

		fmode = Zmode;
		strcpy(table, IsoFile);
		if (Zmode == 1) strcat(table, ".table");
		else strcat(table, ".index");
		if (stat(table, &buf) == -1) {		
			printf("Error loading %s\n", table);
			cdHandle = NULL;
			return 0;
		}
		f = fopen(table, "rb");
		Ztable = (char*)malloc(buf.st_size);
		if (Ztable == NULL) {
			cdHandle = NULL;
			return 0;
		}
		fread(Ztable, 1, buf.st_size, f);
		fclose(f);
	} else {
		fmode = 0;
		pbuffer = cdbuffer;
	}

	cdHandle = fopen(IsoFile, "rb");
	if (cdHandle == NULL) {
		SysMessage("Error loading %s\n", IsoFile);
		return -1;
	}

	return 0;
}

long CDRclose(void) {
	if (cdHandle == NULL)
		return 0;
	fclose(cdHandle);
	cdHandle = NULL;
	if (Ztable) { free(Ztable); Ztable = NULL; }

	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
long CDRgetTN(unsigned char *buffer) {
	buffer[0] = 1;
	buffer[1] = 1;

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
long CDRgetTD(unsigned char track, unsigned char *buffer) {
	buffer[2] = 0;
	buffer[1] = 2;
	buffer[0] = 0;

	return 0;
}

// read track
// time : byte 0 - minute ; byte 1 - second ; byte 2 - frame
// uses bcd format
long CDRreadTrack(unsigned char *time) {

	if (cdHandle == NULL) return -1;

//	printf ("CDRreadTrack %d:%d:%d\n", btoi(time[0]), btoi(time[1]), btoi(time[2]));

	if (!fmode) {
		fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW + 12, SEEK_SET);
		fread(cdbuffer, 1, DATA_SIZE, cdHandle);
	} else if (fmode == 1) { //.Z
		unsigned long pos, p;
		unsigned long size;
		unsigned char Zbuf[CD_FRAMESIZE_RAW];

		p = MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2]));

		pos = *(unsigned long*)&Ztable[p * 6];
		fseek(cdHandle, pos, SEEK_SET);

		p = *(unsigned short*)&Ztable[p * 6 + 4];
		fread(Zbuf, 1, p, cdHandle);

		size = CD_FRAMESIZE_RAW;
		uncompress(cdbuffer, &size, Zbuf, p);
		
		pbuffer = cdbuffer + 12;
	} else { // .bz
		unsigned long pos, p, rp;
		unsigned long size;
		unsigned char Zbuf[CD_FRAMESIZE_RAW * 10 * 2];
		int i;

		for (i=0; i<10; i++) {
			if (memcmp(time, &cdbuffer[i * CD_FRAMESIZE_RAW + 12], 3) == 0) {
				pbuffer = &cdbuffer[i * CD_FRAMESIZE_RAW + 12];

				return 0;
			}
		}

		p = MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2]));

		rp = p % 10;
		p/= 10;

		pos = *(unsigned long*)&Ztable[p * 4];
		fseek(cdHandle, pos, SEEK_SET);

		p = *(unsigned long*)&Ztable[p * 4 + 4] - pos;
		fread(Zbuf, 1, p, cdHandle);

		size = CD_FRAMESIZE_RAW * 10;
		BZ2_bzBuffToBuffDecompress((char*)cdbuffer, (unsigned int*)&size, (char*)Zbuf, p, 0, 0);

		pbuffer = cdbuffer + rp * CD_FRAMESIZE_RAW + 12;
	}

	return 0;
}

// return readed track
unsigned char* CDRgetBuffer(void) {
	return pbuffer;
}

// plays cdda audio
// sector : byte 0 - minute ; byte 1 - second ; byte 2 - frame
// does NOT uses bcd format
long CDRplay(unsigned char *sector) {
	return 0;
}

// stops cdda audio
long CDRstop(void) {
	return 0;
}

long CDRtest(void) {
	if (*IsoFile == 0)
		return 0;
	cdHandle = fopen(IsoFile, "rb");
	if (cdHandle == NULL)
		return -1;
	fclose(cdHandle);
	cdHandle = NULL;
	return 0;
}

int CDRisoFreeze(gzFile f, int Mode) {
	gzfreezelarr(cdbuffer);
	pbuffer = (unsigned char*)((intptr_t)pbuffer-(intptr_t)cdbuffer);
	gzfreezel(&pbuffer);
	pbuffer = (unsigned char*)((intptr_t)pbuffer+(intptr_t)cdbuffer);
	return 0;
}